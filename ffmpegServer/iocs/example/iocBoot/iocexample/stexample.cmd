# This file was automatically generated on Thu 24 Jul 2014 16:29:02 BST from
# source: /home/up45/github/ffmpegServer/etc/makeIocs/example.xml
# 
# 

< envPaths

cd "$(INSTALL)"

epicsEnvSet "EPICS_TS_MIN_WEST", '0'


# Loading libraries
# -----------------

# Device initialisation
# ---------------------

cd "$(INSTALL)"

dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver(pdbbase)

# simDetectorConfig(portName, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory)
simDetectorConfig("C1.CAM", 1600, 1200, 1, 50, 0)

ffmpegServerConfigure(8081)
# ffmpegStreamConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxMemory)
ffmpegStreamConfigure("C1.MJPG", 2, 0, "C1.CAM", "0", -1)

# ffmpegFileConfigure(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr)
ffmpegFileConfigure("C1.FILE", 16, 0, "C1.CAM", 0)

# Final ioc initialisation
# ------------------------
cd "$(INSTALL)"
dbLoadRecords 'db/example_expanded.db'
iocInit
