# Makefile to call sub-makes

INSTAPP=install -o aptdvl -g 10
EXE=
INSTALLDIR=/home/aptdvl/bin
CGIDIR=/home/httpd/html/archive/cgi

include Make.ver

SRCZIP=channelarchiver-$(VERSION).$(RELEASE).src.zip

all:
	$(MAKE) -C ../Tools
	$(MAKE) -C LibIO
	$(MAKE) -C Manager
	$(MAKE) -C Export
	$(MAKE) -C CGIExport
	$(MAKE) -C Engine


clean:
	@$(MAKE) -C ../Tools clean
	@$(MAKE) -C LibIO clean
	@$(MAKE) -C Manager clean
	@$(MAKE) -C Export clean
	@$(MAKE) -C CGIExport clean
	@$(MAKE) -C Engine clean

cleanCGITest:
	@-rm CGIExport/Tests/cgi/tmp/*
	@-rm CGIExport/Tests/cgi/CGIExport.cgi


new:	clean all

zip:	cleanCGITest
	(cd ..;rm $(SRCZIP);zip -r $(SRCZIP) Tools ChannelArchiver -x \*/O.\* \*/Debug/\*)

install:
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveExport$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveEngine$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveManager$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/CGIExport$(EXE) $(CGIDIR)/`date +CGIExport%y%m%d.cgi`
	-(cd $(CGIDIR); rm CGIExport.cgi)
	(cd $(CGIDIR); ln -s `date +CGIExport%y%m%d.cgi` CGIExport.cgi)

