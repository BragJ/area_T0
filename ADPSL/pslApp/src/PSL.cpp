/* PSL.cpp
 *
 * This is a driver for Photonic Sciences Ltd. CCD detectors.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  Sept. 21, 2010
 *
 */
 
#include <stdio.h>
#include <string.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsStdlib.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <iocsh.h>

#include <asynOctetSyncIO.h>
#include <asynCommonSyncIO.h>

#include "ADDriver.h"
#include <epicsExport.h>

#include <string>
#include <set>
using namespace std;

// Messages to/from server.  Needs to be big enough for binary reads of images that seem to come in 2048 blocks
#define MAX_MESSAGE_SIZE 4096
#define MAX_FILENAME_LEN 256
#define PSL_SERVER_TIMEOUT 2.0
#define PSL_GET_IMAGE_TIMEOUT 20.0
#define MAX_CHOICES 64


#define PSLCameraNameString  "PSL_CAMERA_NAME"
#define PSLTIFFCommentString "PSL_TIFF_COMMENT"

typedef std::set<std::string> choice_t;


static const char *driverName = "PSL";

/** Driver for Photonic Sciencies Ltd. CCD detector; communicates with the PSL servr over a TCP/IP
  * socket.
  * The server must be started with the PSL Software Menu/Connexion. 
  */
class PSL : public ADDriver {
public:
    PSL(const char *portName, const char *PSLPort,
           int maxBuffers, size_t maxMemory,
           int priority, int stackSize);
                 
    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                                size_t nElements, size_t *nIn);
    void report(FILE *fp, int details);
    void PSLTask();             /**< This should be private but is called from C, must be public */
    epicsEventId stopEventId_;   /**< This should be private but is accessed from C, must be public */

protected:
    int PSLCameraName_;
    #define FIRST_PSL_PARAM PSLCameraName_
    int PSLTIFFComment_;
    #define LAST_PSL_PARAM  PSLTIFFComment_

private:                                        
    /* These are the methods that are new to this class */
    asynStatus writeReadServer(const char *output, char *input, size_t maxChars, double timeout);
    asynStatus writeReadServer(const char *output, double timeout);
    asynStatus writeReadServer(const char *output);
    asynStatus openCamera(int cameraId);
    asynStatus getConfig();
    asynStatus getImage();
    asynStatus getVersion();
    void getChoices(const char *command, choice_t& choices);
    const std::string& getChoiceFromIndex(choice_t& choices, int index);
    asynStatus doEnumCallbacks();
    asynStatus startAcquire();
   
    /* Our data */
    epicsEventId startEventId_;
    char toServer_[MAX_MESSAGE_SIZE];
    char fromServer_[MAX_MESSAGE_SIZE];
    size_t nRead_;
    size_t nWrite_;
    asynUser *pasynUserServer_;
    asynUser *pasynUserCommon_;
    float serverVersion_;
    choice_t validOptions_;
    choice_t cameraNameChoices_;
    choice_t triggerModeChoices_;
    choice_t recordFormatChoices_;
    int nCameras_;
};
#define NUM_PSL_PARAMS ((int)(&LAST_PSL_PARAM - &FIRST_PSL_PARAM + 1))


static void PSLTaskC(void *drvPvt)
{
    PSL *pPvt = (PSL *)drvPvt;
    
    pPvt->PSLTask();
}



