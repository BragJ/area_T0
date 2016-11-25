TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = qimagingApp
PROD_IOC_Linux += $(PROD_NAME)
PROD_IOC_WIN32 += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += qimagingSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp AppMain.cpp

# Add locally compiled object code
PROD_LIBS           += qimaging
ifeq (win32-x86,$(findstring win32-x86, $(T_A)))
      LIB_LIBS_WIN32 += QCamDriver
	  PROD_LIBS_WIN32 += QCamDriver
endif
ifeq (windows-x64,$(findstring windows-x64, $(T_A)))
      LIB_LIBS_WIN32 += QCamDriverx64
	  PROD_LIBS_WIN32 += QCamDriverx64
endif

ifeq (windows-x64-debug, $(findstring windows-x64-debug, $(T_A)))
USR_LDFLAGS_WIN32 += /NODEFAULTLIB:LIBCMT.lib
endif

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

