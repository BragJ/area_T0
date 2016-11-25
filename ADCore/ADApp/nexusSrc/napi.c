/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format
  
  Application Program Interface Routines
  
  Copyright (C) 1997-2006 Mark Koennecke, Przemek Klosowski
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
 
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
  For further information, see <http://www.nexusformat.org>

----------------------------------------------------------------------------*/

/* static const char* rscid = "$Id: napi.c 1814 2012-02-07 14:37:57Z Freddie Akeroyd $"; */	/* Revision inserted by CVS */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "napi.h"
#include "nxstack.h"

/*---------------------------------------------------------------------
 Recognized and handled napimount URLS
 -----------------------------------------------------------------------*/
#define NXBADURL 0
#define NXFILE 1

/*--------------------------------------------------------------------*/
/* static int iFortifyScope; */
/*----------------------------------------------------------------------
  This is a section with code for searching the NX_LOAD_PATH
  -----------------------------------------------------------------------*/
#ifdef _WIN32
#define LIBSEP ";"
#define PATHSEP "\\"
#define THREAD_LOCAL __declspec(thread)
#else
#define LIBSEP ":"
#define PATHSEP "/"
#define THREAD_LOCAL __thread
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif /* _MSC_VER */

#include "nx_stptok.h"

#if defined(_WIN32)
/*
 *  HDF5 on windows does not do locking for multiple threads conveniently so we will implement it ourselves.
 *  Freddie Akeroyd, 16/06/2011
 */
#include <windows.h>


static CRITICAL_SECTION nx_critical;

static int nxilock()
{
    static int first_call = 1;
    if (first_call)
    {
        first_call = 0;
	InitializeCriticalSection(&nx_critical);
    }
    EnterCriticalSection(&nx_critical);
    return NX_OK;
}

static int nxiunlock(int ret)
{
    LeaveCriticalSection(&nx_critical);
    return ret;
}

#define LOCKED_CALL(__call) \
    ( nxilock() , nxiunlock(__call) )

#elif HAVE_LIBPTHREAD

#include <pthread.h>

static pthread_mutex_t nx_mutex;

#ifdef PTHREAD_MUTEX_RECURSIVE
#define RECURSIVE_LOCK PTHREAD_MUTEX_RECURSIVE
#else
#define RECURSIVE_LOCK PTHREAD_MUTEX_RECURSIVE_NP
#endif /* PTHREAD_MUTEX_RECURSIVE */

static void nx_pthread_init()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, RECURSIVE_LOCK);
    pthread_mutex_init(&nx_mutex, &attr);
}

static int nxilock()
{
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    if (pthread_once(&once_control, nx_pthread_init) != 0)
    {
        NXReportError("pthread_once failed");
	return NX_ERROR;
    }
    if (pthread_mutex_lock(&nx_mutex) != 0)
    {
        NXReportError("pthread_mutex_lock failed");
        return NX_ERROR;
    }
    return NX_OK;
}

static int nxiunlock(int ret)
{
    if (pthread_mutex_unlock(&nx_mutex) != 0)
    {
        NXReportError("pthread_mutex_unlock failed");
        return NX_ERROR;
    }
    return ret;
}

#define LOCKED_CALL(__call) \
    ( nxilock() , nxiunlock(__call) )

#else

#define LOCKED_CALL(__call) \
	   __call

#endif /* _WIN32 */

/**
 * valid NeXus names
 */
int validNXName(const char* name, int allow_colon)
{
    int i;
    if (name == NULL)
    {
	return 0;
    }
    for(i=0; i<(int)strlen(name); ++i)
    {
	if ( (name[i] >= 'a' && name[i] <= 'z') ||
	     (name[i] >= 'A' && name[i] <= 'Z') ||
	     (name[i] >= '0' && name[i] <= '9') ||
	     (name[i] == '_') )
	{
	    ;
	}
	else if (allow_colon && name[i] == ':')
	{
	    ;
	}
	else
	{
	    return 0;
	}
    }
    return 1;
}

static int64_t* dupDimsArray(int* dims_array, int rank)
{
	int i;
	int64_t* dims64 = (int64_t*)malloc(rank * sizeof(int64_t));
	if (dims64 != NULL)
	{
		for(i=0; i<rank; ++i)
		{
	    		dims64[i] = dims_array[i];
		}
	}
	return dims64;
}

/*---------------------------------------------------------------------
 wrapper for getenv. This is a future proofing thing for porting to OS
 which have different ways of accessing environment variables
 --------------------------------------------------------------------*/ 
static char *nxgetenv(const char *name){
  return getenv(name);
}
/*----------------------------------------------------------------------*/
static int canOpen(char *filename){
  FILE *fd = NULL;

  fd = fopen(filename,"r");
  if(fd != NULL){
    fclose(fd);
    return 1;
  } else {
    return 0;
  }
} 
/*--------------------------------------------------------------------*/
static char *locateNexusFileInPath(char *startName){
  char *loadPath = NULL, *testPath = NULL, *pPtr = NULL;
  char pathPrefix[256];
  int length;

  if(canOpen(startName)){
    return strdup(startName);
  }

  loadPath = nxgetenv("NX_LOAD_PATH");
  if(loadPath == NULL){
    /* file not found will be issued by upper level code */
    return strdup(startName);
  }

  pPtr = stptok(loadPath,pathPrefix,255,LIBSEP);
  while(pPtr != NULL){
    length = (int)strlen(pathPrefix) + (int)strlen(startName) + (int)strlen(PATHSEP) + 2;
    testPath = (char*)malloc(length*sizeof(char));
    if(testPath == NULL){
      return strdup(startName);
    }
    memset(testPath,0,length*sizeof(char));
    strcpy(testPath, pathPrefix);
    strcat(testPath,PATHSEP);
    strcat(testPath,startName);
    if(canOpen(testPath)){
      return(testPath);
    }
    free(testPath);
    pPtr = stptok(pPtr,pathPrefix,255,LIBSEP);
  }
  return  strdup(startName);
}
/*------------------------------------------------------------------------
  HDF-5 cache size special stuff
  -------------------------------------------------------------------------*/
long nx_cacheSize =  1024000; /* 1MB, HDF-5 default */

NXstatus NXsetcache(long newVal)
{
  if(newVal > 0)
  {
    nx_cacheSize = newVal;
    return NX_OK;
  }
  return NX_ERROR;
}

#ifdef NXXML
/*-----------------------------------------------------------------------*/
static NXstatus NXisXML(CONSTCHAR *filename)
{
  FILE *fd = NULL;
  char line[132];

  fd = fopen(filename,"r");
  if(fd) {
    int fgets_ok = (fgets(line,131,fd) != NULL);
    fclose(fd);
    if (fgets_ok) {
      if(strstr(line,"?xml") != NULL){
	return NX_OK;
      }
    }
    else {
      return NX_ERROR;
    }
  }
  return NX_ERROR;
}
#endif

/*-------------------------------------------------------------------------*/
  static void NXNXNXReportError(void *pData, char *string)
  {
    fprintf(stderr, "%s \n", string);
  }
  /*---------------------------------------------------------------------*/

  static void *NXEHpData = NULL;
  static void (*NXEHIReportError)(void *pData, char *string) = NXNXNXReportError;
#ifdef HAVE_TLS
  static THREAD_LOCAL void *NXEHpTData = NULL;
  static THREAD_LOCAL void (*NXEHIReportTError)(void *pData, char *string) = NULL;