/** Constructor for PSL driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data, 
  * and sets reasonable default values the parameters defined in this class, asynNDArrayDriver, and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] serverPort The name of the asyn port driver previously created with drvAsynIPPortConfigure
  *            connected to the PSL_server program.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
PSL::PSL(const char *portName, const char *serverPort,
                                int maxBuffers, size_t maxMemory,
                                int priority, int stackSize)

    : ADDriver(portName, 1, NUM_PSL_PARAMS, maxBuffers, maxMemory,
               asynEnumMask, asynEnumMask,    /* Implements asynEnum interface */
               ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)
{
    nCameras_ = 1;
    int status = asynSuccess;
    const char *functionName = "PSL";

    /* Create the epicsEvents for signaling to the PSL task when acquisition starts and stops */
    startEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!startEventId_) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s epicsEventCreate failure for start event\n", 
            driverName, functionName);
        return;
    }
    stopEventId_ = epicsEventCreate(epicsEventEmpty);
    if (!stopEventId_) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s epicsEventCreate failure for stop event\n", 
            driverName, functionName);
        return;
    }

    status = createParam(PSLCameraNameString,  asynParamInt32, &PSLCameraName_);  
    status = createParam(PSLTIFFCommentString, asynParamOctet, &PSLTIFFComment_);  
    
    /* Connect to server */
    status = pasynOctetSyncIO->connect(serverPort, 0, &pasynUserServer_, NULL);
    status = pasynCommonSyncIO->connect(serverPort, 0, &pasynUserCommon_, NULL);
    status = getVersion();
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, incorrect server version\n",
            driverName, functionName);
        return;
    }
    getChoices("GetCamList", cameraNameChoices_);
    // Add the multi-camera configuration since it is not returned by GetCamList
    cameraNameChoices_.insert("multiconf");
    
    // Open first camera in list to start with
    openCamera(0);

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "PSL");
    status |= setStringParam (ADModel, "CCD");
    status |= setIntegerParam(NDDataType,  NDUInt16);
    status |= setIntegerParam(ADImageMode, ADImageSingle);
    status |= setDoubleParam (ADAcquirePeriod, 0.);
    status |= setDoubleParam(ADAcquireTime, 1.0);
    status |= setIntegerParam(ADNumImages, 1);
       
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: unable to set camera parameters\n", 
            driverName, functionName);
        return;
    }
    
    /* Create the thread that collects the data */
    status = (epicsThreadCreate("PSLTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)PSLTaskC,
                                this) == NULL);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s epicsThreadCreate failure for data collection task\n", 
            driverName, functionName);
        return;
    }
}

asynStatus PSL::openCamera(int cameraId)
{
    asynStatus status;
    char cameraModel[MAX_FILENAME_LEN];
    int maxSizeX, maxSizeY;
    const char *cameraName = getChoiceFromIndex(cameraNameChoices_, cameraId).c_str();

    status = writeReadServer("Close");
    epicsSnprintf(toServer_, sizeof(toServer_), "Open;%s", cameraName);
    status = writeReadServer(toServer_);
    epicsSnprintf(toServer_, sizeof(toServer_), "Select;%s", cameraName);
    status = writeReadServer(toServer_);
    status = writeReadServer("GetCamNum");
    nCameras_ = atoi(fromServer_);
    status = writeReadServer("GetCam");
    sscanf(fromServer_, "%s", cameraModel);
    setStringParam(ADModel, cameraModel);
    setStringParam(ADManufacturer, "PSL");
    status = writeReadServer("GetMaximumSize");
    sscanf(fromServer_, "(%d,%d)", &maxSizeX, &maxSizeY);
    setIntegerParam(ADMaxSizeX, maxSizeX);
    setIntegerParam(ADMaxSizeY, maxSizeY);
    getChoices("GetOptions",                  validOptions_);
    getChoices("GetOptionRange;RecordFormat", recordFormatChoices_);
    getChoices("GetOptionRange;TriggerMode",  triggerModeChoices_);
    getConfig();
    doEnumCallbacks();
    return status;
}

void PSL::getChoices(const char *command, choice_t& choices)
{
    char *optionPtr = fromServer_;
    char *pBracket = fromServer_;
    asynStatus status;
    static const char *functionName = "getChoices";
    
    status = writeReadServer(command);
    if (status != asynSuccess)
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s::%s error in write/read to server %d\n",
                  driverName, functionName, status);
    while ((pBracket = strchr(++pBracket, (int)'[')) != NULL);
    if (nCameras_ > 1) {
        optionPtr = strtok(fromServer_, "[]");
    } else {
        optionPtr = fromServer_;
    }
    // Look for ' and , delimited options
    optionPtr = strtok(optionPtr, "',[] ");
    if (optionPtr == NULL) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s::%s Options are NULL\n", 
        driverName, functionName);
        return;
    }
    choices.clear();
    while (optionPtr != NULL) {
        choices.insert(std::string(optionPtr));
        optionPtr = strtok(NULL, "',[] ");
    }
}

