
function archdataplot2(a,b)
% archdataplot2(data)
%
% 2D plot of ChannelArchiver data from two channels a, b
%
% kasemir@lanl.gov


plot(a.d, a.v, 'r', b.d, b.v, 'b');
datetick('x');
al=a.l;
bl=b.l;
t0=min(a.d(1), b.d(1));
t1=max(a.d(al), b.d(bl));
xlabel([datestr(t0) ' - ' datestr(t1)]);
legend(a.n, b.n);
