TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# NOTE: There are lots of lines that are commented out in this Makefile because
# the adsc driver does not yet support reading back the arrays from disk and doing
# callbacks.  Thus, the application does not need to link to the NDPlugin library,
# or any of the libraries that the plugins use.  In the future this may change.

# Set to YES or NO to use or not use the simadsc simulation library instead of
# the real ADSC detcon_lib_th library

USE_SIMADSC = YES

# ADSC control software location (only used if USE_SIMADSC is NO)

ADSC_HOME = /opt/adscdc

ifeq (YES, $(USE_SIMADSC))
  USR_CXXFLAGS += -DUSE_SIMADSC
endif

# The required ADSC header files are detcon_entry.h, detcon_par.h, and
# detcon_state.h.  USR_INCLUDES should include the directories containing
# these header files (only used if USE_SIMADSC is NO).

ifeq (NO, $(USE_SIMADSC))
  USR_INCLUDES += -I$(ADSC_HOME)/src_std/incl
  USR_INCLUDES += -I$(ADSC_HOME)/src_std/detcon_lib_th
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

LIBRARY_IOC_cygwin32 += adsc
LIBRARY_IOC_Darwin   += adsc
LIBRARY_IOC_Linux    += adsc
LIBRARY_IOC_solaris  += adsc

ifeq (YES, $(USE_SIMADSC))
  SRC_DIRS += ../simadsc
endif

LIB_SRCS += adsc.cpp
ifeq (YES, $(USE_SIMADSC))
  LIB_SRCS += simadsc.c
endif

include $(ADCORE)/ADApp/commonLibraryMakefile

DBD += adscSupport.dbd

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

