disp 'Open archive...'
[a,c,v]=archive('../../Engine/Test/freq_directory');

disp 'List of channels:'
c=channel_find(a);
while channel_valid(c),
    fprintf(1, '   %s\n', channel_name(c));
    channel_next(c);
end

disp 'Find a channel:'
c=channel_find(a, 'jane');
fprintf(1, '-> %s\n', channel_name(c));


disp 'First Value:'
% value_after(c, v, '03-23-2000 10:19:00');
value_after(c, v);
while value_valid(v),
    fprintf(1, '%s   %g %s\n', value_datestr(v), value_get(v), value_status(v));
    value_next(v);
end

archive_close(a,c,v)

