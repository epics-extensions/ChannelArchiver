# Problems:
#
# Building a shared library
# can cause problems unless you have
# a new C++ compiler that can compile the Archiver's libs
# *AND* it seems to be necessary that EPICS base
# was also compiled with that compiler and the proper options?
#
# Don't know, yet, but as a matter of fact
# it builds OK on Win32 (with the project file, not this Makefile)
# as well as on Linux,
# but on solaris I wasn't successful so far....
# -> build standalone TCL shell instead of loadable module

include ../Make.ver

CFLAGS=-DATAC_VERSION=$(VERSION)
CFLAGS+=-DATAC_REVISION=$(RELEASE)
CFLAGS+=-DATAC_VERSION_STRING=\"$(VERSION).$(RELEASE)\"

LINUX=1
# SOLARIS=1

BASE=../../../../base
EXTENSIONS=../../..

# RedHat 6.0
ifdef LINUX
	CC=g++
	CFLAGS+=-fPIC
	LD=g++ -shared
	TARGET=module
	TCLLIBS=/usr/local/lib/libtcl8.2.so
# Only needed for TARGET==shell:
#	TCLLIBS+=/usr/local/lib/libtk8.2.so

# PSI configuration:
	TCLLIBS=/usr/lib/libtcl8.0.so
	BASE=/work/epics/base
endif

# solaris @ LEDA/LANL
ifdef SOLARIS
	CC=g++
	CFLAGS+=-fPIC
	LD=/usr/ccs/bin/ld -G -z text
	TARGET=shell
	TCLLIBS=/usr/local/lib/libtcl8.1.so /usr/local/lib/libtk8.1.so
	LIBS = -ldl  -lsocket -lnsl -lm
endif

CFLAGS+=-I $(EXTENSIONS)/include -I $(BASE)/include
LIBS += $(EXTENSIONS)/lib/$(HOST_ARCH)/libChanArchIO.a
LIBS += $(EXTENSIONS)/lib/$(HOST_ARCH)/libTools.a
LIBS += $(BASE)/lib/$(HOST_ARCH)/libca.a
LIBS += $(BASE)/lib/$(HOST_ARCH)/libDb.a
LIBS += $(BASE)/lib/$(HOST_ARCH)/libCom.a
LIBS += $(TCLLIBS)

OUT=O.$(HOST_ARCH)

all: $(OUT) $(TARGET)

module:
	$(CC) $(CFLAGS) -c atac.cpp -o $(OUT)/atac.o
	$(LD) -o $(OUT)/atac.so $(OUT)/atac.o $(LIBS)

shell:
	$(CC) $(CFLAGS) -c atac.cpp -o $(OUT)/atac.o
	$(CC) $(CFLAGS) -c atacAppInit.c -o $(OUT)/atacAppInit.o
	$(CC) -o $(OUT)/atac $(OUT)/atac.o $(OUT)/atacAppInit.o $(LIBS)

clean:
	-rm -rf $(OUT)

$(OUT):
	mkdir $(OUT)


