% -*- text -*-

figure(1);

% fred, freddy, jane, janet each have date/value vectors,
% but taken at different points in time.
%
% Interpolate onto matching dates:
d0=datenum('03-22-2000 17:03:00');
d1=datenum('03-22-2000 17:10:00');
di=linspace(d0,d1,200);

Z(1,:)=interp1(fred.d,fred.v,di);
Z(2,:)=interp1(freddy.d,freddy.v,di);
Z(3,:)=interp1(jane.d,jane.v,di);
Z(4,:)=interp1(janet.d,janet.v,di);
surf(di,1:4,Z);
view(-90,90);
axis tight;
colorbar;
shading interp;

title('{\bfArchive Data Display:}{\it"Waterfall"}')
xlabel('Date: March 22^{nd}, 2000')
datetick('x');
set(gca,'YTick',[1 2 3 4]);
set(gca,'YTickLabel',{'fred','freddy','jane','janet'});
zlabel('value')
rotate3d on
text(datenum('03-22-2000 17:08:30'), 3.5, 10, '\leftarrow Excursion!')


rotate3d on;