const std::string& PSL::getChoiceFromIndex(choice_t& choices, int index)
{
    choice_t::iterator it;
    int i;
    
    for (i=0, it=choices.begin(); i<index && it!=choices.end(); i++, it++);
    return *it;
}  

asynStatus PSL::writeReadServer(const char *output)
{
    return writeReadServer(output, fromServer_, sizeof(fromServer_), PSL_SERVER_TIMEOUT);
}


asynStatus PSL::writeReadServer(const char *output, double timeout)
{
    return writeReadServer(output, fromServer_, sizeof(fromServer_), timeout);
}


asynStatus PSL::writeReadServer(const char *output, char *input, size_t maxChars, double timeout)
{
    asynStatus status;
    int eomReason;
    asynUser *pasynUser = pasynUserServer_;
    const char *functionName="writeReadServer";

    // We need to connect to the device each time
    status = pasynCommonSyncIO->disconnectDevice(pasynUserCommon_);
    status = pasynCommonSyncIO->connectDevice(pasynUserCommon_);
    status = pasynOctetSyncIO->writeRead(pasynUser, output, strlen(output), 
                                         input, maxChars, timeout,
                                         &nWrite_, &nRead_, &eomReason);
                                        
    if (status) asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s, status=%d, sent\n%s\n",
                    driverName, functionName, status, output);

    /* Set output string so it can get back to EPICS */
    setStringParam(ADStringToServer, output);
    setStringParam(ADStringFromServer, input);
    callParamCallbacks();

    return status;
}

asynStatus PSL::getVersion()
{
    asynStatus status;
    asynUser *pasynUser = pasynUserServer_;
    const char *expectedResponse = "PSLViewer-";
    const char *functionName = "getVersion";
    status = writeReadServer("GetVersion");
    if (status) goto bad;
    if (strstr(fromServer_, expectedResponse) == 0)  goto bad;
    if (sscanf(fromServer_ + strlen(expectedResponse), "%f", &serverVersion_) != 1) goto bad;    
    if (serverVersion_ < 4.3) goto bad;
    return asynSuccess;
    
    bad:
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s, ERROR, unexpected version string = %s.\n",
              driverName, functionName, fromServer_);
    return asynError;
}

