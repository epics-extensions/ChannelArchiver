all:    oct

oct:	ArchiveData.oct

mex:
	@echo "No clue, yet"

# Includes -------------------------------------------
# Archiver
FLAGS += -I../../../include
# EPICS Base
FLAGS += -I$(EPICS_BASE_RELEASE)/include
FLAGS += -I$(EPICS_BASE_RELEASE)/include/os/$(HOST_ARCH)
# xml-rpc
FLAGS += -I/usr/local/include
# Libraries ------------------------------------------
FLAGS += -L../../../lib/$(EPICS_HOST_ARCH) -lDataServer -lStorage -lTools 
# EPICS Base
FLAGS += -L$(EPICS_BASE_RELEASE)/lib/$(EPICS_HOST_ARCH) -ldbStaticHost -lCom -lca
# compare to output of xmlrpc-c-config libwww-client --libs
FLAGS += -lxmlrpc_client -lwwwxml -lxmltok -lxmlparse -lwwwzip -lwwwinit -lwwwapp -lwwwhtml -lwwwtelnet -lwwwnews -lwwwhttp -lwwwmime -lwwwgopher -lwwwftp -lwwwfile -lwwwdir -lwwwcache -lwwwstream -lwwwmux -lwwwtrans -lwwwcore -lwwwutils -lmd5 -ldl -lz -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok

ArchiveData.oct: ArchiveData.cpp
	mkoctfile -v -DOCTAVE $(FLAGS) ArchiveData.cpp

# Mex ideas:
#
# Ideally, you simply type
#   mex ArchiveData.cpp
# Under RedHat Linux with Matlap 6p1 that worked, but
# when trying to use the stuff I'd get an error
# "...undefined symbol: __gxx_personality_v0".
# Fix: Compile like this from within matlab:
#   eval(['mex -v -f ' matlabroot '/bin/cxxopts.sh ArchiveData.cpp'])

clean:
	-rm ArchiveData.oct