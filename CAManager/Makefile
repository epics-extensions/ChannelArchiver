FILES  = ./install.tcl
FILES += ./src/CAbgManager.tcl
FILES += ./src/CAManager.tcl
FILES += ./libsrc/comm.tcl
FILES += ./libsrc/gui.tcl
FILES += ./libsrc/httpd.tcl
FILES += ./libsrc/misc.tcl
FILES += ./libsrc/util.tcl
FILES += ./libsrc/xec.tcl
FILES += ./libsrc/plugins/caget.tcl


install.sh: install-tail.sh $(FILES)
	@echo Creating install.sh
	-@mkdir CAtmp
	@cp -r install.tcl src libsrc CAtmp
	@find CAtmp -name CVS | xargs rm -rf
	@shar -m CAtmp | grep -v '^exit 0' >install.shar
	@cat install.shar install-tail.sh >$@
	@chmod +x $@
	@rm -rf CAtmp install.shar
	@echo done.

clean:
	rm install.sh
