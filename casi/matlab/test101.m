disp 'Open archive...'
a=archive('../../Engine/Test/freq_directory');

disp 'List channels:'
c=find_channel(a);
while channel_valid(c),
    fprintf(1, '   %s\n', channel_name(c));
    channel_next(c);
end
channel_close(c);

disp 'Find a channel:'
c=find_channel(a, 'jane');
fprintf(1, '-> %s\n', channel_name(c));
channel_close(c);

archive_close(a);

