TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC += QCamApi.h
INC += QImaging.h

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
# Note, the following files were manually copied from the QImaging SDK
LIB_INSTALLS_WIN32    += ../os/win32-x86/QCamDriver.lib
BIN_INSTALLS_WIN32    += ../os/win32-x86/QCamDriver.dll

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS_WIN32 += ../os/windows-x64/QCamDriverx64.lib
BIN_INSTALLS_WIN32 += ../os/windows-x64/QCamDriverx64.dll
endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