#endif

  void NXIReportError(void *pData, char *string) {
	fprintf(stderr, "Your application uses NXIReportError, but its first parameter is ignored now - you should use NXReportError.");
	NXReportError(string);
  }

  void NXReportError(char *string) {
#ifdef HAVE_TLS
	if (NXEHIReportTError) {
		(*NXEHIReportTError)(NXEHpTData, string);
		return;
	} 
#endif
	if (NXEHIReportError) {
	    (*NXEHIReportError)(NXEHpData, string);
	}
  }

  /*---------------------------------------------------------------------*/
  extern void NXMSetError(void *pData, void (*NewError)(void *pD, char *text))
  {
    NXEHpData = pData;
    NXEHIReportError = NewError;
  }
/*----------------------------------------------------------------------*/
  extern void NXMSetTError(void *pData, void (*NewError)(void *pD, char *text))
  {
#ifdef HAVE_TLS
    NXEHpTData = pData;
    NXEHIReportTError = NewError;
#else
    NXMSetError(pData, NewError);
#endif
  }
/*----------------------------------------------------------------------*/
extern ErrFunc NXMGetError(){
#ifdef HAVE_TLS
	if (NXEHIReportTError) {
		return NXEHIReportTError;
	}
#endif
  return NXEHIReportError;
}

/*----------------------------------------------------------------------*/
static void NXNXNoReport(void *pData, char *string){
  /* do nothing */
}  
/*----------------------------------------------------------------------*/

static ErrFunc last_global_errfunc = NXNXNXReportError;
#ifdef HAVE_TLS
static THREAD_LOCAL ErrFunc last_thread_errfunc = NULL;
#endif

extern void NXMDisableErrorReporting()
{
#ifdef HAVE_TLS
	if (NXEHIReportTError) {
		last_thread_errfunc = NXEHIReportTError;
		NXEHIReportTError = NXNXNoReport;
	 	return;
	} 
#endif
	if (NXEHIReportError) {
	    last_global_errfunc = NXEHIReportError;
	    NXEHIReportError = NXNXNoReport;
	}
}

extern void NXMEnableErrorReporting()
{
#ifdef HAVE_TLS
	if (last_thread_errfunc) {
		NXEHIReportTError = last_thread_errfunc;
		last_thread_errfunc = NULL;
	 	return;
	} 
#endif
	if (last_global_errfunc) {
	    NXEHIReportError = last_global_errfunc;
	    last_global_errfunc = NULL;
	}
}

/*----------------------------------------------------------------------*/
#ifdef HDF5
#include "napi5.h"
#endif
#ifdef HDF4
#include "napi4.h"
#endif
#ifdef NXXML
#include "nxxml.h"
#endif  
  /* ---------------------------------------------------------------------- 
  
                          Definition of NeXus API

   ---------------------------------------------------------------------*/
static int determineFileTypeImpl(CONSTCHAR *filename)
{
  FILE *fd = NULL;
  int iRet;
  
  /*
    this is for reading, check for existence first
  */
  fd = fopen(filename,"r");
  if(fd == NULL){
    return -1;
  }
  fclose(fd);
#ifdef HDF5
  iRet=H5Fis_hdf5((const char*)filename);
  if( iRet > 0){
    return 2;
  }
#endif  
#ifdef HDF4
  iRet=Hishdf((const char*)filename);
  if( iRet > 0){
    return 1;
  }
#endif
#ifdef NXXML
  iRet = NXisXML(filename);
  if(iRet == NX_OK){
    return 3;
  }
#endif
  /*
    file type not recognized
  */
  return 0;
}

static int determineFileType(CONSTCHAR *filename)
{
    return LOCKED_CALL(determineFileTypeImpl(filename));
}

/*---------------------------------------------------------------------*/
static pNexusFunction handleToNexusFunc(NXhandle fid){
  pFileStack fileStack = NULL;
  fileStack = (pFileStack)fid;
  if(fileStack != NULL){
    return peekFileOnStack(fileStack);
  } else {
    return NULL;
  }
}
/*--------------------------------------------------------------------*/
static NXstatus   NXinternalopen(CONSTCHAR *userfilename, NXaccess am, 
           pFileStack fileStack);
