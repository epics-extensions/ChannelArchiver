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

names={ 'Test_HPRF:Kly1:Pwr_Fwd_Out', 'Test_HPRF:SSA1:Pwr_Fwd_Out' }
ml_arch_names(url, key, 'IOC')
ml_arch_names(url, key, names{1})


disp('Get Values:');
[data,data2]=ArchiveData(url, 'values', 4, {'DoublePV','EnumPV'}, ...
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


% Only Matlab:
count=100;
how=3;
t0=datenum(2003,9,17);
t1=datenum(2003,9,20);
[out, in]=ArchiveData(url, 'values', key, names, t0, t1, count, how);
tin=in(1,:);
tout=out(1,:);
in=in(3,:);
out=out(3,:);
plot(tin, in*10, 'b-', tout, out, 'r-');
legend('Klystron Input [10 W]', 'Klystron Output [kW]');
datetick('x', 0);