asynStatus PSL::getConfig()
{
    int minX, minY, sizeX, sizeY, binX, binY, right, bottom, reverseX, reverseY;
    int i;
    choice_t::iterator it;
    int fileNumber;
    double exposure;
    double gain;
    asynStatus status;
    char *pStart;
    static const char* functionName="getConfig";
    
    status = writeReadServer("GetSize");
    if (status == asynSuccess) {
        sscanf(fromServer_, "(%d,%d)", &sizeX, &sizeY);
        setIntegerParam(NDArraySizeX, sizeX);
        setIntegerParam(NDArraySizeY, sizeY);
    }
    // GetMode is always supported
    status = writeReadServer("GetMode");
    if (status == asynSuccess) {
        setIntegerParam(NDColorMode, NDColorModeMono);
        if (strcmp(fromServer_, "L") == 0)         setIntegerParam(NDDataType, NDUInt8);
        else if (strcmp(fromServer_, "I;16") == 0) setIntegerParam(NDDataType, NDUInt16);
        else if (strcmp(fromServer_, "I") == 0)    setIntegerParam(NDDataType, NDUInt32);
        else if (strcmp(fromServer_, "F") == 0)    setIntegerParam(NDDataType, NDFloat32);
        else if (strcmp(fromServer_, "RGB") == 0) {
            setIntegerParam(NDDataType, NDUInt8);
            setIntegerParam(NDColorMode, NDColorModeMono);
        }
        else asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                       "%s:%s: unknown mode=%s\n",
                       driverName, functionName, fromServer_);
    }
    if (validOptions_.count("TriggerMode")) {
        status = writeReadServer("GetTriggerMode");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart = strtok(fromServer_+2, "'");
            for (i=0, it=triggerModeChoices_.begin(); it!=triggerModeChoices_.end(); i++, it++) {
                if (*it == pStart) {
                    setIntegerParam(ADTriggerMode, i);
                    break;
                }
            }
            if (i == (int)triggerModeChoices_.size())
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                          "%s:%s: unknown trigger mode=%s\n",
                          driverName, functionName, pStart);
        }
    }
    if (validOptions_.count("Exposure")) {
        status = writeReadServer("GetExposure");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart=fromServer_+1;
            sscanf(pStart, "(%lf", &exposure);
            if (strstr(fromServer_, "Millisec")) exposure = exposure/1e3;
            else if (strstr(fromServer_, "Microsec")) exposure = exposure/1e6;
            setDoubleParam(ADAcquireTime, exposure);
        }
    }
    if (validOptions_.count("ChipGain")) {
        status = writeReadServer("GetChipGain");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart=fromServer_+1;
            sscanf(pStart, "%lf", &gain);
            setDoubleParam(ADGain, gain);
        }
    }
    setIntegerParam(ADBinX, 1);
    setIntegerParam(ADBinY, 1);
    if (validOptions_.count("Binning")) {
        status = writeReadServer("GetBinning");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart=fromServer_+1;
            sscanf(pStart, "(%d,%d)", &binX, &binY);
            setIntegerParam(ADBinX, binX);
            setIntegerParam(ADBinY, binY);
        }
    }
    if (validOptions_.count("SubArea")) {
        status = writeReadServer("GetSubArea");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart=fromServer_+1;
            sscanf(pStart, "(%d,%d,%d,%d)", &minX, &minY, &right, &bottom);
            setIntegerParam(ADMinX, minX);
            setIntegerParam(ADMinY, minY);
            sizeX = right - minX + 1;
            sizeY = bottom - minY + 1;
            setIntegerParam(ADSizeX, sizeX);
            setIntegerParam(ADSizeY, sizeY);
        }
    }
    if (validOptions_.count("Fliplr")) {
        status = writeReadServer("GetFliplr");
        if (status == asynSuccess) {
            sscanf(fromServer_, "%d", &reverseX);
            setIntegerParam(ADReverseX, reverseX);
        }
    }
    if (validOptions_.count("Flipud")) {
        status = writeReadServer("GetFlipud");
        if (status == asynSuccess) {
            sscanf(fromServer_, "%d", &reverseY);
            setIntegerParam(ADReverseY, reverseY);
        }
    }
    if (validOptions_.count("RecordPath")) {
        status = writeReadServer("GetRecordPath");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart = strtok(fromServer_+2, "'");
            setStringParam(NDFilePath, pStart);
        }
    }
    if (validOptions_.count("RecordName")) {
        status = writeReadServer("GetRecordName");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart = strtok(fromServer_+2, "'");
            setStringParam(NDFileName, pStart);
        }
    }
    if (validOptions_.count("RecordNumber")) {
        status = writeReadServer("GetRecordNumber");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart=fromServer_+1;
            sscanf(pStart, "%d", &fileNumber);
            setIntegerParam(NDFileNumber, fileNumber);
        }
    }
    if (validOptions_.count("RecordFormat")) {
        status = writeReadServer("GetRecordFormat");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart = strtok(fromServer_+2, "'");
            for (i=0, it=recordFormatChoices_.begin(); it!=recordFormatChoices_.end(); i++, it++) {
                if (*it == pStart) {
                    setIntegerParam(NDFileFormat, i);
                    break;
                }
            }
            if (i == (int)recordFormatChoices_.size()) {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                          "%s:%s: error, unknown file format string = %s\n",
                          driverName, functionName, pStart);
                //return asynError;
            }
        }
    }
    if (validOptions_.count("RecordTag")) {
        status = writeReadServer("GetRecordTag");
        if (status == asynSuccess) {
            pStart=fromServer_;            
            if (nCameras_ > 1)  pStart = strtok(fromServer_+2, "'");
            setStringParam(PSLTIFFComment_, pStart);
        }
    }

    callParamCallbacks();
    return asynSuccess;
}


