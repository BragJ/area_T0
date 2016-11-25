
/**
 * @brief areaDetector driver that is a V4 neutron data client for nED.
 *
 * @author Matt Pearson
 * @date Sept 2014
 */

#ifndef ADNED_H
#define ADNED_H

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
// nexus 
#include <napi.h>
#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <cantProceed.h>
#include <epicsTypes.h>
#include <vector>


#include <asynOctetSyncIO.h>

#include <pv/pvTimeStamp.h>
#include "ADDriver.h"
#include "nEDChannel.h"
#include "ADnEDTransform.h"
#include "ADnEDGlobals.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
#define ADnEDFirstParamString              "ADNED_FIRST"
#define ADnEDLastParamString               "ADNED_LAST"
#define ADnEDResetParamString              "ADNED_RESET"
#define ADnEDStartParamString              "ADNED_START"
#define ADnEDStopParamString               "ADNED_STOP"
#define ADnEDPauseParamString              "ADNED_PAUSE"
#define ADnEDEventDebugParamString         "ADNED_EVENT_DEBUG"
#define ADnEDSeqCounterParamString         "ADNED_SEQ_COUNTER"
#define ADnEDPulseCounterParamString       "ADNED_PULSE_COUNTER"
#define ADnEDEventRateParamString          "ADNED_EVENT_RATE"
#define ADnEDSeqIDParamString              "ADNED_SEQ_ID"
#define ADnEDSeqIDMissingParamString       "ADNED_SEQ_ID_MISSING"
#define ADnEDSeqIDNumMissingParamString    "ADNED_SEQ_ID_NUM_MISSING"
#define ADnEDBadTimeStampParamString       "ADNED_BAD_TIMESTAMP"
#define ADnEDPChargeParamString            "ADNED_PCHARGE"
#define ADnEDPChargeIntParamString         "ADNED_PCHARGE_INT"
#define ADnEDEventUpdatePeriodParamString  "ADNED_EVENT_UPDATE_PERIOD"
#define ADnEDFrameUpdatePeriodParamString  "ADNED_FRAME_UPDATE_PERIOD"
#define ADnEDNumChannelsParamString        "ADNED_NUM_CHANNELS"
#define ADnEDPVNameParamString             "ADNED_PV_NAME"
#define ADnEDNumDetParamString             "ADNED_NUM_DET"
#define ADnEDDetPixelNumStartParamString   "ADNED_DET_PIXEL_NUM_START"
#define ADnEDDetPixelNumEndParamString     "ADNED_DET_PIXEL_NUM_END"
#define ADnEDDetPixelNumSizeParamString    "ADNED_DET_PIXEL_NUM_SIZE"
#define ADnEDDetTOFNumBinsParamString      "ADNED_DET_TOF_NUM_BINS"
#define ADnEDDet2DTypeParamString          "ADNED_DET_2D_TYPE"
#define ADnEDDetNDArrayStartParamString    "ADNED_DET_NDARRAY_START"
#define ADnEDDetNDArrayEndParamString      "ADNED_DET_NDARRAY_END"
#define ADnEDDetNDArraySizeParamString     "ADNED_DET_NDARRAY_SIZE"
#define ADnEDDetNDArrayTOFStartParamString "ADNED_DET_NDARRAY_TOF_START"
#define ADnEDDetNDArrayTOFEndParamString   "ADNED_DET_NDARRAY_TOF_END"
#define ADnEDDetEventRateParamString       "ADNED_DET_EVENT_RATE"
#define ADnEDDetEventTotalParamString      "ADNED_DET_EVENT_TOTAL"
#define ADnEDDetTOFROIStartParamString     "ADNED_DET_TOF_ROI_START"
#define ADnEDDetTOFROISizeParamString      "ADNED_DET_TOF_ROI_SIZE"
#define ADnEDDetTOFROIEnableParamString    "ADNED_DET_TOF_ROI_ENABLE"
#define ADnEDDetTOFArrayResetParamString   "ADNED_DET_TOF_ARRAY_RESET"
//Params to use with ADnEDTransform
#define ADnEDDetTOFTransFile0ParamString   "ADNED_DET_TOF_TRANS_FILE0"
#define ADnEDDetTOFTransFile1ParamString   "ADNED_DET_TOF_TRANS_FILE1"
#define ADnEDDetTOFTransFile2ParamString   "ADNED_DET_TOF_TRANS_FILE2"
#define ADnEDDetTOFTransFile3ParamString   "ADNED_DET_TOF_TRANS_FILE3"
#define ADnEDDetTOFTransFile4ParamString   "ADNED_DET_TOF_TRANS_FILE4"
#define ADnEDDetTOFTransFile5ParamString   "ADNED_DET_TOF_TRANS_FILE5"
#define ADnEDDetTOFTransInt0ParamString    "ADNED_DET_TOF_TRANS_INT0"
#define ADnEDDetTOFTransInt1ParamString    "ADNED_DET_TOF_TRANS_INT1"
#define ADnEDDetTOFTransInt2ParamString    "ADNED_DET_TOF_TRANS_INT2"
#define ADnEDDetTOFTransInt3ParamString    "ADNED_DET_TOF_TRANS_INT3"
#define ADnEDDetTOFTransInt4ParamString    "ADNED_DET_TOF_TRANS_INT4"
#define ADnEDDetTOFTransInt5ParamString    "ADNED_DET_TOF_TRANS_INT5"
#define ADnEDDetTOFTransFloat0ParamString  "ADNED_DET_TOF_TRANS_FLOAT0"
#define ADnEDDetTOFTransFloat1ParamString  "ADNED_DET_TOF_TRANS_FLOAT1"
#define ADnEDDetTOFTransFloat2ParamString  "ADNED_DET_TOF_TRANS_FLOAT2"
#define ADnEDDetTOFTransFloat3ParamString  "ADNED_DET_TOF_TRANS_FLOAT3"
#define ADnEDDetTOFTransFloat4ParamString  "ADNED_DET_TOF_TRANS_FLOAT4"
#define ADnEDDetTOFTransFloat5ParamString  "ADNED_DET_TOF_TRANS_FLOAT5"
#define ADnEDDetTOFTransPrintParamString   "ADNED_DET_TOF_TRANS_PRINT"
#define ADnEDDetTOFTransDebugParamString   "ADNED_DET_TOF_TRANS_DEBUG"
#define ADnEDDetTOFTransTypeParamString    "ADNED_DET_TOF_TRANS_TYPE"
#define ADnEDDetTOFTransOffsetParamString  "ADNED_DET_TOF_TRANS_OFFSET"
#define ADnEDDetTOFTransScaleParamString   "ADNED_DET_TOF_TRANS_SCALE"
//
#define ADnEDDetPixelMapFileParamString    "ADNED_DET_PIXEL_MAP_FILE"
#define ADnEDDetPixelMapPrintParamString   "ADNED_DET_PIXEL_MAP_PRINT"
#define ADnEDDetPixelMapEnableParamString  "ADNED_DET_PIXEL_MAP_ENABLE"
#define ADnEDDetPixelROIStartXParamString  "ADNED_DET_PIXEL_ROI_START_X"
#define ADnEDDetPixelROISizeXParamString   "ADNED_DET_PIXEL_ROI_SIZE_X"
#define ADnEDDetPixelROIStartYParamString  "ADNED_DET_PIXEL_ROI_START_Y"
#define ADnEDDetPixelROISizeYParamString   "ADNED_DET_PIXEL_ROI_SIZE_Y"
#define ADnEDDetPixelSizeXParamString      "ADNED_DET_PIXEL_SIZE_X"
#define ADnEDDetPixelROIEnableParamString  "ADNED_DET_PIXEL_ROI_ENABLE"
#define ADnEDTOFMaxParamString             "ADNED_TOF_MAX"
#define ADnEDAllocSpaceParamString         "ADNED_ALLOC_SPACE"
#define ADnEDAllocSpaceStatusParamString   "ADNED_ALLOC_SPACE_STATUS"
#define ADnEDAllocSpaceStatusParamString   "ADNED_ALLOC_SPACE_STATUS"
// mine
#define ADnEDHdfFullFileNameParamString  "ADNED_HDF_FULL_FILE_NAME"
#define ADnEDHdfWriteModeParamString  "ADNED_HDF_WRITE_MODE"
#define ADnEDHdfFileTemplateParamString  "ADNED_HDF_FILE_TEMPLATE"

