# Makefile to call sub-makes

include Make.ver

all:
	$(MAKE) -C Tools
	$(MAKE) -C LibIO
	$(MAKE) -C Manager
	$(MAKE) -C Export
	$(MAKE) -C CGIExport
	$(MAKE) -C Engine

clean:
	@$(MAKE) -C Tools clean
	@$(MAKE) -C LibIO clean
	@$(MAKE) -C Manager clean
	@$(MAKE) -C Export clean
	@$(MAKE) -C CGIExport clean
	@$(MAKE) -C Engine clean

cleanCGITest:
	@-rm -f CGIExport/Tests/cgi/tmp/*
	@-rm -f CGIExport/Tests/cgi/CGIExport.cgi

SRCZIP=channelarchiver-$(VERSION).$(RELEASE).$(PATCH).src.zip

brokenzip:	cleanCGITest
	(cd /tmp; \
	 rm -rf /tmp/ChannelArchiver;\
         cvs -d :ext:kasemir@ics-srv01.sns.ornl.gov:/sns/ADE/cvsroot\
	    export -r ChannelArchiver-$(VERSION)-$(RELEASE)-$(PATCH) \
	    -d ChannelArchiver epics/supTop/extensions/1.1/src/ChannelArchiver;\
	 rm $(SRCZIP);\
	 zip -r $(SRCZIP) ChannelArchiver\
	)  