asynStatus PSL::getImage()
{
    size_t dims[3];
    int nDims;
    int itemp1, itemp2;
    int imageCounter;
    int dataLen;
    int nCopied;
    int eomReason;
    int maxRead;
    int headerLen;
    int prefixLen=0;
    int sizeLen;
    NDDataType_t dataType=NDUInt8;
    NDArray *pImage=NULL;
    NDColorMode_t colorMode;
    epicsTimeStamp now;
    asynStatus status;
    NDArrayInfo arrayInfo;
    char *pOut=NULL;
    char *pIn;
    const char *functionName = "getImage";

    writeReadServer("GetImage", PSL_GET_IMAGE_TIMEOUT);
    nDims = 2;
    colorMode = NDColorModeMono;
    if (strncmp(fromServer_, "L;", 2) == 0) {
        prefixLen = 2;
        dataType = NDUInt8;
    } else if (strncmp(fromServer_, "I;16;", 5) == 0) {
        prefixLen = 5;
        dataType = NDUInt16;
    } else if (strncmp(fromServer_, "I;", 2) == 0) {
        prefixLen = 2;
        dataType = NDUInt32;
    } else if (strncmp(fromServer_, "F;", 2) == 0) {
        prefixLen = 2;
        dataType = NDFloat32;
    } else if (strncmp(fromServer_, "RGB;", 4) == 0) {
        prefixLen = 4;
        dataType = NDUInt8;
        nDims = 3;
        colorMode = NDColorModeRGB1;
    }
    setIntegerParam(NDDataType, dataType);
    sscanf(fromServer_+prefixLen, "%d;%d;%d;%n", &itemp1, &itemp2, &dataLen, &sizeLen);
    if (nDims == 2) {
        dims[0] = itemp1;
        dims[1] = itemp2;
        dims[2] = 1;
    } else {
        dims[0] = 3;
        dims[1] = itemp1;
        dims[2] = itemp2;
    }
    headerLen = prefixLen + sizeLen;
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s:%s: headerLen=%d, dims=[%d,%d,%d], dataLen=%d\n",
        driverName, functionName, headerLen, 
        (int)dims[0], (int)dims[1], (int)dims[2],
        (int)dataLen);
    if ((dims[0] <= 0) || (dims[1] <= 0)) return asynError;
    pImage = this->pNDArrayPool->alloc(nDims, dims, dataType, 0, NULL);
    pImage->getInfo(&arrayInfo);
    setIntegerParam(NDArraySizeX, arrayInfo.xSize);
    setIntegerParam(NDArraySizeY, arrayInfo.ySize);
    setIntegerParam(NDArraySize, arrayInfo.totalBytes);
    pIn = fromServer_ + headerLen;
    pOut = (char *)pImage->pData;
    nRead_ -= headerLen;
    for (nCopied=0; nCopied<dataLen; nCopied+=(int)nRead_) {
        if (nCopied > 0) {
            maxRead = sizeof(fromServer_);
            if (maxRead > (dataLen - nCopied)) maxRead = dataLen - nCopied;
            status = pasynOctetSyncIO->read(pasynUserServer_, fromServer_, maxRead, 1,
                                            &nRead_, &eomReason);
            if (status != asynSuccess) {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                    "%s:%s: error reading image, status=%d, dataLen=%d, nCopied=%d, maxRead=%d, nRead=%d\n",
                    driverName, functionName, status, dataLen, nCopied, 
                    (int)maxRead, (int)nRead_);
                break;
            }
            pIn = fromServer_;
        }
        memcpy(pOut, pIn, nRead_);
        pOut+= nRead_;
    }
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s:%s: end of image readout\n",
        driverName, functionName);

    getIntegerParam(NDArrayCounter, &imageCounter);

    /* Put the frame number and time stamp into the NDArray */
    epicsTimeGetCurrent(&now);
    pImage->uniqueId = imageCounter;
    pImage->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
    updateTimeStamp(&pImage->epicsTS);

    pImage->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);

    /* Get any attributes that have been defined for this driver */        
    this->getAttributes(pImage->pAttributeList);

    /* Call the NDArray callback */
    /* Must release the lock here, or we can get into a deadlock, because we can
     * block on the plugin lock, and the plugin can be calling us */
    this->unlock();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
         "%s:%s: calling NDArray callback\n", driverName, functionName);
    doCallbacksGenericPointer(pImage, NDArrayData, 0);
    this->lock();

    /* Free the image buffer */
    pImage->release();
    return asynSuccess;
}

