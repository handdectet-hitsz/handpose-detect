clc;
clear all;
%test.dat
%200hz ���ݲ���   6�����ݣ��ֱ���ԭʼgyro����xyz��51hz�˲����gyro����
%%  ԭʼ����
load test

dat = data2';
dat1 = data4';
Ts1 = 0.005;

[m,n]=size(dat);
t1 = (0:n-1)*Ts1;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
A=1;  
B=1;  
H=1; 
% Q=0.01;  % QԽ��˵����ģ�͵�����Խ�ͣ�ԽС��˵������A���õ������������ͻ�С 
% R=0.5; % RԽ��˵���Թ۲����ݵ�����Խ�ͣ���������С������ʧ��
Q=0.001; 
R=5; 
X=zeros(n,1);    
Z=dat;  
P=zeros(1,n);   
x_now = zeros(n,1);
P_now = zeros(n,1);
delt = zeros(n,1);
x = zeros(n,1);

for i=2:n  
    delt(i) = Z(i) - Z(i-1);
end
[b a]=butter(2,0.5); %30hz  30 / (200 / 2)    %
u = filter(b,a,delt);

%kalman����
for i=2:n  
    %Ԥ��
    x(i) = A * x_now(i-1) + B * u(i-1);
    P(i) = A * P_now(i-1) * A' + Q;
    %����
    kg = P(i) / (P(i) + R);
    x_now(i) = x(i) + kg * (Z(i) - x(i));
    P_now(i) = (1 - kg) * P(i);
end


figure(1)
plot(t1,dat,t1,x_now,'r');
title('����');
legend('ԭʼ����','�˲�֮������');
grid on;

 figure(2)
 plot(t1,delt,t1,u);
 title('����');
 legend('delt','U');
 grid on;

% figure(2)
% plot(t1,P_now);
% title('����');
% legend('ԭʼ����','�˲�֮������');
% grid on;


