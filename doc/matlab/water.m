function water(varargin)
% water(var. arg)
%
% Plot given channels in a "Waterfall" plot.
%
% kasemir@lanl.gov

% disable OpenGL because it doesn't work
opengl neverselect;

% Find common time range d0..d1
% This looks more complicated than it is
% because of the variable argument list...
d0=varargin{1}.d(1);
l=varargin{1}.l;
minl=l;
d1=varargin{1}.d(l);
for i=1:nargin,
    d0=max(d0, varargin{i}.d(1));
    l=varargin{i}.l;
    minl=min(minl, l);
    d1=min(d1, varargin{i}.d(l));
end

% Interpolate onto matching dates in d0..d1, minl steps:
T=linspace(d0, d1, minl);
for i=1:nargin,
    Z(i,:)=interp1(varargin{i}.d, varargin{i}.v, T);
    names{i}=varargin{i}.n;
end

% Plot as colored 3D surface, view from above
surf(T, 1:nargin, Z);
view(-90, 90);
axis tight;
colorbar;
shading interp;

title('{\bfArchive Data Display: }{\it"Waterfall"}')
% xlabel([datestr(d0) ' - ' datestr(d1)]);
datetick('x');
set(gca,'YTick', 1:nargin);
names
set(gca,'YTickLabel', names);
zlabel('value')

rotate3d on