/*----------------------------------------------------------------------*/
NXstatus   NXopen(CONSTCHAR *userfilename, NXaccess am, NXhandle *gHandle){
  int status;
  pFileStack fileStack = NULL;

  *gHandle = NULL;
  fileStack = makeFileStack();
  if(fileStack == NULL){
    NXReportError("ERROR: no memory to create filestack");
      return NX_ERROR;
  }
  status = NXinternalopen(userfilename,am,fileStack);
  if(status == NX_OK){
    *gHandle = fileStack;
  }

  return status;
}
/*-----------------------------------------------------------------------*/
static NXstatus   NXinternalopenImpl(CONSTCHAR *userfilename, NXaccess am, pFileStack fileStack)
  {
    int hdf_type=0;
    int iRet=0;
    NXhandle hdf5_handle = NULL;
    pNexusFunction fHandle = NULL;
    NXstatus retstat = NX_ERROR;
    char error[1024];
    char *filename = NULL;
    int my_am = (am & NXACCMASK_REMOVEFLAGS);
        
    /* configure fortify 
    iFortifyScope = Fortify_EnterScope();
    Fortify_CheckAllMemory();
    */
    
    /*
      allocate data
    */
    fHandle = (pNexusFunction)malloc(sizeof(NexusFunction));
    if (fHandle == NULL) {
      NXReportError("ERROR: no memory to create Function structure");
      return NX_ERROR;
    }
    memset(fHandle, 0, sizeof(NexusFunction)); /* so any functions we miss are NULL */
       
    /*
      test the strip flag. Elimnate it for the rest of the tests to work
    */
    fHandle->stripFlag = 1;
    if(am & NXACC_NOSTRIP){
      fHandle->stripFlag = 0;
      am = (NXaccess)(am & ~NXACC_NOSTRIP);
    }
    fHandle->checkNameSyntax = 0;
    if (am & NXACC_CHECKNAMESYNTAX) {
        fHandle->checkNameSyntax = 1;
        am = (NXaccess)(am & ~NXACC_CHECKNAMESYNTAX);
    }


    if (my_am==NXACC_CREATE) {
      /* HDF4 will be used ! */
      hdf_type=1;
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATE4) {
      /* HDF4 will be used ! */
      hdf_type=1;   
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATE5) {
      /* HDF5 will be used ! */
      hdf_type=2;   
      filename = strdup(userfilename);
    } else if (my_am==NXACC_CREATEXML) {
      /* XML will be used ! */
      hdf_type=3;   
      filename = strdup(userfilename);
    } else {
      filename = locateNexusFileInPath((char *)userfilename);
      if(filename == NULL){
	NXReportError("Out of memory in NeXus-API");
	free(fHandle);
	return NX_ERROR;
      }
      /* check file type hdf4/hdf5/XML for reading */
      iRet = determineFileType(filename);
      if(iRet < 0) {
	snprintf(error,1023,"failed to open %s for reading",
		 filename);
	NXReportError(error);
	free(filename);
	return NX_ERROR;
      }
      if(iRet == 0){
	snprintf(error,1023,"failed to determine filetype for %s ",
		 filename);
	NXReportError(error);
	free(filename);
	free(fHandle);
	return NX_ERROR;
      }
      hdf_type = iRet;
    }
    if(filename == NULL){
	NXReportError("Out of memory in NeXus-API");
	return NX_ERROR;
    }

    if (hdf_type==1) {
      /* HDF4 type */
#ifdef HDF4
    NXhandle hdf4_handle = NULL;
      retstat = NX4open((const char *)filename,am,&hdf4_handle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=hdf4_handle;
      NX4assignFunctions(fHandle);
      pushFileStack(fileStack,fHandle,filename);
#else
      NXReportError(
         "ERROR: Attempt to create HDF4 file when not linked with HDF4");
      retstat = NX_ERROR;
#endif /* HDF4 */
      free(filename);
      return retstat; 
    } else if (hdf_type==2) {
      /* HDF5 type */
#ifdef HDF5
      retstat = NX5open(filename,am,&hdf5_handle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=hdf5_handle;
      NX5assignFunctions(fHandle);
      pushFileStack(fileStack,fHandle, filename);
#else
      NXReportError(
	 "ERROR: Attempt to create HDF5 file when not linked with HDF5");
      retstat = NX_ERROR;
#endif /* HDF5 */
      free(filename);
      return retstat;
    } else if(hdf_type == 3){
      /*
	XML type
      */
#ifdef NXXML
    NXhandle xmlHandle = NULL;
      retstat = NXXopen(filename,am,&xmlHandle);
      if(retstat != NX_OK){
	free(fHandle);
	free(filename);
	return retstat;
      }
      fHandle->pNexusData=xmlHandle;
      NXXassignFunctions(fHandle);
      pushFileStack(fileStack,fHandle, filename);
#else
      NXReportError(
	 "ERROR: Attempt to create XML file when not linked with XML");
      retstat = NX_ERROR;
#endif
    } else {
      NXReportError(
          "ERROR: Format not readable by this NeXus library");
      retstat = NX_ERROR;
    }
    if (filename != NULL) {
 	free(filename); 
    }
    return retstat;
  }

static NXstatus   NXinternalopen(CONSTCHAR *userfilename, NXaccess am, pFileStack fileStack)
{
    return LOCKED_CALL(NXinternalopenImpl(userfilename, am, fileStack));
}


NXstatus  NXreopen(NXhandle pOrigHandle, NXhandle* pNewHandle)
{
      pFileStack newFileStack;
	  pFileStack origFileStack = (pFileStack)pOrigHandle;
	pNexusFunction fOrigHandle = NULL, fNewHandle = NULL;
	  *pNewHandle = NULL;
	  newFileStack = makeFileStack();
	  if(newFileStack == NULL){
          NXReportError ("ERROR: no memory to create filestack");
          return NX_ERROR;
      }
	  // The code below will only open the last file on a stack
	  // for the moment raise an error, but this behaviour may be OK
	  if (fileStackDepth(origFileStack) > 0)    
	  {
		NXReportError (
          "ERROR: handle stack referes to many files - cannot reopen");
		  return NX_ERROR;
	  }
	  fOrigHandle = peekFileOnStack(origFileStack);
	  if (fOrigHandle->nxreopen == NULL)
	  {
		NXReportError (
          "ERROR: NXreopen not implemented for this underlying file format");
		  return NX_ERROR;
	  }
	  fNewHandle = (NexusFunction*)malloc(sizeof(NexusFunction));
	  memcpy(fNewHandle, fOrigHandle, sizeof(NexusFunction));
	  LOCKED_CALL(fNewHandle->nxreopen( fOrigHandle->pNexusData, &(fNewHandle->pNexusData) ));
	  pushFileStack( newFileStack, fNewHandle, peekFilenameOnStack(origFileStack) );
	  *pNewHandle = newFileStack;
	  return NX_OK;
}


/* ------------------------------------------------------------------------- */

  NXstatus  NXclose (NXhandle *fid)
  { 
    NXhandle hfil; 
    int status;
    pFileStack fileStack = NULL;
    pNexusFunction pFunc=NULL;
    if (*fid == NULL)
    {
	return NX_OK;
    }
    fileStack = (pFileStack)*fid;
    pFunc = peekFileOnStack(fileStack);
    hfil = pFunc->pNexusData;
    status = LOCKED_CALL(pFunc->nxclose(&hfil));
    pFunc->pNexusData = hfil;
    free(pFunc);
    popFileStack(fileStack);
    if(fileStackDepth(fileStack) < 0){
      killFileStack(fileStack);
      *fid = NULL;
    }
    /* we can't set fid to NULL always as the handle points to a stack of files for external file support */
    /* 
    Fortify_CheckAllMemory();
    */

    return status;   
  }

  /*-----------------------------------------------------------------------*/   

  NXstatus  NXmakegroup (NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) 
  {
     char buffer[256];
     pNexusFunction pFunc = handleToNexusFunc(fid);
     if ( pFunc->checkNameSyntax && (nxclass != NULL) /* && !strncmp("NX", nxclass, 2) */ && !validNXName(name, 0) )
     {
        sprintf(buffer, "ERROR: invalid characters in group name \"%s\"", name);
        NXReportError(buffer);
	return NX_ERROR;
     }
     return LOCKED_CALL(pFunc->nxmakegroup(pFunc->pNexusData, name, nxclass));   
  }
  /*------------------------------------------------------------------------*/
static int analyzeNapimount(char *napiMount, char *extFile, int extFileLen, 
			    char *extPath, int extPathLen){
  char *pPtr = NULL, *path = NULL;
  int length;

  memset(extFile,0,extFileLen);
  memset(extPath,0,extPathLen);
  pPtr = strstr(napiMount,"nxfile://");
  if(pPtr == NULL){
    return NXBADURL;
  }
  path = strrchr(napiMount,'#');
  if(path == NULL){
    length = (int)strlen(napiMount) - 9;
    if(length > extFileLen){
      NXReportError("ERROR: internal errro with external linking");
      return NXBADURL;
    }
    memcpy(extFile,pPtr+9,length);
    strcpy(extPath,"/");
    return NXFILE;
  } else {
    pPtr += 9;
    length = (int)(path - pPtr);
    if(length > extFileLen){
      NXReportError("ERROR: internal errro with external linking");
      return NXBADURL;
    }
    memcpy(extFile,pPtr,length);
    length = (int)strlen(path-1);
    if(length > extPathLen){
      NXReportError("ERROR: internal error with external linking");
      return NXBADURL;
    }
    strcpy(extPath,path+1);
    return NXFILE;
  }
  return NXBADURL;
}
  /*------------------------------------------------------------------------*/

  NXstatus  NXopengroup (NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass)
  {
    int status, attStatus, type = NX_CHAR, length = 1023;
    NXaccess access = NXACC_READ;
    NXlink breakID;
    pFileStack fileStack;    
    char nxurl[1024], exfile[512], expath[512];
    pNexusFunction pFunc = NULL;

    fileStack = (pFileStack)fid;
    pFunc = handleToNexusFunc(fid);

    status = LOCKED_CALL(pFunc->nxopengroup(pFunc->pNexusData, name, nxclass));  
    if(status == NX_OK){
      pushPath(fileStack,name);
    }
    NXMDisableErrorReporting();
    attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
    NXMEnableErrorReporting();
    if(attStatus == NX_OK){
      /*
	this is an external linking group
      */
      status = analyzeNapimount(nxurl,exfile,511,expath,511);
      if(status == NXBADURL){
	return NX_ERROR;
      }
      status = NXinternalopen(exfile, access, fileStack);
      if(status == NX_ERROR){
	return status;
      }
      status = NXopenpath(fid, expath);
      NXgetgroupID(fid,&breakID);
      setCloseID(fileStack,breakID);
    }
    
    return status;
  } 

  /* ------------------------------------------------------------------- */

  NXstatus  NXclosegroup (NXhandle fid)
  {
    int status;
    pFileStack fileStack = NULL;
    NXlink closeID, currentID;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    fileStack = (pFileStack)fid;
    if(fileStackDepth(fileStack) == 0){
      status = LOCKED_CALL(pFunc->nxclosegroup(pFunc->pNexusData));  
      if(status == NX_OK){
	popPath(fileStack);
      }
      return status;
    } else {
      /* we have to check for leaving an external file */
      NXgetgroupID(fid,&currentID);
      peekIDOnStack(fileStack,&closeID);
      if(NXsameID(fid,&closeID,&currentID) == NX_OK){
	NXclose(&fid);
	status = NXclosegroup(fid);
      } else {
	status = LOCKED_CALL(pFunc->nxclosegroup(pFunc->pNexusData));
	if(status == NX_OK){
	  popPath(fileStack);
	}
      }
      return status;
    }
  }   

  /* --------------------------------------------------------------------- */
  
  NXstatus  NXmakedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                                  int rank, int dimensions[])
  {
	  int status;
	  int64_t* dims64 = dupDimsArray(dimensions, rank);
      status = NXmakedata64(fid, name, datatype, rank, dims64);
	  free(dims64);
	  return status;
  }

  NXstatus  NXmakedata64 (NXhandle fid, CONSTCHAR *name, int datatype, 
                                  int rank, int64_t dimensions[])
  {
    char buffer[256];
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if ( pFunc->checkNameSyntax && !validNXName(name, 0) )
    {
        sprintf(buffer, "ERROR: invalid characters in dataset name \"%s\"", name);
        NXReportError(buffer);
	return NX_ERROR;
    }
    return LOCKED_CALL(pFunc->nxmakedata64(pFunc->pNexusData, name, datatype, rank, dimensions)); 
  }

 /* --------------------------------------------------------------------- */
  
  NXstatus  NXcompmakedata (NXhandle fid, CONSTCHAR *name, int datatype, 
                           int rank, int dimensions[], int compress_type, int chunk_size[])
  {
	  int status;
	  int64_t* dims64 = dupDimsArray(dimensions, rank);
	  int64_t* chunk64 = dupDimsArray(chunk_size, rank);
      status = NXcompmakedata64(fid, name, datatype, rank, dims64, compress_type, chunk64);
	  free(dims64);
	  free(chunk64);
	  return status;
  } 
  
  NXstatus  NXcompmakedata64 (NXhandle fid, CONSTCHAR *name, int datatype, 
                           int rank, int64_t dimensions[], int compress_type, int64_t chunk_size[])
  {
    char buffer[256];
    pNexusFunction pFunc = handleToNexusFunc(fid); 
    if ( pFunc->checkNameSyntax && !validNXName(name, 0) )
    {
        sprintf(buffer, "ERROR: invalid characters in dataset name \"%s\"", name);
        NXReportError(buffer);
	return NX_ERROR;
    }
    return LOCKED_CALL(pFunc->nxcompmakedata64 (pFunc->pNexusData, name, datatype, rank, dimensions, compress_type, chunk_size)); 
  } 
 
  /* --------------------------------------------------------------------- */

  NXstatus  NXcompress (NXhandle fid, int compress_type)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid); 
    return LOCKED_CALL(pFunc->nxcompress (pFunc->pNexusData, compress_type)); 
  }
  
 
  /* --------------------------------------------------------------------- */
  
  NXstatus  NXopendata (NXhandle fid, CONSTCHAR *name)
  {
    int status, attStatus, type = NX_CHAR, length = 1023;
    NXaccess access = NXACC_READ;
    NXlink breakID;
    pFileStack fileStack;
    char nxurl[1024], exfile[512], expath[512];
    pNexusFunction pFunc = NULL;

    fileStack = (pFileStack)fid;
    pFunc = handleToNexusFunc(fid);
    status = LOCKED_CALL(pFunc->nxopendata(pFunc->pNexusData, name)); 

    if(status == NX_OK){
      pushPath(fileStack,name);
    }

    NXMDisableErrorReporting();
    attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
    NXMEnableErrorReporting();
    if(attStatus == NX_OK){
      /*
        this is an external linking group
      */
      status = analyzeNapimount(nxurl,exfile,511,expath,511);
      if(status == NXBADURL){
        return NX_ERROR;
      }
      status = NXinternalopen(exfile, access, fileStack);
      if(status == NX_ERROR){
        return status;
      }
      status = NXopenpath(fid,expath);
      NXgetdataID(fid,&breakID);
      setCloseID(fileStack,breakID);
    }

    return status;
  } 

  /* ----------------------------------------------------------------- */
    
  NXstatus NXclosedata (NXhandle fid)
  { 
    int status;
    pFileStack fileStack = NULL;
    NXlink closeID, currentID;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    fileStack = (pFileStack)fid;

    if(fileStackDepth(fileStack) == 0){
      status = LOCKED_CALL(pFunc->nxclosedata(pFunc->pNexusData));
      if(status == NX_OK){
        popPath(fileStack);
      }
      return status;
    } else {
      /* we have to check for leaving an external file */
      NXgetdataID(fid,&currentID);
      peekIDOnStack(fileStack,&closeID);
      if(NXsameID(fid,&closeID,&currentID) == NX_OK){
        NXclose(&fid);
        status = NXclosedata(fid);
      } else {
        status = LOCKED_CALL(pFunc->nxclosedata(pFunc->pNexusData));
        if(status == NX_OK){
          popPath(fileStack);
        }
      }
      return status;
    }
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputdata (NXhandle fid, const void *data)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxputdata(pFunc->pNexusData, data));
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputattr (NXhandle fid, CONSTCHAR *name, const void *data, 
                                  int datalen, int iType)
  {
    char buffer[256];
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if (datalen > 1 && iType != NX_CHAR)
    {
	NXReportError("NXputattr: numeric arrays are not allowed as attributes - only character strings and single numbers");
	return NX_ERROR;
    }
    if ( pFunc->checkNameSyntax && !validNXName(name, 0) )
    {
        sprintf(buffer, "ERROR: invalid characters in attribute name \"%s\"", name);
        NXReportError(buffer);
	return NX_ERROR;
    }
    return LOCKED_CALL(pFunc->nxputattr(pFunc->pNexusData, name, data, datalen, iType));
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXputslab (NXhandle fid, const void *data, const int iStart[], const int iSize[])
  {
	  int i, iType, rank;
	  int64_t iStart64[NX_MAXRANK], iSize64[NX_MAXRANK];
	  if (NXgetinfo64(fid, &rank, iStart64, &iType) != NX_OK)
	  {
		  return NX_ERROR;
	  }
	  for(i=0; i < rank; ++i)
	  {
		  iStart64[i] = iStart[i];
		  iSize64[i] = iSize[i];
	  }
	  return NXputslab64(fid, data, iStart64, iSize64);
  }

  NXstatus  NXputslab64 (NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxputslab64(pFunc->pNexusData, data, iStart, iSize));
  }

  /* ------------------------------------------------------------------- */

  NXstatus  NXgetdataID (NXhandle fid, NXlink* sRes)
  {  
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetdataID(pFunc->pNexusData, sRes));
  }


  /* ------------------------------------------------------------------- */

  NXstatus  NXmakelink (NXhandle fid, NXlink* sLink)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxmakelink(pFunc->pNexusData, sLink));
  }
  /* ------------------------------------------------------------------- */

  NXstatus  NXmakenamedlink (NXhandle fid, CONSTCHAR *newname,  NXlink* sLink)
  {
    char buffer[256];
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if ( pFunc->checkNameSyntax && !validNXName(newname, 0) )
    {
        sprintf(buffer, "ERROR: invalid characters in link name \"%s\"", newname);
        NXReportError(buffer);
	return NX_ERROR;
    }
    return LOCKED_CALL(pFunc->nxmakenamedlink(pFunc->pNexusData, newname, sLink));
  }
  /* --------------------------------------------------------------------*/
  NXstatus  NXopensourcegroup(NXhandle fid)
  {
    char target_path[512];
    int status, type = NX_CHAR, length = 511;

    status = NXgetattr(fid,"target",target_path,&length,&type);
    if(status != NX_OK)
    {
      NXReportError("ERROR: item not linked");
      return NX_ERROR;
    }
    return NXopengrouppath(fid,target_path);
  }
  /*----------------------------------------------------------------------*/

  NXstatus  NXflush(NXhandle *pHandle)
  {
    NXhandle hfil; 
    pFileStack fileStack = NULL;
    int status;
   
    pNexusFunction pFunc=NULL;
    fileStack = (pFileStack)*pHandle;
    pFunc = peekFileOnStack(fileStack);
    hfil = pFunc->pNexusData;
    status =  LOCKED_CALL(pFunc->nxflush(&hfil));
    pFunc->pNexusData = hfil;
    return status; 
  }


  /*-------------------------------------------------------------------------*/
  
  
  NXstatus  NXmalloc (void** data, int rank, 
				   const int dimensions[], int datatype)
  {
	  int status;
	  int64_t* dims64 = dupDimsArray((int *)dimensions, rank);
	  status = NXmalloc64(data, rank, dims64, datatype);
	  free(dims64);
	  return status;
  }

  NXstatus  NXmalloc64 (void** data, int rank, 
				   const int64_t dimensions[], int datatype)
  {
    int i;
    size_t size = 1;
    *data = NULL;
    for(i=0; i<rank; i++)
	{
        size *= (size_t)dimensions[i];
	}
    if ((datatype == NX_CHAR) || (datatype == NX_INT8) 
	    || (datatype == NX_UINT8)) {
        /* allow for terminating \0 */
      size += 2;
      }
      else if ((datatype == NX_INT16) || (datatype == NX_UINT16)) {
      size *= 2;
      }    
      else if ((datatype == NX_INT32) || (datatype == NX_UINT32) 
	       || (datatype == NX_FLOAT32)) {
        size *= 4;
      }    
      else if ((datatype == NX_INT64) || (datatype == NX_UINT64)){
        size *= 8;
      }    
      else if (datatype == NX_FLOAT64) {
        size *= 8;
      }
      else {
        NXReportError(
			"ERROR: NXmalloc - unknown data type in array");
        return NX_ERROR;
    }
    *data = (void*)malloc(size);
     memset(*data,0,size);
    return NX_OK;
  }
    
  /*-------------------------------------------------------------------------*/

  NXstatus  NXfree (void** data)
  {
    if (data == NULL) {
       NXReportError( "ERROR: passing NULL to NXfree");
       return NX_ERROR;
    }
    if (*data == NULL) {
       NXReportError("ERROR: passing already freed pointer to NXfree");
       return NX_ERROR;
    }
    free(*data);
    *data = NULL;
    return NX_OK;
  }

  /* --------------------------------------------------------------------- */
           
 
  NXstatus  NXgetnextentry (NXhandle fid, NXname name, NXname nxclass, int *datatype)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetnextentry(pFunc->pNexusData, name, nxclass, datatype));  
  }
