cd CAtmp
echo
if [ "x$DISPLAY" = "x" ]
then
  tclsh install.tcl
else
  wish install.tcl
fi
cd ..
rm -rf CAtmp
