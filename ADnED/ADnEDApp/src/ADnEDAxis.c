/**
 * Functions for an aSub record to generate an array for an axis scale on a X/Y plot.
 *
 * Matt Pearson
 * March 2015.
 *
 * The record definition should look something like:
 * 
 * # ///
 * # /// INPA - Start value
 * # /// INPB - Size of the desired scale array (must be <=NOVA)
 * # /// INPC - Bin size
 * # /// NOVA - Max size of the output array (cannot change)
 * # ///
 * record(aSub, "$(P)$(R)Det$(DET):TOF:XAxis")
 * {
 *    field(INAM, "aSubInit")
 *    field(SNAM, "aSubProcess")
 *    field(PREC, "5")
 *    field(INPA, "$(P)$(R)Det$(DET):TOF:XAxis_Start")    //     
 *    field(INPB, "$(P)$(R)Det$(DET):TOF:XAxis_Size")   //size PV  
 *    field(INPC, "$(P)$(R)Det$(DET):TOF:XAxis_Bin")  // Bin size
 *    field(FTA, "DOUBLE")
 *    field(FTB, "LONG")
 *    field(FTC, "DOUBLE")
 *    field(FTVA, "DOUBLE")
 *    field(NOVA, "$(TOFXSIZE)") <- this is the max size of the TOF waveform for DETX
 * }
 *
 */

#include <registryFunction.h>
#include <string.h>
#include <stdio.h>
#include <dbCommon.h>
#include <aSubRecord.h>
#include <epicsExport.h>
#include <cantProceed.h>



long ADnEDAxisInit(struct aSubRecord *psub)
{
  if (psub->nova < 1) {
    psub->nova  = 1;
  }
  psub->dpvt = (double *)callocMustSucceed(psub->nova, sizeof(double), "ADnEDAxis calloc failed");
  return(0);
}

long ADnEDAxisProcess(struct aSubRecord *psub)
{
  int i = 0;
  int maxsize = psub->nova;   // 1600
  double start = ((double *)psub->a)[0];   // 0
  int size = ((int *)psub->b)[0];          //1600
  double bin = ((double *)psub->c)[0];    // ?? 
  //printf("start ==%f ; size ==%d ; bin ==%f\n",start,size,bin);
  if (size > maxsize) {
    return 1;  // return erro  info
  }
  
  double *pData = psub->dpvt;        
  double point = size*bin;  // size is not " User Size "
  for (i=0; i<maxsize; ++i) { 
    pData[i] = point;    

  }
  point = start;             //   0
  for (i=0; i<size; ++i) {   //  1600
    pData[i] = point;
    point += bin;
  }

  memcpy(psub->vala, pData, maxsize*sizeof(double));
    printf("start ==%f ; name= %s; VALA==%d; maxsize==%d; size ==%d ; bin ==%f \n",start,psub->name,psub->vala,maxsize,size,bin);
  return(0);
}




static registryFunctionRef aSubRef[] = {
    {"ADnEDAxisInit",(REGISTRYFUNCTION)ADnEDAxisInit},
    {"ADnEDAxisProcess",(REGISTRYFUNCTION)ADnEDAxisProcess}
};

void ADnEDAxis(void)
{
    registryFunctionRefAdd(aSubRef,NELEMENTS(aSubRef));
}

epicsExportRegistrar(ADnEDAxis);