/*----------------------------------------------------------------------*/
/*
**  TRIM.C - Remove leading, trailing, & excess embedded spaces
**
**  public domain by Bob Stout
*/
#define NUL '\0'

char *nxitrim(char *str)
{
      char *ibuf = str;
      int i = 0;

      /*
      **  Trap NULL
      */

      if (str)
      {
            /*
            **  Remove leading spaces (from RMLEAD.C)
            */

            for (ibuf = str; *ibuf && isspace(*ibuf); ++ibuf)
                  ;
	    str = ibuf;

            /*
            **  Remove trailing spaces (from RMTRAIL.C)
            */
	    i = (int)strlen(str);
            while (--i >= 0)
            {
                  if (!isspace(str[i]))
                        break;
            }
            str[++i] = NUL;
      }
      return str;
}
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetdata (NXhandle fid, void *data)
  {
    int status, type, rank;
	int64_t iDim[NX_MAXRANK];
    char *pPtr, *pPtr2;

    pNexusFunction pFunc = handleToNexusFunc(fid);
    status = LOCKED_CALL(pFunc->nxgetinfo64(pFunc->pNexusData, &rank, iDim, &type)); /* unstripped size if string */
    /* only strip one dimensional strings */
    if ( (type == NX_CHAR) && (pFunc->stripFlag == 1) && (rank == 1) )
    {
		pPtr = (char*)malloc((size_t)iDim[0]+5);
        memset(pPtr, 0, (size_t)iDim[0]+5);
        status = LOCKED_CALL(pFunc->nxgetdata(pFunc->pNexusData, pPtr)); 
		pPtr2 = nxitrim(pPtr);
		strncpy((char*)data, pPtr2, strlen(pPtr2)); /* not NULL terminated by default */
		free(pPtr);
    }
    else
    {
        status = LOCKED_CALL(pFunc->nxgetdata(pFunc->pNexusData, data)); 
    }
    return status;
  }
