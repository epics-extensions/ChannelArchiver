function xyplot(a,b)
% xyplot(a,b)
%
% Plot channels A,B as X vs. Y over time.
%
% kasemir@lanl.gov

% disable OpenGL because it doesn't work
opengl neverselect;

% Find common time range
alen=a.l;
blen=b.l;
d0=max(a.d(1), b.d(1));
d1=min(a.d(alen), b.d(blen));

% Interpolate onto matching dates:
T=linspace(d0, d1, min(alen,blen));
X=interp1(a.d, a.v, T);
Y=interp1(b.d, b.v, T);
plot3(X, Y, T);
view([0, 90]);
title('{\bfArchive Data Display:}{\it"XY"}')
zlabel([datestr(d0) ' - ' datestr(d1)]);
datetick('z');
xlabel(a.n);
ylabel(b.n)
grid on;
rotate3d on;
