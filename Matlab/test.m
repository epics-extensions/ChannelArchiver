% Test of ArchiveData                                  -*- octave -*-

eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')

url='http://bogart/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
key=3;

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

key=4;

disp('Get Values:');
[data,data2]=ArchiveData(url, 'values', key, {'DoublePV','EnumPV'}, ...
                         datenum(2004,3,5), now, 100);
for i=1:size(data,2)
  disp(sprintf('%s.%06d %g', ...
               datestr(data(1,i)), round(data(2,i)*1e6), data(3,i)))
end

dates=data(1,:);
values=data(3,:);
if is_matlab==1
   eval('plot(dates, values); datetick(''x'', 0);');
else
   [Y,M,D,h,m,s] = datevec(dates(i));
   day=floor(dates(1));
   xlabel(sprintf('Time on %02d/%02d/%04d [24h]', M, D, Y))
   plot(dates-day, values, '-@;DoublePV;')
end


