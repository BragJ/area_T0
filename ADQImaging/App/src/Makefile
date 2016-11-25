TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC_WIN32  = QImaging

PROD_LIBS         += QImaging
LIB_SRCS          += QImaging.cpp

DBD += qimagingSupport.dbd

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

include $(ADCORE)/ADApp/commonLibraryMakefile


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE