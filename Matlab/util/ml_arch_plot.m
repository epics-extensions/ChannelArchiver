function ml_arch_plot(url, key, name, t0, t1, how, count)
% Plot archive data in Matlab

if (nargin < 7)
   count=100
end

if (nargin < 6)
   how=3
end

data=ArchiveData(url, 'values', key, name, t0, t1, count, how);
dates=data(1,:);
values=data(3,:);
plot(dates, values);
datetick('x', 0);
