% -*- text-mode -*-

figure(1);
subplot(1,1,1);
plot3(fred.d,1*ones(size(fred.v)),fred.v,'b-',freddy.d,2*ones(size(freddy.v)),freddy.v,'r-',jane.d,3*ones(size(jane.v)),jane.v,'m-',janet.d,4*ones(size(janet.v)),janet.v,'k-')

title('{\bfArchive Data Display},\newline(distorted {\itsin \rm\omega\itt})')
xlabel('Date: March 22^{nd}, 2000')
zlabel('value')
legend('fred','freddy','jane','janet');
a=axis;
a(1)=datenum('03-22-2000 17:00:00');
a(2)=datenum('03-22-2000 17:11:00');
axis(a);
datetick('x');
grid on
rotate3d on

