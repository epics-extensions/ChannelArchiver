# Makefile to call sub-makes

INSTAPP=install -o aptdvl -g 10
EXE=
INSTALLDIR=/home/aptdvl/bin
CGIDIR=/home/httpd/html/archive/cgi

include Make.ver

SRCZIP=channelarchiver-$(VERSION).$(RELEASE).$(PATCH).src.zip

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
	@-rm -f CGIExport/Tests/cgi/tmp/*
	@-rm -f CGIExport/Tests/cgi/CGIExport.cgi


new:	clean all

zip:	cleanCGITest
	(cd /tmp; \
	 rm -rf /tmp/Tools;\
	 rm -rf /tmp/ChannelArchiver;\
         cvs -d :ext:kasemir@ics-srv01.sns.ornl.gov:/sns/ADE/cvsroot\
	    export -r Tools-$(VERSION)-$(RELEASE)-$(PATCH) \
	    -d Tools epics/supTop/extensions/1.1/src/Tools;\
         cvs -d :ext:kasemir@ics-srv01.sns.ornl.gov:/sns/ADE/cvsroot\
	    export -r ChannelArchiver-$(VERSION)-$(RELEASE)-$(PATCH) \
	    -d ChannelArchiver epics/supTop/extensions/1.1/src/ChannelArchiver;\
	 rm $(SRCZIP);\
	 zip -r $(SRCZIP) Tools ChannelArchiver\
	)  

install:
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveExport$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveEngine$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/ArchiveManager$(EXE) $(INSTALLDIR)
	$(INSTAPP) ../../bin/$(HOST_ARCH)/CGIExport$(EXE) $(CGIDIR)/`date +CGIExport%y%m%d.cgi`
	-(cd $(CGIDIR); rm CGIExport.cgi)
	(cd $(CGIDIR); ln -s `date +CGIExport%y%m%d.cgi` CGIExport.cgi)

