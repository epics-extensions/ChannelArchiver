% Test of ArchiveData
eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')
if is_matlab==0
	path('MatComp',path);
end

url='http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
key=1;

disp('Server Info:');
[ver, desc]=ArchiveData(url, 'info')

disp('List available archives:')
[keys,names,paths]=ArchiveData(url, 'archives')
% With Matlab, use
% celldisp(paths)
% to see the contents

disp('List channel names:');
[names,starts,ends]=ArchiveData(url, 'names', 1, 'IOC');
names

[names,starts,ends]=ArchiveData(url, 'names', 1, 'IOC.*Load');
names
datestr(starts)
datestr(ends)

disp('Get Values:');
[times,values]=ArchiveData(url, 'values', key, 'fred', datenum(2004,2,25,16,44,55), now, 5);
for i=1:length(times)
    disp(sprintf('%s %g', datestr(times(i)), values(i)))
end

% ArchiveData(url, 'values', key, {'fred';'janet'}, datenum(2003,3,11), now, 42, 3)
