function archdataplot(data)
% archdataplot(data)
%
% 2D plot of ChannelArchiver data
%
% kasemir@lanl.gov

plot(data.d, data.v);
datetick('x');
l=data.l;
xlabel([datestr(data.d(l)) ' - ' datestr(data.d(l))]);
title(data.n);

