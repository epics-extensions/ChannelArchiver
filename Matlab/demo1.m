clear
disp 'loading data...'
load_data
disp 'plotting...'
figure(1)
subplot(2,2,1)
archdataplot(fred)
title('fred')
subplot(2,2,2)
archdataplot(freddy)
title('freddy')
subplot(2,2,3)
archdataplot(jane)
title('jane')
subplot(2,2,4)
archdataplot(janet)
title('janet')

