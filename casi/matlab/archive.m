%ARCHIVE
%
%   [a,c,v] = archive('directory_file')
%
%   Open ChannelArchiver data source, given the directory file.
%   The directory file is often called 'freq_directory'.
%
%   Returns an archive handle, channel iterator and value iterator.
%   All have to be closed with ARCHIVE_CLOSE.
%   The iterators are invalid until initialized with CHANNEL_FIND etc.
%
%   See also ARCHIVE_CLOSE
%   See also CHANNEL_FIND CHANNEL_VALID CHANNEL_NAME CHANNEL_NEXT
%   See also VALUE_AFTER VALUE_VALID VALUE_GET VALUE_STATUS VALUE_DATESTR VALUE_NEXT VALUE_PREV

%   MEX-File function.

