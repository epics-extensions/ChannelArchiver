% -*- text-mode -*-

figure(1);

subplot(1,1,1);

% fred, freddy, jane, janet each have date/value vectors,
% but taken at different points in time.
%
% Interpolate onto matching dates:
d0=datenum('03-22-2000 17:03:00');
d1=datenum('03-22-2000 17:10:00');
di=linspace(d0,d1,200);
Ai=interp1(fred.d,fred.v,di);
Bi=interp1(jane.d,jane.v,di);

plot3(Ai,Bi,di,'r-',Ai,Bi,di,'b.');
l=length(Ai);
text(Ai(l),Bi(l),di(l),'\leftarrow End');
title('{\bfArchive Data Display: {}^X/_Y, Trajectory}')
xlabel('PV {\it''X''} [mm]')
ylabel('PV {\it''Y''} [mm]')
zlabel('Time on 03-22-2000')
datetick('z');
grid on
view(-90,90);
rotate3d on

disp 'press key'
pause
view(-60,22)