#define ADnEDSingleHdfFileNumberParamString  "ADNED_SINGLE_HDF_FILE_NUMBER"
#define ADnEDCaptureHdfFileNumberParamString  "ADNED_CAPTURE_HDF_FILE_NUMBER"

#define ADnEDHdfFilePathParamString  "ADNED_HDF_FILE_PATH"

#define ADnEDHdfPauseParamString "ADNED_HDF_PAUSE"
#define ADnEDHdfStatusMessageParamString "ADNED_HDF_STATUS_MESSAGE"

#define ADnEDHdfHV1MessageParamString "ADNED_HDF_HV1_MESSAGE"
#define ADnEDHdfHV2MessageParamString "ADNED_HDF_HV2_MESSAGE"
#define ADnEDHdfGasContentMessageParamString "ADNED_HDF_GAS_CONTENT_MESSAGE"

#define ADnEDHdfNumPulsePerFileParamString "ADNED_HDF_NUM_PULSE_PER_FILE"





extern "C" {
  asynStatus ADnEDConfig(const char *portName, int maxBuffers, size_t maxMemory, int debug);
  asynStatus ADnEDCreateFactory();
}

namespace epics {
  namespace pvData {
    class PVStructure;
  }
}

class ADnED : public ADDriver {

 public:
  ADnED(const char *portName, int maxBuffers, size_t maxMemory, int debug);
  virtual ~ADnED();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, 
                                  size_t nChars, size_t *nActual);
  virtual void report(FILE *fp, int details);

  static asynStatus createFactory();
  //void saveHDF5StreamMode(epics::pvData::uint32 pixelsize,size_t single_pixel_data[pixelsize],size_t single_tof_data[pixelsize]);


  //   ######### mine ############     \\
  #define MAX_FILENAME_LEN 256
  // file num
  int single_file_num;
  int capture_file_num,capture_group_num;
  int stream_group_num;
  int pulse_num_per_file;
  int time_per_file;
  
  //file or group name
  char single_hdf_file_name[MAX_FILENAME_LEN];
  char capture_group_name[MAX_FILENAME_LEN];
  char stream_group_name[MAX_FILENAME_LEN];
  char capture_pixel_name[MAX_FILENAME_LEN],capture_tof_name[MAX_FILENAME_LEN];
  char pixel_name[MAX_FILENAME_LEN],tof_name[MAX_FILENAME_LEN];
  

  // new structure para
  int start_every_slap;  
  int slab_start[1], slab_size[1]; 
  int unlimited_dims[1]={NX_UNLIMITED};

  //int unlimited_dims[1];

  // nexus file write param
  int pulse_id_dim; 
  NXhandle file_id;
  NXhandle capture_file_id;
  NXhandle stream_file_id;
  NXstatus nxstat;
  
  // PV param
  int common_aChar_len;
  char hdfFullFileName[MAX_FILENAME_LEN];
  char hdfFileTemplate[MAX_FILENAME_LEN];
  int hdfFileWriteMode;
  char hdfFilePath[MAX_FILENAME_LEN];

  // meta data
  char hdfHV1Message[MAX_FILENAME_LEN];
  char hdfHV2Message[MAX_FILENAME_LEN];
  char hdfGasContentMessage[MAX_FILENAME_LEN];

  size_t m_threeAttribute[3]; 
 // int three_len = 3;


  // storage flag
  int storageFlag;


  void eventTask(void);
  void frameTask(void);
  void eventHandler(std::tr1::shared_ptr<epics::pvData::PVStructure> const &pv_struct, epicsUInt32 channelID);
  asynStatus allocArray(void); 
  asynStatus clearParams(void);


 private:

  //Put private functions here
  void printPixelMap(epicsUInt32 det);
  void print_data (const char *prefix, void *data, int type, int num);
  void printTofTrans(epicsUInt32 det);
  asynStatus checkPixelMap(epicsUInt32 det);
  asynStatus setupChannelMonitor(const char *pvName, int channel);
  bool matchTransFile(const int asynParam, epicsUInt32 &transIndex);
  bool matchTransInt(const int asynParam, epicsUInt32 &transIndex);
  bool matchTransFloat(const int asynParam, epicsUInt32 &transIndex);
  void resetTOFArray(epicsUInt32 det);
 
  //Put private static data members here
  static const epicsInt32 s_ADNED_MAX_STRING_SIZE;
  static const epicsInt32 s_ADNED_MAX_DETS;
  static const epicsInt32 s_ADNED_MAX_CHANNELS;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_OK;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_REQ;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_FAIL;
  static const epicsUInt32 s_ADNED_2D_PLOT_XY;
  static const epicsUInt32 s_ADNED_2D_PLOT_XTOF;
  static const epicsUInt32 s_ADNED_2D_PLOT_YTOF;
  static const epicsUInt32 s_ADNED_2D_PLOT_PIXELIDTOF;

  //Put private dynamic here
  epicsUInt32 m_acquiring; 
  epicsUInt32 m_seqCounter[ADNED_MAX_CHANNELS];
  epicsUInt32 m_seqID[ADNED_MAX_CHANNELS];
  epicsUInt32 m_lastSeqID[ADNED_MAX_CHANNELS];
  epicsUInt32 m_pulseCounter;
  epicsFloat64 m_pChargeInt;
  epicsTimeStamp m_nowTime;
  double m_nowTimeSecs;
  double m_lastTimeSecs;
  epicsUInt32 *p_Data;
  epicsUInt32 *p_PixelMap[ADNED_MAX_DETS+1];
  epicsUInt32 m_PixelMapSize[ADNED_MAX_DETS+1];
  bool m_dataAlloc;
  epicsUInt32 m_dataMaxSize;
  epicsUInt32 m_bufferMaxSize;
  epicsUInt32 m_tofMax;
  epics::pvData::PVTimeStamp m_PVTimeStamp; 
  epics::pvData::TimeStamp m_TimeStamp[ADNED_MAX_CHANNELS];
  epics::pvData::TimeStamp m_TimeStampLast[ADNED_MAX_CHANNELS];
  int m_detStartValues[ADNED_MAX_DETS+1];
  int m_detEndValues[ADNED_MAX_DETS+1];
  int m_detSizeValues[ADNED_MAX_DETS+1];
  int m_NDArrayStartValues[ADNED_MAX_DETS+1];
  int m_NDArrayTOFStartValues[ADNED_MAX_DETS+1];
  int m_detTOFROIStartValues[ADNED_MAX_DETS+1];
  int m_detTOFROISizeValues[ADNED_MAX_DETS+1];
  int m_detTOFROIEnabled[ADNED_MAX_DETS+1];
  int m_detPixelMappingEnabled[ADNED_MAX_DETS+1];
  int m_detTOFTransType[ADNED_MAX_DETS+1];
  double m_detTOFTransScale[ADNED_MAX_DETS+1];
  double m_detTOFTransOffset[ADNED_MAX_DETS+1];
  int m_detPixelROIStartX[ADNED_MAX_DETS+1];
  int m_detPixelROIStartY[ADNED_MAX_DETS+1];
  int m_detPixelROISizeX[ADNED_MAX_DETS+1];
  int m_detPixelROISizeY[ADNED_MAX_DETS+1];
  int m_detPixelSizeX[ADNED_MAX_DETS+1];
  int m_detPixelROIEnable[ADNED_MAX_DETS+1];
  epicsUInt32 m_eventsSinceLastUpdate;
  epicsUInt32 m_detEventsSinceLastUpdate[ADNED_MAX_DETS+1];
  epicsFloat64 m_detTotalEvents[ADNED_MAX_DETS+1];

  epics::pvAccess::ChannelProvider::shared_pointer p_ChannelProvider;
  std::tr1::shared_ptr<nEDChannel::nEDChannelRequester> p_ChannelRequester;
  std::tr1::shared_ptr<nEDChannel::nEDMonitorRequester> p_MonitorRequester[ADNED_MAX_CHANNELS];
  epics::pvData::Monitor::shared_pointer p_Monitor[ADNED_MAX_CHANNELS];
  epics::pvAccess::Channel::shared_pointer p_Channel[ADNED_MAX_CHANNELS];

  ADnEDTransform *p_Transform[ADNED_MAX_DETS+1];

  //Constructor parameters.
  const epicsUInt32 m_debug;

  epicsEventId m_startEvent;
  epicsEventId m_stopEvent;
  epicsEventId m_startFrame;
  epicsEventId m_stopFrame;
  
  //Values used for pasynUser->reason, and indexes into the parameter library.
  int ADnEDFirstParam;
  #define ADNED_FIRST_DRIVER_COMMAND ADnEDFirstParam
  int ADnEDResetParam;
  int ADnEDStartParam;
  int ADnEDStopParam;
  int ADnEDPauseParam;
  int ADnEDEventDebugParam;
  int ADnEDSeqCounterParam;
  int ADnEDPulseCounterParam;
  int ADnEDEventRateParam;
  int ADnEDSeqIDParam;
  int ADnEDSeqIDMissingParam;
  int ADnEDSeqIDNumMissingParam;
  int ADnEDBadTimeStampParam;
  int ADnEDPChargeParam;
  int ADnEDPChargeIntParam;
  int ADnEDEventUpdatePeriodParam;
  int ADnEDFrameUpdatePeriodParam;
  int ADnEDNumChannelsParam;
  int ADnEDPVNameParam;
  int ADnEDNumDetParam;
  int ADnEDDetPixelNumStartParam;
  int ADnEDDetPixelNumEndParam;
  int ADnEDDetPixelNumSizeParam;
  int ADnEDDetTOFNumBinsParam;
  int ADnEDDet2DTypeParam;
  int ADnEDDetNDArrayStartParam;
  int ADnEDDetNDArrayEndParam;
  int ADnEDDetNDArraySizeParam;
  int ADnEDDetNDArrayTOFStartParam;
  int ADnEDDetNDArrayTOFEndParam;
  int ADnEDDetEventRateParam;
  int ADnEDDetEventTotalParam;
  int ADnEDDetTOFROIStartParam;
  int ADnEDDetTOFROISizeParam;
  int ADnEDDetTOFROIEnableParam;
  int ADnEDDetTOFArrayResetParam;
  //Params to use with ADnEDTransform
  int ADnEDDetTOFTransFile0Param;
  int ADnEDDetTOFTransFile1Param;
  int ADnEDDetTOFTransFile2Param;
  int ADnEDDetTOFTransFile3Param;
  int ADnEDDetTOFTransFile4Param;
  int ADnEDDetTOFTransFile5Param;
  int ADnEDDetTOFTransInt0Param;
  int ADnEDDetTOFTransInt1Param;
  int ADnEDDetTOFTransInt2Param;
  int ADnEDDetTOFTransInt3Param;
  int ADnEDDetTOFTransInt4Param;
  int ADnEDDetTOFTransInt5Param;
  int ADnEDDetTOFTransFloat0Param;
  int ADnEDDetTOFTransFloat1Param;
  int ADnEDDetTOFTransFloat2Param;
  int ADnEDDetTOFTransFloat3Param;
  int ADnEDDetTOFTransFloat4Param;
  int ADnEDDetTOFTransFloat5Param;
  int ADnEDDetTOFTransPrintParam;
  int ADnEDDetTOFTransDebugParam;
  int ADnEDDetTOFTransTypeParam;
  int ADnEDDetTOFTransOffsetParam;
  int ADnEDDetTOFTransScaleParam;
  //
  int ADnEDDetPixelMapFileParam;
  int ADnEDDetPixelMapPrintParam;
  int ADnEDDetPixelMapEnableParam;
  int ADnEDDetPixelROIStartXParam;
  int ADnEDDetPixelROISizeXParam;
  int ADnEDDetPixelROIStartYParam;
  int ADnEDDetPixelROISizeYParam;
  int ADnEDDetPixelSizeXParam;
  int ADnEDDetPixelROIEnableParam;
  int ADnEDTOFMaxParam;
  int ADnEDAllocSpaceParam;
  int ADnEDAllocSpaceStatusParam;

  // mine 
  int ADnEDHdfFullFileNameParam;
  int ADnEDHdfWriteModeParam;
  int ADnEDHdfFileTemplateParam;

  int ADnEDSingleHdfFileNumberParam;
  int ADnEDCaptureHdfFileNumberParam;
  
  int ADnEDHdfFilePathParam;
  int ADnEDHdfPauseParam;
  int ADnEDHdfStatusMessageParam;

  int ADnEDHdfHV1MessageParam;
  int ADnEDHdfHV2MessageParam;
  int ADnEDHdfGasContentMessageParam;
  int ADnEDHdfNumPulsePerFileParam;


  int ADnEDLastParam;
  
 
  #define ADNED_LAST_DRIVER_COMMAND ADnEDLastParam

};

#define NUM_DRIVER_PARAMS (&ADNED_LAST_DRIVER_COMMAND - &ADNED_FIRST_DRIVER_COMMAND + 1)

#endif //ADNED_H
