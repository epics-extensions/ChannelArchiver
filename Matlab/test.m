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
[names,starts,ends]=ArchiveData(url, 'names', 1, 'IOC')



[names,starts,ends]=ArchiveData(url, 'names', 1, 'IOC.*Load')
datestr(starts)
datestr(ends)
