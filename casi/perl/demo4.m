% -*- text-mode -*-

figure(1);
subplot(1,3,1);

d0=datenum('03-22-2000 19:40:00');
d1=datenum('03-22-2000 19:50:00');
di=linspace(d0,d1,1000);
vi=interp1(fred.d,fred.v,di);

% deltaT[sec]
dT=(di(2)-di(1))*24*60*60
Hz=1/dT

plot(di,vi,'b-');
datetick;
axis tight
xlabel('Time on March 22^{nd},2000');
title('Value');
hold on;
plot(fred.d,fred.v,'r.');
hold off;

subplot(1,3,2);
f=fft(vi);
f(1)=0; % ignore DC
n=length(f);
power=abs(f(1:n/2)).^2;
nyquist=Hz/2
freq=(1:n/2)/(n/2)*nyquist;
plot(freq,power);
axis([0 nyquist 0 100]);
xlabel('\omega [Hz]');
title('Frequencies');

subplot(1,3,3);
period=1./freq;
plot(period,power);
axis([0 20 0 60]);
axis 'auto y';
xlabel('\DeltaT [sec]');
title('Periods');
th=text(9.6,36.5,'\DeltaT\approx9s!\newline     \downarrow');
set(th,'HorizontalAlignment','center');
set(th,'VerticalAlignment','bottom');
zoom on;