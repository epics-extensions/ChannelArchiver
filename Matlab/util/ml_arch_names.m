function ml_arch_names(url, key, pattern)
% Plot archive data in Matlab

if (nargin < 3)
    pattern='';
end

[names, starts, ends]=ArchiveData(url, 'names', key, pattern);
for i=1:length(names)
   [ '''' names{i} ''' : ' datestr(starts(i)) ' - ' datestr(ends(i)) ]
end