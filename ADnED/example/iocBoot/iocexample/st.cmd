#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}


epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "10000000")

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

#####################################################
# ADnED 

ADnEDCreateFactory(0)


epicsEnvSet PORT "N1"
< $(ADNED)/st.cmd.config

#asynSetTraceIOMask("$(PORT)",0,0xFF)
#asynSetTraceMask("$(PORT)",0,0xFF)

#####################################################

#################################################
# autosave

epicsEnvSet IOCNAME example
epicsEnvSet SAVE_DIR /opt/egcs/epics/modules/areaDetector/ADnED

save_restoreSet_Debug(0)

### status-PV prefix, so save_restore can find its status PV's.
save_restoreSet_status_prefix("BL99:CS:Det:ADnED")

set_requestfile_path("$(SAVE_DIR)")
set_savefile_path("$(SAVE_DIR)")

save_restoreSet_NumSeqFiles(3)
save_restoreSet_SeqPeriodInSeconds(600)
set_pass0_restoreFile("$(IOCNAME).sav")
set_pass1_restoreFile("$(IOCNAME).sav")

#################################################

## Load record instances
dbLoadRecords "db/example.db"

cd ${TOP}/iocBoot/${IOC}
iocInit

# Create request file and start periodic 'save'
epicsThreadSleep(2)
makeAutosaveFileFromDbInfo("$(SAVE_DIR)/$(IOCNAME).req", "autosaveFields")
create_monitor_set("$(IOCNAME).req", 1000)

epicsThreadSleep(2)

epicsEnvSet PVNAME "BL99:Det:N1:"
dbpf "$(PVNAME)Det1:StartupProc.PROC","1"
#dbpf "$(PVNAME)Det2:StartupProc.PROC","1"
#dbpf "$(PVNAME)Det3:StartupProc.PROC","1"
#dbpf "$(PVNAME)Det4:StartupProc.PROC","1"












