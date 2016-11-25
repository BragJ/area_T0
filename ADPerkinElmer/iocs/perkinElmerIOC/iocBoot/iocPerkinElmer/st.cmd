errlogInit(20000)

< envPaths

dbLoadDatabase("$(TOP)/dbd/PerkinElmerApp.dbd")
PerkinElmerApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "13PE1:")
# The port name for the detector
epicsEnvSet("PORT",   "PEDET1")
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

# Create a PerkinElmer driver
# PerkinElmerConfig(const char *portName, IDType, IDValue, maxBuffers, size_t maxMemory, int priority, int stackSize)
# IDType = 0 Frame grabber card, IDValue="", use first frame grabber or directly connected GigE detector
#        = 1 GigE detector by IP address (e.g. 164.54.160.21)
#        = 2 GigE detector by MAC address (e.g. 00005b032e6b, must be lower-case letters)
#        = 3 GigE detector by detector name (e.g. 8#2608).  Can get network detector names with asynReport(10)

# This is for the first PCI/PCIExpress frame grabber detector in the system
PerkinElmerConfig("$(PORT)", 0, "", 0, 0, 0, 0)

# This is for a GigE detector at IP address 164.54.160.204
#PerkinElmerConfig("$(PORT)", 1, 164.54.160.204, 0, 0, 0, 0)

# This is for a GigE detector at MAC address 00005b032e6b
#PerkinElmerConfig("$(PORT)", 2, 00005b032e6b, 0, 0, 0, 0)

# This is for a GigE detector with name 8#2608
#PerkinElmerConfig("$(PORT)", 3, 8#2608, 0, 0, 0, 0)

asynSetTraceIOMask($(PORT), 0, 2)
# Setting this to 9 to enable ASYN_TRACEIO_DRIVER and see XIS calls
#asynSetTraceMask($(PORT),0, 9)

dbLoadRecords("$(ADPERKINELMER)/db/PerkinElmer.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

# Create a standard arrays plugin, set it to get data from Driver.
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
# Set NELEMENTS to at least the total number of pixels in the detector.  The following is a little larger than 4096 x 4096
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,TIMEOUT=1,NDARRAY_PORT=$(PORT),TYPE=Int16,SIZE=16,FTVL=SHORT,NELEMENTS=17000000")

# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd
set_requestfile_path("$(ADPERKINELMER)/perkinElmerApp/Db")

iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")

#asynSetTraceMask($(PORT),0,0xff)
