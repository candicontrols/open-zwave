#
# Makefile for OpenzWave Linux applications
# Greg Satz

# GNU make only

# requires libudev-dev

.SUFFIXES:	.cpp .o .a .s

CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG $(EXTRA_CFLAGS)
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3 $(EXTRA_CFLAGs)

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	+= -c $(DEBUG_CFLAGS) $(EXTRA_CFLAGs)
LDFLAGS	+= $(DEBUG_LDFLAGS)

LDFLAGS += -Wl,-rpath=.

SRCDIR ?= ../../../
LIBDIR ?= $(SRCDIR)/lib/linux
EXDIR  ?= .

INCLUDES	:= -I $(SRCDIR)/src -I $(SRCDIR)/src/command_classes/ -I $(SRCDIR)/src/value_classes/ \
	-I $(SRCDIR)/src/platform/ -I $(SRCDIR)/h/platform/unix -I $(SRCDIR)/tinyxml/ -I $(SRCDIR)/hidapi/hidapi/
LIBS := $(wildcard $(LIBDIR)/*.so)

%.o : $(EXDIR)/%.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

all: test

test:	Main.o $(LIBS)
	$(LD) $(EXTRA_CFLAGS) -o $@ $(LDFLAGS) $< $(LIBS) -pthread -ludev

clean:
	rm -f test Main.o

XMLLINT := $(shell whereis -b xmllint | cut -c10-)

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../../config/zwcfg.xsd zwcfg_*.xml
	@$(XMLLINT) --noout --schema ../../../../config/zwscene.xsd zwscene.xml
endif