/*---------------------------------------------------------------------------*/
  NXstatus  NXgetrawinfo64 (NXhandle fid, int *rank, 
				    int64_t dimension[], int *iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetinfo64(pFunc->pNexusData, rank, dimension, iType));
  }

  NXstatus  NXgetrawinfo (NXhandle fid, int *rank, 
				    int dimension[], int *iType)
  {
    int i, status;
	int64_t dims64[NX_MAXRANK];
    pNexusFunction pFunc = handleToNexusFunc(fid);
    status = LOCKED_CALL(pFunc->nxgetinfo64(pFunc->pNexusData, rank, dims64, iType));
	for(i=0; i < *rank; ++i)
	{
		dimension[i] = (int)dims64[i];
	}
    return status;
  }
  /*-------------------------------------------------------------------------*/
 
  NXstatus  NXgetinfo (NXhandle fid, int *rank, 
				    int dimension[], int *iType)
  {
	  int i, status;
	  int64_t dims64[NX_MAXRANK];
	  status = NXgetinfo64(fid, rank, dims64, iType);
	  for(i=0; i < *rank; ++i)
	  {
		  dimension[i] = (int)dims64[i];
	  }
	  return status;
  }

  NXstatus  NXgetinfo64 (NXhandle fid, int *rank, 
				    int64_t dimension[], int *iType)
  {
    int status;
    char *pPtr = NULL;
    pNexusFunction pFunc = handleToNexusFunc(fid);
	*rank = 0;
    status = LOCKED_CALL(pFunc->nxgetinfo64(pFunc->pNexusData, rank, dimension, iType));
    /*
      the length of a string may be trimmed....
    */
    /* only strip one dimensional strings */
    if((*iType == NX_CHAR) && (pFunc->stripFlag == 1) && (*rank == 1)){
      pPtr = (char *)malloc((size_t)(dimension[0]+1)*sizeof(char));
      if(pPtr != NULL){
	memset(pPtr,0,(size_t)(dimension[0]+1)*sizeof(char));
	LOCKED_CALL(pFunc->nxgetdata(pFunc->pNexusData, pPtr));
	dimension[0] = strlen(nxitrim(pPtr));
	free(pPtr);
      }
    } 
    return status;
  }
  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetslab (NXhandle fid, void *data, 
				    const int iStart[], const int iSize[])
  {
	  int i, iType, rank;
	  int64_t iStart64[NX_MAXRANK], iSize64[NX_MAXRANK];
	  if (NXgetinfo64(fid, &rank, iStart64, &iType) != NX_OK)
	  {
		  return NX_ERROR;
	  }
	  for(i=0; i < rank; ++i)
	  {
		  iStart64[i] = iStart[i];
		  iSize64[i] = iSize[i];
	  }
	  return NXgetslab64(fid, data, iStart64, iSize64);
  }
  
  NXstatus  NXgetslab64 (NXhandle fid, void *data, 
				    const int64_t iStart[], const int64_t iSize[])
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetslab64(pFunc->pNexusData, data, iStart, iSize));
  }
  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetnextattr (NXhandle fileid, NXname pName,
                                     int *iLength, int *iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return LOCKED_CALL(pFunc->nxgetnextattr(pFunc->pNexusData, pName, iLength, iType));  
  }
 

  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetattr (NXhandle fid, char *name, void *data, int* datalen, int* iType)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetattr(pFunc->pNexusData, name, data, datalen, iType));  
  }


  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetattrinfo (NXhandle fid, int *iN)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetattrinfo(pFunc->pNexusData, iN));  
  }


  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetgroupID (NXhandle fileid, NXlink* sRes)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return LOCKED_CALL(pFunc->nxgetgroupID(pFunc->pNexusData, sRes));  
  }

  /*-------------------------------------------------------------------------*/

  NXstatus  NXgetgroupinfo (NXhandle fid, int *iN, NXname pName, NXname pClass)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxgetgroupinfo(pFunc->pNexusData, iN, pName, pClass));  
  }

  
  /*-------------------------------------------------------------------------*/

  NXstatus  NXsameID (NXhandle fileid, NXlink* pFirstID, NXlink* pSecondID)
  {
    pNexusFunction pFunc = handleToNexusFunc(fileid);
    return LOCKED_CALL(pFunc->nxsameID(pFunc->pNexusData, pFirstID, pSecondID));  
  }

  /*-------------------------------------------------------------------------*/
  
  NXstatus  NXinitattrdir (NXhandle fid)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxinitattrdir(pFunc->pNexusData));
  }
  /*-------------------------------------------------------------------------*/
  
  NXstatus  NXsetnumberformat (NXhandle fid, 
					    int type, char *format)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    if(pFunc->nxsetnumberformat != NULL)
    {
      return LOCKED_CALL(pFunc->nxsetnumberformat(pFunc->pNexusData,type,format));
    }
    else
    {
      /*
	silently ignore this. Most NeXus file formats do not require
        this
      */
      return NX_OK;
    }
  }
  
  
  /*-------------------------------------------------------------------------*/
 
  NXstatus  NXinitgroupdir (NXhandle fid)
  {
    pNexusFunction pFunc = handleToNexusFunc(fid);
    return LOCKED_CALL(pFunc->nxinitgroupdir(pFunc->pNexusData));
  }
