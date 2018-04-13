#
# Makefile for OpenZWave Linux build
# Greg Satz

# GNU make only

# Requires libudev-dev

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean verscheck

#The Major Version Number
VERSION_MAJ	:= 1
#The Minor Version Number
VERSION_MIN := 0
#The Location of the svnversion command for determining the repository version
SVNVERSION := $(shell which svnversion)


ifeq ($(SVNVERSION),)
VERSION_REV := 0
else
VERSION_REV := $(shell $(SVNVERSION) ../..|awk -F'[^0-9]*' '$$0=$$1')
endif
ifeq ($(VERSION_REV),)
VERSION_REV := 0
endif
# version number to use on the shared library
VERSION := $(VERSION_MAJ).$(VERSION_MIN)

# support Cross Compiling options
CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

DEBUG_CFLAGS    := -Wall -Wno-inline -Wno-format -Werror -g -DDEBUG -DLOG_STDERR -fPIC -Werror  -Wno-unknown-pragmas   -Wno-sequence-point 
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -O3 -DNDEBUG -fPIC -Werror -Wno-unknown-pragmas   -Wno-sequence-point 

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	+= -c $(RELEASE_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS	:= $(RELEASE_LDFLAGS) $(EXTRA_CFLAGS)
#CFLAGS	+= -c $(DEBUG_CFLAGS) $(EXTRA_CFLAGS)
#LDFLAGS	:= $(DEBUG_LDFLAGS) $(EXTRA_CFLAGS)

SRCDIR  ?= ../..
LIBDIR	?= $(SRCDIR)/lib/linux

INCLUDES	:= -I $(SRCDIR)/src -I $(SRCDIR)/src/command_classes/ -I $(SRCDIR)/src/value_classes/ \
	-I $(SRCDIR)/src/platform/ -I $(SRCDIR)/src/platform/unix -I $(SRCDIR)/tinyxml/ 
SOURCES		:= $(SRCDIR)/src $(SRCDIR)/src/command_classes $(SRCDIR)/tinyxml \
	$(SRCDIR)/src/value_classes $(SRCDIR)/src/platform $(SRCDIR)/src/platform/unix $(SRCDIR)/src/aes

#  $(SRCDIR)/hidapi/linux \

VPATH = $(SRCDIR)/src:$(SRCDIR)/src/command_classes:$(SRCDIR)/tinyxml:$(SRCDIR)/hidapi/linux:\
	$(SRCDIR)/src/value_classes:$(SRCDIR)/src/platform:$(SRCDIR)/src/platform/unix $(SRCDIR)/src/aes

%.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

tinyxml  := $(notdir $(wildcard $(SRCDIR)/tinyxml/*.cpp))
#hidapi   := $(notdir $(wildcard $(SRCDIR)/hidapi/linux/hid.c)) # we do not want the libusb version
aes      := $(notdir $(wildcard $(SRCDIR)/src/aes/*.c))
cclasses := $(notdir $(wildcard $(SRCDIR)/src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard $(SRCDIR)/src/value_classes/*.cpp))
pform    := $(notdir $(wildcard $(SRCDIR)/src/platform/*.cpp)) \
	$(notdir $(wildcard $(SRCDIR)/src/platform/unix/*.cpp))
indep    := $(notdir $(wildcard $(SRCDIR)/src/*.cpp))

exclude := BasicWindowCovering.cpp ClimateControlSchedule.cpp \
           Proprietary.cpp Protection.cpp 
# SensorAlarm.cpp
#ControllerReplication.cpp

cclasses := $(filter-out $(exclude),$(cclasses))

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -Wno-reorder -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

default:       $(LIBDIR)/libopenzwave.so

clean:	
	rm -f *.d *.o

XMLLINT := $(shell whereis -b xmllint | cut -c10-)

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../config/device_classes.xsd ../../../config/device_classes.xml
	@$(XMLLINT) --noout --schema ../../../config/options.xsd ../../../config/options.xml
	@$(XMLLINT) --noout --schema ../../../config/manufacturer_specific.xsd ../../../config/manufacturer_specific.xml
	@$(XMLLINT) --noout --schema ../../../config/device_configuration.xsd ../../../config/*/*.xml
endif

-include $(tinyxml:.cpp=.d)
-include $(hidapi:.c=.d)
-include $(aes:.c=.d)
-include $(cclasses:.cpp=.d)
-include $(vclasses:.cpp=.d)
-include $(pform:.cpp=.d)
-include $(indep:.cpp=.d)

#create a vers.cpp file that contains our version and subversion revisions
vers.cpp:
	@echo "Creating vers.cpp file"
	@echo '#include "Defs.h"' > vers.cpp
	@echo 'uint16_t ozw_vers_major = $(VERSION_MAJ);' >> vers.cpp
	@echo 'uint16_t ozw_vers_minor = $(VERSION_MIN);' >> vers.cpp
	@echo 'uint16_t ozw_vers_revision = $(VERSION_REV);' >> vers.cpp
	@echo 'char ozw_version_string[] = "candi/git";' >> vers.cpp

vers.o:	vers.cpp

$(LIBDIR)/libopenzwave.so:	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.c,%.o,$(aes)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	$(LD) $(EXTRA_CFLAGS) -shared -Wl,-soname,libopenzwave.so -o $@ $+
