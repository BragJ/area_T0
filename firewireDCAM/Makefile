#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocboot))

# make sure examples are only built on linux-x86
ifeq ($(EPICS_HOST_ARCH), linux-x86)
    DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard etc))
    ifeq ($(wildcard etc),etc)
    	include $(TOP)/etc/makeIocs/Makefile.iocs
    	UNINSTALL_DIRS += documentation/doxygen $(IOC_DIRS)
    endif
endif

# Comment out the following line to disable building of example iocs
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocs))
include $(TOP)/configure/RULES_TOP
