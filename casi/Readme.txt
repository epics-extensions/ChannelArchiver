-*- outline -*- (mode for emacs)

* Version Hell
On Linux, you get most things with e.g. RedHat6.2.

For Win32, it was hard to find a working combination of
SWIG1.1, Perl, DBD::Oracle.
What did work:
- ActivePerl517 or ActivePerl522,
  configured to use PERL_OBJECT, no PerlCRT
- DBI-1.14
- DBD::Oracle-1.06
  Builds after commenting out the "environ" stuff (debugging only)
  at the end of dbdimp.c
- SWIG 1.1-883

I've given up on trying to build loadable modules on solaris.
If anyone is more succesful, let me know.

* Perl
** Win32, ActivePerl522
c:\Progra~1\vstudio\vc98\bin\vcvars32
cd <Perl>\win32
# Edited Makefile for INST location d:\sys\perl,
# Enabled PERL_OBJECT usage, didn't have the PerlCRT.DLL
nmake
nmake test
nmake install

** Unix
Usual ./configure && gmake && gmake test && gmake install

* DBI
** Win32
cd DBI-1.14
perl Makefile.PL
nmake
nmake test
nmake install
nmake realclean

** Unix
Same with "make" for "nmake"

* DBD::Oracle
** Win32
cd DBD-Oracle-1.06
set ORACLE_HOME=d:\sys\Oracle\Ora81
perl Makefile.PL
nmake
# TWO_TASK does not work!
set ORACLE_USERID=scott/tiger@DEVL.BOGART
nmake test
nmake install
nmake realclean

** Unix
export ORACLE_HOME=/home/oracle/product/8.1.6           
export LD_LIBRARY_PATH=$ORACLE_HOME/lib
unset ORACLE_SID
unset TWO_TASK
perl Makefile.PL
make
make test
make install
make realclean

* Python
On Win32, install python and you get tcl/tk.
On Unix, tcl/tk have to be in place first!
** Win32
Version 1.5.2 comes with RedHat 6.2
and as binaries for Win32


** Unix
./configure --with-thread --with-gcc --prefix /home/john2/kasemir
_After_ a first make, check Modules/Setup.
Enable Tkinter, make tcl/tk/blt point to whereever they were
installed,
e.g.  /home/john2/kasemir/[include|lib] 

  gmake clean
  gmake
  gmake install

* PMW
Python Megawidgets PMW.0.8.4 works with python 1.5.2
The Pmw directory tree get copied into
the <python>/lib dir
(where there is already a subdir "lib-tk")

* Tcl/Tk
** Win32
Version 8.0 comes with RedHat 6.2
and is included in the Python 1.5.2 pack for Win32,
since python uses Tk.
Installing a newer version like 8.2.3 is tricky
because the tk that goes with that will break python.

** Unix
# TCL
./configure --enable-gcc --enable-threads --prefix /home/john2/kasemir
  gmake
  gmake install
# Tk
./configure --enable-gcc --enable-shared --prefix /home/john2/kasemir
  gmake
  gmake install
    

* BLT
** Win32
(for tcl and python): blt2.4s matches tcl 8.0
and Python 1.5.2

** Unix (blt2.4n)
  ./configure --with-cc=gcc --prefix /home/john2/kasemir
  # Hack Makefile: OPT=-g -O2 -D_REENTRANT
  gmake 
  gmake install

* SWIG
** Win32
cd Win
# Edit make_win.in to point to d:\sys\swig for installation

** Unix (csh/solaris shown)
setenv CXX g++          # Set CXX to your C++ compiler
./configure --prefix /home/john2/kasemir --with-lang=PYTHON\
    --with-doc=HTML --with-tcl=/home/john2/kasemir\
    --with-py=/home/john2/kasemir
gmake
gmake install
gmake nuke
 














