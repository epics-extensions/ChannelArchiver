% -*- text-mode -*-
%
% This examle assumes that we have a ChannelArchiver
% data structure 'fred'.
%
% We plot both the raw data and the fft
% between d0 and d1.
% Since you are unlikely to have a structure 'fred'
% with values for the given time range, this will
% not work for you.
% Is is simply meant as an example.
%
% kasemir@lanl.gov

figure(1);
subplot(1,3,1);

% Define start & end
d0=datenum('03-22-2000 19:40:00');
d1=datenum('03-22-2000 19:50:00');

% Interpolate onto 1000 points from d0..d1
di=linspace(d0,d1,1000);
vi=interp1(fred.d,fred.v,di);

% deltaT[sec]
dT=(di(2)-di(1))*24*60*60
Hz=1/dT

% Plot interpolation
plot(di,vi,'b-');
datetick;
axis tight
xlabel('Time on March 22^{nd},2000');
title('Value');

% Plot raw data points into the same plot
hold on;
plot(fred.d,fred.v,'r.');
hold off;

% Another subplot: Show fft
% see Matlab Manual about how to interprete
% the result of the fft function
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

% Similar, but use period=1/frequency for the time axis
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
