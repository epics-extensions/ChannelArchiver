disp 'Archive methods'
mex -f w32.bat archive.cpp
mex -f w32.bat archive_close.cpp

disp 'Channel methods'
mex -f w32.bat channel_find.cpp
mex -f w32.bat channel_name.cpp
mex -f w32.bat channel_valid.cpp
mex -f w32.bat channel_next.cpp
mex -f w32.bat channel_close.cpp
