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
% [keys, names, paths] = ArchiveData(URL, 'archives')
%    Lists available archives by key, name and path.
%
% [names,starts,ends] = ArchiveData(URL, 'names', KEY [, PATTERN])
%    Lists available channels for archive with given KEY.
%    Optional PATTERN is a regular expression.
%    Returns list of names, start and end times.
%
%    Times: All times are in the format of Matlab
%           serial date numbers.
%           Octave needs to use Matlab-compatibility routines
%           datenum, datevec, ...
%
% ??? = ArchiveData(URL, 'values', KEY, NAME, START, END [, COUNT[, HOW]])
%    Gets values from archive with given key.
%    NAME can either be a single name or a cell array 
%    of names: { 'fred';'janet' }.
%    START and END are serial date numbers.
%    COUNT specifies the number of samples or bins,
%          the exact meaning depends on HOW.
%    HOW   specifies the request method.

% Plotting: datetick   <-> 
%  gset xdata time; gset timefmt "%d/%m"; gset format x "%b %d"

% kasemir@lanl.gov

% Matlab MEX or Octave OCT file function.