/*----------------------------------------------------------------------*/
 NXstatus  NXinquirefile(NXhandle handle, char *filename, 
				int filenameBufferLength){
  pFileStack fileStack;
  char *pPtr = NULL;
  int length, status;

  pNexusFunction pFunc = handleToNexusFunc(handle);
   if (pFunc->nxnativeinquirefile != NULL) {

  	status = LOCKED_CALL(pFunc->nxnativeinquirefile(pFunc->pNexusData, filename, filenameBufferLength));
	if (status < 0) {
		return NX_ERROR;
	} else {
		return NX_OK;
	}

   }

  fileStack = (pFileStack)handle;
  pPtr = peekFilenameOnStack(fileStack);
  if(pPtr != NULL){
    length = (int)strlen(pPtr);
    if(length > filenameBufferLength){
      length = filenameBufferLength -1;
    }
    memset(filename,0,filenameBufferLength);
    memcpy(filename,pPtr, length);
    return NX_OK;
  } else {
    return NX_ERROR;
  }
}
/*------------------------------------------------------------------------*/
NXstatus  NXisexternalgroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass, 
			    char *url, int urlLen){
  int status, attStatus, length = 1023, type = NX_CHAR;
  char nxurl[1024];

  pNexusFunction pFunc = handleToNexusFunc(fid);

   if (pFunc->nxnativeisexternallink != NULL) {
  	status = LOCKED_CALL(pFunc->nxnativeisexternallink(pFunc->pNexusData, name, url, urlLen));
	if (status == NX_OK) {
		return NX_OK;
	}
	// need to continue, could still be old style link
   }

  status = LOCKED_CALL(pFunc->nxopengroup(pFunc->pNexusData, name, nxclass));
  if(status != NX_OK){
    return status;
  }
  NXMDisableErrorReporting();
  attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
  NXMEnableErrorReporting();
  LOCKED_CALL(pFunc->nxclosegroup(pFunc->pNexusData));
  if(attStatus == NX_OK){
    length = (int)strlen(nxurl);
    if(length >= urlLen){
      length = urlLen - 1;
    }
    memset(url,0,urlLen);
    memcpy(url,nxurl,length);
    return attStatus;
  } else {
    return NX_ERROR;
  }
}
/*------------------------------------------------------------------------*/
NXstatus  NXisexternaldataset(NXhandle fid, CONSTCHAR *name,
			    char *url, int urlLen){
  int status, attStatus, length = 1023, type = NX_CHAR;
  char nxurl[1024];

  pNexusFunction pFunc = handleToNexusFunc(fid);

   if (pFunc->nxnativeisexternallink != NULL) {
  	status = LOCKED_CALL(pFunc->nxnativeisexternallink(pFunc->pNexusData, name, url, urlLen));
	if (status == NX_OK) {
		return NX_OK;
	}
	// need to continue, could still be old style link
   }

  status = LOCKED_CALL(pFunc->nxopendata(pFunc->pNexusData, name));
  if(status != NX_OK){
    return status;
  }
  NXMDisableErrorReporting();
  attStatus = NXgetattr(fid,"napimount",nxurl,&length, &type);
  NXMEnableErrorReporting();
  LOCKED_CALL(pFunc->nxclosedata(pFunc->pNexusData));
  if(attStatus == NX_OK){
    length = (int)strlen(nxurl);
    if(length >= urlLen){
      length = urlLen - 1;
    }
    memset(url,0,urlLen);
    memcpy(url,nxurl,length);
    return attStatus;
  } else {
    return NX_ERROR;
  }
}
/*------------------------------------------------------------------------*/
NXstatus  NXlinkexternal(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass, CONSTCHAR *url) {
  int status, type = NX_CHAR, length=1024, urllen;
  char nxurl[1024], exfile[512], expath[512];
  pNexusFunction pFunc = handleToNexusFunc(fid);

  // in HDF5 we support external linking natively
  if (pFunc->nxnativeexternallink != NULL) {
        urllen = (int)strlen(url);
        memset(nxurl, 0, length);
        if(urllen >= length){
          urllen = length - 1;
        }
        memcpy(nxurl, url, urllen);
        status = analyzeNapimount(nxurl,exfile,511,expath,511);
        if(status != NX_OK){
           return status;
        }
	status = LOCKED_CALL(pFunc->nxnativeexternallink(pFunc->pNexusData, name, exfile, expath));
        if(status != NX_OK){
           return status;
        }
        return NX_OK;
  }

  NXMDisableErrorReporting();
  LOCKED_CALL(pFunc->nxmakegroup(pFunc->pNexusData,name,nxclass));
  NXMEnableErrorReporting();

  status = LOCKED_CALL(pFunc->nxopengroup(pFunc->pNexusData,name,nxclass));
  if(status != NX_OK){
    return status;
  }
  length = (int)strlen(url);
  status = NXputattr(fid, "napimount",url,length, type);
  if(status != NX_OK){
    return status;
  }
  LOCKED_CALL(pFunc->nxclosegroup(pFunc->pNexusData));
  return NX_OK;
}
/*------------------------------------------------------------------------*/
NXstatus  NXlinkexternaldataset(NXhandle fid, CONSTCHAR *name, 
			 CONSTCHAR *url){
  int status, type = NX_CHAR, length=1024, urllen;
  char nxurl[1024], exfile[512], expath[512];
  pNexusFunction pFunc = handleToNexusFunc(fid);
  int rank = 1;
  int64_t dims[1] = {1};
  
  //TODO cut and paste

  // in HDF5 we support external linking natively
  if (pFunc->nxnativeexternallink != NULL) {
        urllen = (int)strlen(url);
        memset(nxurl, 0, length);
        if(urllen > length){
          urllen = length - 1;
        }
        memcpy(nxurl, url, urllen);
        status = analyzeNapimount(nxurl,exfile,511,expath,511);
        if(status != NX_OK){
           return status;
        }
	status = LOCKED_CALL(pFunc->nxnativeexternallink(pFunc->pNexusData, name, exfile, expath));
        if(status != NX_OK){
           return status;
        }
        return NX_OK;
  }


  status = LOCKED_CALL(pFunc->nxmakedata64(pFunc->pNexusData, name, NX_CHAR, rank, dims));
  if(status != NX_OK){
    return status;
  }
  status = LOCKED_CALL(pFunc->nxopendata(pFunc->pNexusData, name));
  if(status != NX_OK){
    return status;
  }
  length = (int)strlen(url);
  status = NXputattr(fid, "napimount",url,length, type);
  if(status != NX_OK){
    return status;
  }
  LOCKED_CALL(pFunc->nxclosedata(pFunc->pNexusData));
  return NX_OK;
}
/*------------------------------------------------------------------------
  Implementation of NXopenpath 
  --------------------------------------------------------------------------*/
