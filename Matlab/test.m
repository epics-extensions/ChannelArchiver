% Test of ArchiveData                                  -*- octave -*-

global is_matlab;
eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')

url='http://bogart/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
url='http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';

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
ml_arch_plot(url, key, names{1}, ...
	     datenum(2003, 1, 18), datenum(2003, 1, 20), 3, 500);

% Getting & handling 2 PVs at once:
t0 = datenum(2003, 1, 18);
t1 = t0 + 2;
[out, in]=ArchiveData(url, 'values', key, names, t0, t1, 500, 3);
tin=in(1,:);
tout=out(1,:);
in=in(3,:);
out=out(3,:);
if is_matlab==1
    plot(tin, in*10, 'b-', tout, out, 'r-');
    legend('Klystron Input [10 W]', 'Klystron Output [kW]');
    datetick('x', 0);
else
    t0=min(tin(1),tout(1));
    [Y,M,D,h,m,s] = datevec(t0);
    day=floor(t0);
    xlabel(sprintf('Time on %02d/%02d/%04d [24h]', M, D, Y))
    plot(tin-day, in, '-;Klystron Input [10 W];', ...
         tout-day,out, '-;Klystron Output [kW];');
end

