TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USE_SIMADSC = YES

# ADSC control software location (only used if USE_SIMADSC is NO)

ADSC_HOME = /opt/adscdc

ifeq (YES, $(USE_SIMADSC))
  USR_CXXFLAGS += -DUSE_SIMADSC
endif

# The required ADSC library files are libauxlib.a and libdetcon_th.a.  A
# corresponding <name>_DIR definition is required for each of these and should
# be set to the directory containing the library (only used if USE_SIMADSC is
# NO).

ifeq (NO, $(USE_SIMADSC))
  ifeq (cygwin32, $(OS_CLASS))
    auxlib_DIR    = $(ADSC_HOME)/lib/cygwin
    detcon_th_DIR = $(ADSC_HOME)/lib/cygwin
  endif
  ifeq (Darwin, $(OS_CLASS))
    auxlib_DIR    = $(ADSC_HOME)/lib/darwin
    detcon_th_DIR = $(ADSC_HOME)/lib/darwin
  endif
  ifeq (Linux, $(OS_CLASS))
    auxlib_DIR    = $(ADSC_HOME)/lib/linux
    detcon_th_DIR = $(ADSC_HOME)/lib/linux
  endif
  ifeq (solaris, $(OS_CLASS))
    auxlib_DIR    = $(ADSC_HOME)/lib/sun
    detcon_th_DIR = $(ADSC_HOME)/lib/sun
  endif
endif

PROD_NAME = adscApp
PROD_IOC_cygwin32 += $(PROD_NAME)
PROD_IOC_Darwin   += $(PROD_NAME)
PROD_IOC_Linux    += $(PROD_NAME)
PROD_IOC_solaris  += $(PROD_NAME)
# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += adscSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS_DEFAULT += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp
$(PROD_NAME)_SRCS_vxWorks += $(PROD_NAME)_registerRecordDeviceDriver.cpp

# Add locally compiled object code
$(PROD_NAME)_LIBS += adsc
ifeq (NO, $(USE_SIMADSC))
  $(PROD_NAME)_LIBS += detcon_th auxlib
endif

include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