static int isDataSetOpen(NXhandle hfil)
{
  NXlink id;
  
  /*
    This uses the (sensible) feauture that NXgetdataID returns NX_ERROR
    when no dataset is open
  */
  if(NXgetdataID(hfil,&id) == NX_ERROR)
  {
    return 0;
  }
  else 
  {
    return 1;
  }
}
/*----------------------------------------------------------------------*/
static int isRoot(NXhandle hfil)
{
  NXlink id;
  
  /*
    This uses the feauture that NXgetgroupID returns NX_ERROR
    when we are at root level
  */
  if(NXgetgroupID(hfil,&id) == NX_ERROR)
  {
    return 1;
  }
  else 
  {
    return 0;
  }
}
/*--------------------------------------------------------------------
  copies the next path element into element.
  returns a pointer into path beyond the extracted path
  ---------------------------------------------------------------------*/
static char *extractNextPath(char *path, NXname element)
{
  char *pPtr, *pStart;
  int length;

  pPtr = path;
  /*
    skip over leading /
  */
  if(*pPtr == '/')
  {
    pPtr++;
  }
  pStart = pPtr;
  
  /*
    find next /
  */
  pPtr = strchr(pStart,'/');
  if(pPtr == NULL)
  {
    /*
      this is the last path element
    */
    strcpy(element,pStart);
    return NULL;
  } else {
    length = (int)(pPtr - pStart);
    strncpy(element,pStart,length);
    element[length] = '\0';
  }
  return pPtr + 1;
}
/*-------------------------------------------------------------------*/
static NXstatus gotoRoot(NXhandle hfil)
{
    int status;

    if(isDataSetOpen(hfil))
    {
      status = NXclosedata(hfil);
      if(status == NX_ERROR)
      {
	return status;
      }
    }
    while(!isRoot(hfil))
    {
      status = NXclosegroup(hfil);
      if(status == NX_ERROR)
      {
	return status;
      }
    }
    return NX_OK;
}
/*--------------------------------------------------------------------*/
static int isRelative(char *path)
{
  if(path[0] == '.' && path[1] == '.')
    return 1;
  else
    return 0;
}
/*------------------------------------------------------------------*/
static NXstatus moveOneDown(NXhandle hfil)
{
  if(isDataSetOpen(hfil))
  {
    return NXclosedata(hfil);
  } 
  else
  {
    return NXclosegroup(hfil);
  }
}
/*-------------------------------------------------------------------
  returns a pointer to the remaining path string to move up
  --------------------------------------------------------------------*/