asynStatus PSL::startAcquire()
{
    int imageMode;
    int numImages=1;

    getIntegerParam(ADImageMode, &imageMode);
    switch (imageMode) {
        case ADImageSingle:
            writeReadServer("SetFrameNumber;1");
            writeReadServer("Snap");
            break;
            
        case ADImageMultiple:
            getIntegerParam(ADNumImages, &numImages);
            epicsSnprintf(toServer_, sizeof(toServer_), "SetFrameNumber;%d", numImages);
            writeReadServer(toServer_);
            writeReadServer("Snap");
            break;
            
        case ADImageContinuous:
            writeReadServer("Start");
            break;
    }
    epicsEventSignal(startEventId_);
    return asynSuccess;
}    

/** This thread controls acquisition, reads TIFF files to get the image data, and
 * does the callbacks to send it to higher layers */
void PSL::PSLTask()
{
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int acquire;
    int autoSave;
    int arrayCallbacks;
    int shutterMode;
    const char *functionName = "PSLTask";

    this->lock();

    /* Loop forever */
    while (1) {
        /* Is acquisition active? */
        getIntegerParam(ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
            setStringParam(ADStatusMessage, "Waiting for acquire command");
            callParamCallbacks();
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            this->unlock();
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            epicsEventWait(startEventId_);
            this->lock();
            getIntegerParam(ADAcquire, &acquire);
            setIntegerParam(ADNumImagesCounter, 0);
            callParamCallbacks();
        }
        
        /* Get current values of some parameters */
        getIntegerParam(NDAutoSave, &autoSave);
        getIntegerParam(ADShutterMode, &shutterMode);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        

        /* If arrayCallbacks is set then read the data, do callbacks */
        if (arrayCallbacks) {
            while (1) {
                writeReadServer("HasNewData");
                if (strcmp(fromServer_, "True") == 0) break;
                if (epicsEventTryWait(stopEventId_) == epicsEventWaitOK) break;
                unlock();
                epicsThreadSleep(epicsThreadSleepQuantum());
                lock();
            }
            getImage();
        }

        getIntegerParam(NDArrayCounter, &imageCounter);
        imageCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        getIntegerParam(ADNumImagesCounter, &numImagesCounter);
        numImagesCounter++;
        setIntegerParam(ADNumImagesCounter, numImagesCounter);

        getIntegerParam(ADImageMode, &imageMode);
        if (imageMode == ADImageMultiple) {
            getIntegerParam(ADNumImages, &numImages);
            if (numImagesCounter >= numImages) setIntegerParam(ADAcquire, 0);
        }    
        if (imageMode == ADImageSingle) setIntegerParam(ADAcquire, 0);

        /* Call the callbacks to update any changes */
        callParamCallbacks();
    }
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADBinX, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus PSL::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int binX, binY, minX, minY, sizeX, sizeY;
    asynStatus status = asynSuccess;
    int acquiring;
    const char *functionName = "writeInt32";

    /* Get the current acquire status */
    getIntegerParam(ADAcquire, &acquiring);
    status = setIntegerParam(function, value);

    if (function == ADAcquire) {
        if (value && !acquiring) {
            startAcquire();
        } 
        if (!value && acquiring) {
            /* This was a command to stop acquisition */
            status = writeReadServer("Stop");
            epicsEventSignal(stopEventId_);
        }
    } else if ((function == ADBinX) ||
               (function == ADBinY)) {
        /* Set binning */
        getIntegerParam(ADBinX, &binX);
        getIntegerParam(ADBinY, &binY);
        epicsSnprintf(toServer_, sizeof(toServer_), "SetBinning;(%d,%d)", binX, binY);
        writeReadServer(toServer_);
    } else if ((function == ADMinX) ||
               (function == ADMinY) ||
               (function == ADSizeX) ||
               (function == ADSizeY)) {
        getIntegerParam(ADMinX, &minX);
        getIntegerParam(ADMinY, &minY);
        getIntegerParam(ADSizeX, &sizeX);
        getIntegerParam(ADSizeY, &sizeY);
//        minX /= (nCameras_/2); minY /= (nCameras_/2);
 //       sizeX /= (nCameras_/2); sizeY /= (nCameras_/2);
        epicsSnprintf(toServer_, sizeof(toServer_),
                      "SetSubArea;(%d,%d,%d,%d)",
                      minX, minY, minX+sizeX-1, minY+sizeY-1);
        status = writeReadServer(toServer_);
    } else if (function == ADReverseX) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetFliplr;%d", value);
        writeReadServer(toServer_);
    } else if (function == ADReverseY) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetFlipud;%d", value);
        writeReadServer(toServer_);
    } else if (function == ADTriggerMode) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetTriggerMode;%s", 
            getChoiceFromIndex(triggerModeChoices_, value).c_str());
        writeReadServer(toServer_);
    } else if (function == NDAutoSave) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetAutoSave;%d", value);
        status = writeReadServer(toServer_);
    } else if (function == NDFileFormat) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetRecordFormat;%s",
                      getChoiceFromIndex(recordFormatChoices_, value).c_str());
        status = writeReadServer(toServer_);
    } else if (function == PSLCameraName_) {
        status = openCamera(value);
    } else if (function == NDFileNumber) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetRecordNumber;%d", value);
        status = writeReadServer(toServer_);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_PSL_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }
        
    getConfig();
    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    return status;
}

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus PSL::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeFloat64";

    status = setDoubleParam(function, value);

    if (function == ADAcquireTime) {
        if (value < 0.01) {
            epicsSnprintf(toServer_, sizeof(toServer_),
                          "SetExposure;(%d,'Microsec')",
                          (int)(value*1e6 + 0.5));
        }
        else {
            epicsSnprintf(toServer_, sizeof(toServer_),
                          "SetExpoMS;%d", (int)(value*1e3 + 0.5));
        }
        status = writeReadServer(toServer_);
    } else if (function == ADAcquirePeriod) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetFrameTime;%f", value*1000);
        status = writeReadServer(toServer_);
    } else if (function == ADGain) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetChipGain;%f", value);
        status = writeReadServer(toServer_);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_PSL_PARAM) status = ADDriver::writeFloat64(pasynUser, value);
    }
        
    getConfig();
    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    return status;
}

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including PilatusBadPixelFile, ADFilePath, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus PSL::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(function, (char *)value);
    // Need to find out if these commands are still supported in the server
    if (function == NDFilePath) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetRecordPath;%s", value);
        status = writeReadServer(toServer_);
        checkPath();
    } else if (function == NDFileName) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetRecordName;%s", value);
        status = writeReadServer(toServer_);
    } else if (function == PSLTIFFComment_) {
        epicsSnprintf(toServer_, sizeof(toServer_), "SetRecordTag;%s", value);
        status = writeReadServer(toServer_);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_PSL_PARAM) status = ADDriver::writeOctet(pasynUser, value, nChars, nActual);
    }
    getConfig();
     /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    return status;
}


