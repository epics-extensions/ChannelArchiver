function [times,micros,values]=ml_arch_get(url, key, name, t0, t1, how, count)
% Get archive data into Matlab

if (nargin < 7)
   count=100
end

if (nargin < 6)
   how=1
end

data=ArchiveData(url, 'values', key, name, t0, t1, count, how);
times=data(1,:);
micros=round(data(2,:)*1e6);
values=data(3,:);
if nargout < 1
    for i=1:size(data,2)
        disp(sprintf('%s.%06d %g', ...
             datestr(times(i)), micros(i), values(i)))
    end
end