static char *moveDown(NXhandle hfil, char *path, int *code)
{
  int status;
  char *pPtr;

  *code = NX_OK;

  if(path[0] == '/')
  {
    *code = gotoRoot(hfil);
    return path;
  } 
  else 
  {
    pPtr = path;
    while(isRelative(pPtr))
    {
      status = moveOneDown(hfil);
      if(status == NX_ERROR)
      {
	*code = status;
	return pPtr;
      }
      pPtr += 3;
    }
    return pPtr;
  }
} 
/*--------------------------------------------------------------------*/
static NXstatus stepOneUp(NXhandle hfil, char *name)
{
  int datatype;
  NXname name2, xclass;
  char pBueffel[256];  

  /*
    catch the case when we are there: i.e. no further stepping
    necessary. This can happen with paths like ../
  */
  if (strlen(name) < 1) {
      return NX_OK;
  }
  
  NXinitgroupdir(hfil);

  while(NXgetnextentry(hfil, name2, xclass, &datatype) != NX_EOD)
  {
    if(strcmp(name2,name) == 0)
    {
      if(strcmp(xclass,"SDS") == 0)
      {
	return NXopendata(hfil,name);
      } else {
	return NXopengroup(hfil,name,xclass);
      }
    }
  }
  snprintf(pBueffel,255,"ERROR: NXopenpath cannot step into %s",name);
  NXReportError( pBueffel);
  return NX_ERROR;              
}
/*--------------------------------------------------------------------*/
static NXstatus stepOneGroupUp(NXhandle hfil, char *name)
{
  int datatype;
  NXname name2, xclass;
  char pBueffel[256];  

  /*
    catch the case when we are there: i.e. no further stepping
    necessary. This can happen with paths like ../
  */
  if(strlen(name) < 1)
  {
      return NX_OK;
  }
  
  NXinitgroupdir(hfil);
  while(NXgetnextentry(hfil,name2,xclass,&datatype) != NX_EOD)
  {
    
    if(strcmp(name2,name) == 0)
    {
      if(strcmp(xclass,"SDS") == 0) {
	return NX_EOD;
      } 
      else
      {
	return NXopengroup(hfil,name,xclass);
      }
    }
  }
  snprintf(pBueffel,255,"ERROR: NXopenpath cannot step into %s",name);
  NXReportError( pBueffel);
  return NX_ERROR;              
}
/*---------------------------------------------------------------------*/
NXstatus  NXopenpath(NXhandle hfil, CONSTCHAR *path)
{
  int status, run = 1;
  NXname pathElement;
  char *pPtr;

  if(hfil == NULL || path == NULL)
  {
    NXReportError(
     "ERROR: NXopendata needs both a file handle and a path string");
    return NX_ERROR;
  }

  pPtr = moveDown(hfil,(char *)path,&status);
  if(status != NX_OK)
  {
    NXReportError( 
		    "ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while(run == 1)
  {
    pPtr = extractNextPath(pPtr, pathElement);
    status = stepOneUp(hfil,pathElement);
    if(status != NX_OK)
    {
      return status;
    }
    if(pPtr == NULL)
    {
      run = 0;
    }
  }
  return NX_OK;
}
/*---------------------------------------------------------------------*/
NXstatus  NXopengrouppath(NXhandle hfil, CONSTCHAR *path)
{
  int status, run = 1;
  NXname pathElement;
  char *pPtr;

  if(hfil == NULL || path == NULL)
  {
    NXReportError(
     "ERROR: NXopendata needs both a file handle and a path string");
    return NX_ERROR;
  }

  pPtr = moveDown(hfil,(char *)path,&status);
  if(status != NX_OK)
  {
    NXReportError( 
		    "ERROR: NXopendata failed to move down in hierarchy");
    return status;
  }

  while(run == 1)
  {
    pPtr = extractNextPath(pPtr, pathElement);
    status = stepOneGroupUp(hfil,pathElement);
    if(status == NX_ERROR)
    {
      return status;
    }
    if(pPtr == NULL || status == NX_EOD)
    {
      run = 0;
    }
  }
  return NX_OK;
}
/*---------------------------------------------------------------------*/
NXstatus NXIprintlink(NXhandle fid, NXlink* link)
{
     pNexusFunction pFunc = handleToNexusFunc(fid);
     return LOCKED_CALL(pFunc->nxprintlink(pFunc->pNexusData, link));   
}
/*----------------------------------------------------------------------*/
NXstatus NXgetpath(NXhandle fid, char *path, int pathlen){
  int status;
  pFileStack fileStack = NULL;

  fileStack = (pFileStack)fid;
  status = buildPath(fileStack,path,pathlen);
  if(status != 1){
    return NX_ERROR;
  } 
  return NX_OK;
}

/*--------------------------------------------------------------------
  format NeXus time. Code needed in every NeXus file driver
  ---------------------------------------------------------------------*/
char *NXIformatNeXusTime(){
    time_t timer;
    char* time_buffer = NULL;
    struct tm *time_info;
    const char* time_format;
    long gmt_offset;
#ifdef USE_FTIME
    struct timeb timeb_struct;
#endif 

    time_buffer = (char *)malloc(64*sizeof(char));
    if(!time_buffer){
      NXReportError("Failed to allocate buffer for time data");
      return NULL;
    }

#ifdef NEED_TZSET
    tzset();
#endif 
    time(&timer);
#ifdef USE_FTIME
    ftime(&timeb_struct);
    gmt_offset = -timeb_struct.timezone * 60;
    if (timeb_struct.dstflag != 0)
    {
        gmt_offset += 3600;
    }
#else
    time_info = gmtime(&timer);
    if (time_info != NULL)
    {
        gmt_offset = (long)difftime(timer, mktime(time_info));
    }
    else
    {
        NXReportError( 
        "Your gmtime() function does not work ... timezone information will be incorrect\n");
        gmt_offset = 0;
    }
#endif 
    time_info = localtime(&timer);
    if (time_info != NULL)
    {
        if (gmt_offset < 0)
        {
            time_format = "%04d-%02d-%02dT%02d:%02d:%02d-%02d:%02d";
        }
        else
        {
            time_format = "%04d-%02d-%02dT%02d:%02d:%02d+%02d:%02d";
        }
        sprintf(time_buffer, time_format,
            1900 + time_info->tm_year,
            1 + time_info->tm_mon,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec,
            abs(gmt_offset / 3600),
            abs((gmt_offset % 3600) / 60)
        );
    }
    else
    {
        strcpy(time_buffer, "1970-01-01T00:00:00+00:00");
    }
    return time_buffer;
}
/*----------------------------------------------------------------------
                 F77 - API - Support - Routines
  ----------------------------------------------------------------------*/
  /*
   * We store the whole of the NeXus file in the array - that way
   * we can just pass the array name to C as it will be a valid
   * NXhandle. We could store the NXhandle value in the FORTRAN array
   * instead, but that would mean writing far more wrappers
   */
  NXstatus  NXfopen(char * filename, NXaccess* am, 
                                 NXhandle pHandle)
  {
	NXstatus ret;
 	NXhandle fileid = NULL;
	ret = NXopen(filename, *am, &fileid);
	if (ret == NX_OK)
	{
	  memcpy(pHandle, fileid, getFileStackSize());
	}
	else
	{
	  memset(pHandle, 0, getFileStackSize());
	}
	if (fileid != NULL)
	{
	    free(fileid);
	}
	return ret;
  }
/* 
 * The pHandle from FORTRAN is a pointer to a static FORTRAN
 * array holding the NexusFunction structure. We need to malloc()
 * a temporary copy as NXclose will try to free() this
 */
  NXstatus  NXfclose (NXhandle pHandle)
  {
    NXhandle h;
    NXstatus ret;
    h = (NXhandle)malloc(getFileStackSize());
    memcpy(h, pHandle, getFileStackSize());
    ret = NXclose(&h);		/* does free(h) */
    memset(pHandle, 0, getFileStackSize());
    return ret;
  }
  
/*---------------------------------------------------------------------*/  
  NXstatus  NXfflush(NXhandle pHandle)
  {
    NXhandle h;
    NXstatus ret;
    h = (NXhandle)malloc(getFileStackSize());
    memcpy(h, pHandle, getFileStackSize());
    ret = NXflush(&h);		/* modifies and reallocates h */
    memcpy(pHandle, h, getFileStackSize());
    return ret;
  }
/*----------------------------------------------------------------------*/
  NXstatus  NXfmakedata(NXhandle fid, char *name, int *pDatatype,
		int *pRank, int dimensions[])
  {
    NXstatus ret;
    static char buffer[256];
    int i, *reversed_dimensions;
    reversed_dimensions = (int*)malloc(*pRank * sizeof(int));
    if (reversed_dimensions == NULL)
    {
        sprintf (buffer, 
        "ERROR: Cannot allocate space for array rank of %d in NXfmakedata", 
                *pRank);
        NXReportError( buffer);
	return NX_ERROR;
    }
/*
 * Reverse dimensions array as FORTRAN is column major, C row major
 */
    for(i=0; i < *pRank; i++)
    {
	reversed_dimensions[i] = dimensions[*pRank - i - 1];
    }
    ret = NXmakedata(fid, name, *pDatatype, *pRank, reversed_dimensions);
    free(reversed_dimensions);
    return ret;
  }

/*-----------------------------------------------------------------------*/
  NXstatus  NXfcompmakedata(NXhandle fid, char *name, 
                int *pDatatype,
		int *pRank, int dimensions[],
                int *compression_type, int chunk[])
  {
    NXstatus ret;
    static char buffer[256];
    int i, *reversed_dimensions, *reversed_chunk;
    reversed_dimensions = (int*)malloc(*pRank * sizeof(int));
    reversed_chunk = (int*)malloc(*pRank * sizeof(int));
    if (reversed_dimensions == NULL || reversed_chunk == NULL)
    {
        sprintf (buffer, 
      "ERROR: Cannot allocate space for array rank of %d in NXfcompmakedata", 
         *pRank);
        NXReportError( buffer);
	return NX_ERROR;
    }
/*
 * Reverse dimensions array as FORTRAN is column major, C row major
 */
    for(i=0; i < *pRank; i++)
    {
	reversed_dimensions[i] = dimensions[*pRank - i - 1];
	reversed_chunk[i] = chunk[*pRank - i - 1];
    }
    ret = NXcompmakedata(fid, name, *pDatatype, *pRank, 
        reversed_dimensions,*compression_type, reversed_chunk);
    free(reversed_dimensions);
    free(reversed_chunk);
    return ret;
  }
/*-----------------------------------------------------------------------*/
  NXstatus  NXfcompress(NXhandle fid, int *compr_type)
  { 
      return NXcompress(fid,*compr_type);
  }
/*-----------------------------------------------------------------------*/
  NXstatus  NXfputattr(NXhandle fid, const char *name, const void *data, 
                                   int *pDatalen, int *pIType)
  {
    return NXputattr(fid, name, data, *pDatalen, *pIType);
  }


  /*
   * implement snprintf when it is not available 
   */
  int nxisnprintf(char* buffer, int len, const char* format, ... )
  {
	  int ret;
	  va_list valist;
	  va_start(valist,format);
	  ret = vsprintf(buffer, format, valist);
	  va_end(valist);
	  return ret;
  }

/*--------------------------------------------------------------------------*/
NXstatus NXfgetpath(NXhandle fid, char *path, int *pathlen)
{
  return NXgetpath(fid,path,*pathlen);
}

const char* NXgetversion()
{
    return NEXUS_VERSION ;
}