asynStatus PSL::readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                         size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    int i;
    choice_t choices;
    choice_t::iterator it;

    if (function == PSLCameraName_) {
        choices = cameraNameChoices_;
    } else if (function == ADTriggerMode) {
        choices = triggerModeChoices_;
    } else if (function == NDFileFormat) {
        choices = recordFormatChoices_;
    } else {
        *nIn = 0;
        return asynError;
    }

    for (i=0, it=choices.begin(); it!=choices.end(); i++, it++) {
        if (strings[i]) free(strings[i]);
        strings[i] = epicsStrDup((*it).c_str());
        values[i] = i;
        severities[i] = 0;
    }
    *nIn = i;
    return asynSuccess;   
}

asynStatus PSL::doEnumCallbacks()
{
    int function, i;
    int functions[] = {NDFileFormat, ADTriggerMode, PSLCameraName_};
    choice_t choices[] = {recordFormatChoices_, triggerModeChoices_, cameraNameChoices_};
    choice_t::iterator it;
    char *strings[MAX_CHOICES];
    int values[MAX_CHOICES];
    int severities[MAX_CHOICES];   
    int numFunctions = sizeof(functions)/sizeof(functions[0]);

    for (function=0; function<numFunctions; function++) {
        for (i=0, it=choices[function].begin(); it!=choices[function].end() && i<MAX_CHOICES; i++, it++) {
            strings[i] = epicsStrDup((*it).c_str());
            values[i] = i;
            severities[i] = 0;
        }
        doCallbacksEnum(strings, values, severities, i, functions[i], 0);
    }
    return asynSuccess;   
}



