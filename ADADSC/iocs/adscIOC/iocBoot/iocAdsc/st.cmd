< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/adscApp.dbd")
adscApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "13ADCS1:")
# The port name for the detector
epicsEnvSet("PORT",   "ADSC1")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "2048")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "2048")
# The maximum number of time seried points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

#
# adscConfig(const char *portName, const char *modelName)
#   portName   asyn port name
#   modelName  ADSC detector model name; must be one of "Q4", "Q4r", "Q210",
#              "Q210r", "Q270", "Q315", or "Q315r"
#
adscConfig("$(PORT)","Q210r")
dbLoadRecords("$(ADADSC)/db/adsc.template",  "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

#asynSetTraceMask("$(PORT)",0,255)

set_requestfile_path("./")
set_savefile_path("./autosave")
set_requestfile_path("$(ADCORE)/ADApp/Db")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
save_restoreSet_status_prefix("$(PREFIX)")
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db","P=$(PREFIX)")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req",30,"P=$(PREFIX),R=cam1:")
