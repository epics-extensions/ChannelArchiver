% ArchiveData - Channel Archiver Data Access
%
% ArchiveData(URL, CMD, ...) interfaces to the Network Data Server
% of the Channel Archiver.
%
% The two required arguments are:
%
%    URL of the data server, example:
%       'http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi'
%    CMD specifies the command, and the remaining arguments depend
%        on the command, see following examples.
%   
% [ver, desc] = ArchiveData(URL, 'info')
%    Gets version number and description string from data server.   
%
% [keys, names] = ArchiveData(URL, 'archives')
%    Lists available archives by key and name.
%
% [names] = ArchiveData(URL, 'names', KEY [, PATTERN])
%    Lists available archives by key and name.

% kasemir@lanl.gov

% Matlab MEX or Octave OCT file function.


