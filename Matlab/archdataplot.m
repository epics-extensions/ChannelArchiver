function archdataplot(data)
% archdataplot(data)
%
% 2D plot of data as generated by mldump.pl
%

plot(data.d, data.v);
datetick('x');
l=max(size(data.d));
xlabel([datestr(data.d(1)) ' - ' datestr(data.d(l))]);
title(data.n);

