close all
clear all

%cd '../telemetry/all data';
%D:\Dropbox\Work\Grass\working\robot\shell force\5-11-15_data_with_chen\data\';

%load('D:\Dropbox\Work\Grass\working\robot\shell force\5-11-15_data_with_chen\N_matrix_trial9.mat')
load('../telemetry/N_matrix_trial9.mat')
% T = csvread('velociroach_s=3cm_w=3cm_h=10cm_layer=5_f=10Hz_beetleshell_run3.txt',9,0);
T = csvread('../telemetry/alldata/velociroach_s=10cm_w=5.5cm_h=27cm_layer=3_ply=6_f=13Hz_beetleshell_run3.txt',9,0);
data = T;  % data is used by state_plot
state_plot  % process robot state information from telemetry file

S = T(:,17:24);
A = [S(:,1),S(:,1).^2,S(:,1).^3,S(:,2),S(:,2).^2,S(:,2).^3,S(:,3),S(:,3).^2,S(:,3).^3,S(:,4),S(:,4).^2,S(:,4).^3,S(:,5),S(:,5).^2,S(:,5).^3,S(:,6),S(:,6).^2,S(:,6).^3,S(:,7),S(:,7).^2,S(:,7).^3,S(:,8),S(:,8).^2,S(:,8).^3];
Frecov = A*N;
Frecov1 = Frecov;
i = 2;

%eliminate duplicates
while 0
    Flen = size(Frecov1,1);
    if Flen < i
        break
    end
    if sum(Frecov1(i,:) == Frecov1(i-1,:)) == 6
        Frecov1 = [Frecov1(1:i-1,:);Frecov1(i+1:end,:)];
    else
        i = i + 1;
    end
end

T=T/1000000;
Frecov1offset=mean(Frecov1(1:50,:));
for ii=1:6
    Frecov1(:,ii)=Frecov1(:,ii)-Frecov1offset(ii);
end

%%

ftsz=15;  % font size

%%%%%%%%%%%%%%%%%%%%%%%%
% butterworth filter
% sample rate is 1 kHz, try cutoff frequency of 20 Hz
Wn = 20/1000;
N = 4; % filter order
[B,A]=butter(N,Wn);
Frecov1=filter(B,A,Frecov1);
% filter accelerometer as well
AX=filter(B,A,AX);
AY=filter(B,A,AY);
AZ=filter(B,A,AZ);

%%%%%%%%%%%%%%%%%%%%%%%%
figure(1);
clf;
set(gcf,'color','w');
set(gcf,'Units','inches');
set(gcf,'Position',[1 1 8 4]);

subplot(2,1,1);hold all;box on;
plot(T(:,1),Frecov1(:,1),'LineWidth',1.5)
plot(T(:,1),Frecov1(:,2),'LineWidth',1.5)
plot(T(:,1),Frecov1(:,3),'g-','LineWidth',1.5)
line([0 12.3],[0 0],'color','k');
ylabel('F (N)','fontsize',ftsz)
set(gca,'fontsize',ftsz);
legend('F_x','F_y','F_z');
xlabel('Time (s)','fontsize',ftsz);
xlim([0 maxt]);
ylim([-0.4 0.4]);

subplot(2,1,2);hold all;box on;
plot(T(:,1),Frecov1(:,4),'LineWidth',1.5)
plot(T(:,1),Frecov1(:,5),'LineWidth',1.5)
plot(T(:,1),Frecov1(:,6),'g-','LineWidth',1.5)
line([0 12.3],[0 0],'color','k');
ylabel('M (mN*m)','fontsize',ftsz)
set(gca,'fontsize',ftsz);
legend('M_x','M_y','M_z');
xlabel('Time (s)','fontsize',ftsz);
xlim([0 maxt]);
ylim([-20 20]);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% plot of robot state and force sensor
figure(2)
clf;
ha = tight_subplot(7,1,[.02 0],[.1 .08],[.1 .01]);
axes(ha(1)); plot(time(1:s:end)/1000,TorqueR(1:s:end),...
    'r-','LineWidth',2);
hold on; plot(time(1:s:end)/1000,-TorqueL(1:s:end),...
    'b:','LineWidth',2);ylabel('\tau (mN-m)','FontSize', 14, 'FontName', 'CMU Serif');axis([0,maxt,-2.1,2.1]); legend('Right leg','Left leg')
axes(ha(2)); plot(time(1:s:end)/1000,AngleZ(1:s:end),...
    'k','LineWidth',3);ylabel('\theta_Z (rad)','FontSize', 14, 'FontName', 'CMU Serif');
    axis([0,maxt,-.8,10]);% gyro Z angle
axes(ha(3)); plot(time(1:s:end)/1000,AX(1:s:end),'k','LineWidth',2);
    ylabel('x" (m/s^2)','FontSize', 14, 'FontName', 'CMU Serif');axis([0,maxt,-15,15]);
axes(ha(4)); plot(time(1:s:end)/1000,AY(1:s:end),'k','LineWidth',2);
    ylabel('y" (m/s^2)','FontSize', 14, 'FontName', 'CMU Serif');axis([0,maxt,-15,15]);
axes(ha(5)); plot(time(1:s:end)/1000,AZ(1:s:end),'k','LineWidth',2);
    ylabel('z" (m/s^2)','FontSize', 14, 'FontName', 'CMU Serif');axis([-0,maxt,-5,25]);
%%%%% now plot contact forces and torques %%%%%%%%%%
axes(ha(6)); plot(time(1:s:end)/1000,Frecov1(1:s:end,1),'r','LineWidth',2);  
hold on; plot(time(1:s:end)/1000,Frecov1(1:s:end,2),'b','LineWidth',2); 
    plot(time(1:s:end)/1000,Frecov1(1:s:end,3),'g','LineWidth',2); 
ylabel('F(N)','FontSize', 14, 'FontName', 'CMU Serif');
    axis([0,maxt,-0.2,0.2]); legend('F_x','F_y','F_z')
axes(ha(7)); plot(time(1:s:end)/1000,Frecov1(1:s:end,4),'r','LineWidth',2);  
hold on; plot(time(1:s:end)/1000,Frecov1(1:s:end,5),'b','LineWidth',2); 
    plot(time(1:s:end)/1000,Frecov1(1:s:end,6),'g','LineWidth',2); 
ylabel('M (mN-M)','FontSize', 14, 'FontName', 'CMU Serif');
    axis([0,maxt,-20,20]); legend('M_x','M_y','M_z')

set(ha(1:6),'XTickLabel','') % only 1 time lable
for i = 1:7
    axes(ha(i));
    set(gca,'FontName','CMU Serif','FontSize',14);
    %axis([0,10,-1.5,1.5]);
    hold on
    temp = get(gca,'XTick');
    plot([temp(1),temp(end)],[0,0],'k','LineWidth',1);
    grid OFF
    
end
axes(ha(7));
xlabel('Time (s)','FontSize', 18, 'FontName', 'CMU Serif');
set(gcf,'Units','inches');
set(gcf,'Position',[1 1 14 16]);