/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void PSL::report(FILE *fp, int details)
{
    fprintf(fp, "PSL detector %s\n", this->portName);
    if (details > 0) {
        choice_t::iterator it;
        
        fprintf(fp, "  Server version:    %f\n", serverVersion_);
        fprintf(fp, "  Sub-cameras:       %d\n", nCameras_);
        fprintf(fp, "  Cameras (%d):\n", (int)cameraNameChoices_.size());
        for (it=cameraNameChoices_.begin(); it!= cameraNameChoices_.end(); it++) {
            fprintf(fp, "    %s\n", (*it).c_str());
        }
        fprintf(fp, "  Options (%d):\n", (int)validOptions_.size());
        for (it=validOptions_.begin(); it!= validOptions_.end(); it++) {
            fprintf(fp, "    %s\n", (*it).c_str());
        }
        fprintf(fp, "  Trigger modes (%d):\n", (int)triggerModeChoices_.size());
        for (it=triggerModeChoices_.begin(); it!= triggerModeChoices_.end(); it++) {
            fprintf(fp, "    %s\n", (*it).c_str());
        }
        fprintf(fp, "  Record formats (%d):\n", (int)recordFormatChoices_.size());
        for (it=recordFormatChoices_.begin(); it!= recordFormatChoices_.end(); it++) {
            fprintf(fp, "    %s\n", (*it).c_str());
        }
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}

extern "C" int PSLConfig(const char *portName, const char *serverPort, 
                            int maxBuffers, size_t maxMemory,
                            int priority, int stackSize)
{
    new PSL(portName, serverPort, maxBuffers, maxMemory, priority, stackSize);
    return asynSuccess;
}


/* Code for iocsh registration */
static const iocshArg PSLConfigArg0 = {"Port name", iocshArgString};
static const iocshArg PSLConfigArg1 = {"server port name", iocshArgString};
static const iocshArg PSLConfigArg2 = {"maxBuffers", iocshArgInt};
static const iocshArg PSLConfigArg3 = {"maxMemory", iocshArgInt};
static const iocshArg PSLConfigArg4 = {"priority", iocshArgInt};
static const iocshArg PSLConfigArg5 = {"stackSize", iocshArgInt};
static const iocshArg * const PSLConfigArgs[] =  {&PSLConfigArg0,
                                                     &PSLConfigArg1,
                                                     &PSLConfigArg2,
                                                     &PSLConfigArg3,
                                                     &PSLConfigArg4,
                                                     &PSLConfigArg5};
static const iocshFuncDef configPSL = {"PSLConfig", 6, PSLConfigArgs};
static void configPSLCallFunc(const iocshArgBuf *args)
{
    PSLConfig(args[0].sval, args[1].sval, args[2].ival,
                 args[3].ival, args[4].ival, args[5].ival);
}


static void PSLRegister(void)
{
    iocshRegister(&configPSL, configPSLCallFunc);
}

extern "C" {
epicsExportRegistrar(PSLRegister);
}
