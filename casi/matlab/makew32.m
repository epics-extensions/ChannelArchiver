disp 'Archive methods'
mex -inline -f w32.bat archive.cpp
mex -inline -f w32.bat archive_close.cpp

disp 'Channel methods'
mex -inline -f w32.bat channel_find.cpp
mex -inline -f w32.bat channel_name.cpp
mex -inline -f w32.bat channel_valid.cpp
mex -inline -f w32.bat channel_next.cpp

disp 'Value methods'
mex -inline -f w32.bat value_after.cpp
mex -inline -f w32.bat value_next.cpp
mex -inline -f w32.bat value_prev.cpp
mex -inline -f w32.bat value_valid.cpp
mex -inline -f w32.bat value_datestr.cpp
mex -inline -f w32.bat value_get.cpp
mex -inline -f w32.bat value_status.cpp
