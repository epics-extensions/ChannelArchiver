% Test of ArchiveData                                  -*- octave -*-

eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')

url='http://bogart/cgi-bin/xmlrpc/ArchiveDataServer.cgi';

ml_arch_info(url);
[ver, desc, hows]=ml_arch_info(url);

ml_arch_archives(url);
[keys,names,paths]=ml_arch_archives(url);

names={ 'Test_HPRF:Kly1:Pwr_Fwd_Out', 'Test_HPRF:SSA1:Pwr_Fwd_Out' }
key=3;
ml_arch_names(url, key, 'IOC');
ml_arch_names(url, key, names{1});
ml_arch_get(url, key, names{1}, datenum(2003, 1, 18), datenum(2003, 1, 20),...
            1, 20);
ml_arch_get(url, key, names{1}, datenum(2003, 1, 18), datenum(2003, 1, 20),...
            3, 20);

% Getting 2 PVs at once,....
[data,data2]=ArchiveData(url, 'values', 4, {'DoublePV','EnumPV'}, ...
                         datenum(2004,3,5), now, 100);
for i=1:size(data,2)
  disp(sprintf('%s.%06d %g', ...
               datestr(data(1,i)), round(data(2,i)*1e6), data(3,i)))
end

ml_arch_plot(url, key, names{1}, datenum(2003, 1, 18), datenum(2003, 1, 20), 3, 50);




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


