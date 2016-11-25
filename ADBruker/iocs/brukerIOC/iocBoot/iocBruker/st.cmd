< envPaths
errlogInit(20000)

dbLoadDatabase("$(TOP)/dbd/BISDetectorApp.dbd")
BISDetectorApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "BIS:")
# The port name for the detector
epicsEnvSet("PORT",   "APX")
epicsEnvSet("COMMAND_PORT", "BIS_COMMAND")
epicsEnvSet("FILE_PORT",    "BIS_FILE")
epicsEnvSet("STATUS_PORT",  "BIS_STATUS")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "512")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "512")
# The maximum number of time seried points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

###
# Create the asyn port to talk to the BIS server on command port 49153.
drvAsynIPPortConfigure($(COMMAND_PORT),"chemmat21:49153")
asynSetTraceIOMask($(COMMAND_PORT),0,2)
#asynSetTraceMask($(COMMAND_PORT),0,255)

drvAsynIPPortConfigure($(FILE_PORT),"chemmat21:49154")
asynSetTraceIOMask($(FILE_PORT),0,2)
#asynSetTraceMask($(FILE_PORT),0,255)

drvAsynIPPortConfigure($(STATUS_PORT),"chemmat21:49155")
asynSetTraceIOMask($(STATUS_PORT),0,2)
asynSetTraceIOTruncateSize($(STATUS_PORT),0,512)
#asynSetTraceMask($(STATUS_PORT),0,255)

# Set the terminators
asynOctetSetOutputEos($(COMMAND_PORT), 0, "\n")
# Note: The BIS documentation says that the responses on the COMMAND port are terminated with \n, but they are not in our BIS version.
# The do all end with ], so use that.
asynOctetSetInputEos($(COMMAND_PORT), 0, "]")
asynOctetSetInputEos($(STATUS_PORT), 0, "\n")

BISDetectorConfig($(PORT), $(COMMAND_PORT), $(STATUS_PORT), 0, 0)
dbLoadRecords("$(ADBRUKER/db/BIS.template",    "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1,BIS_PORT=$(COMMAND_PORT)")
asynSetTraceIOMask($(PORT),0,2)
#asynSetTraceMask($(PORT),0,255)


# Create a standard arrays plugin
NDStdArraysConfigure("apxImage", 5, 0, $(PORT), 0, 0)
# This creates a waveform large enough to hold a 4096x4096 image
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=apxImage,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int32,FTVL=LONG,NELEMENTS=16777216")

# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
set_requestfile_path("$(ADBRUKER)/brukerApp/Db")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30,"P=$(PREFIX)")

