
function archdataplot(a,b)
% archdataplot(a,b)
%

plot(a.d, a.v, 'r', b.d, b.v, 'b');
datetick('x');
al=a.l;
bl=b.l;
t0=min(a.d(1), b.d(1));
t1=max(a.d(al), a.d(bl));
xlabel([datestr(t0) ' - ' datestr(t1)]);
legend(a.n, b.n);
