close all
clear all

%cd '../telemetry/all data';
%D:\Dropbox\Work\Grass\working\robot\shell force\5-11-15_data_with_chen\data\';

%load('D:\Dropbox\Work\Grass\working\robot\shell force\5-11-15_data_with_chen\N_matrix_trial9.mat')
load('../telemetry/N_matrix_trial9.mat')
% T = csvread('velociroach_s=3cm_w=3cm_h=10cm_layer=5_f=10Hz_beetleshell_run3.txt',9,0);
T = csvread('../telemetry/alldata/velociroach_s=10cm_w=5.5cm_h=27cm_layer=3_ply=6_f=13Hz_beetleshell_run3.txt',9,0);

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

ftsz=15;
xlimmax=197/30;
filterfreq=500;

% leave off filter for now
% Frecov1=butterfilter(Frecov1,filterfreq,5);
%%%%%%%%%%%%%%%%%%%%%%%%
% butterworth filter
% sample rate is 1 kHz, try cutoff frequency of 20 Hz
Wn = 20/1000;
N = 4; % filter order
[B,A]=butter(N,Wn);
Frecov1=filter(B,A,Frecov1);


%%%%%%%%%%%%%%%%%%%%%%%%
figure(1);
set(gcf,'color','w');
set(gcf,'Units','inches');
set(gcf,'Position',[1 1 8 4]);

subplot(2,1,1);hold all;box on;
plot(T(:,1),Frecov1(:,1))
plot(T(:,1),Frecov1(:,2))
plot(T(:,1),Frecov1(:,3))
line([0 12.3],[0 0],'color','k');
ylabel('F (N)','fontsize',ftsz)
set(gca,'fontsize',ftsz);
legend('F_x','F_y','F_z');
xlabel('Time (s)','fontsize',ftsz);
xlim([0 xlimmax]);
ylim([-0.4 0.4]);

subplot(2,1,2);hold all;box on;
plot(T(:,1),Frecov1(:,4))
plot(T(:,1),Frecov1(:,5))
plot(T(:,1),Frecov1(:,6))
line([0 12.3],[0 0],'color','k');
ylabel('M (mN*m)','fontsize',ftsz)
set(gca,'fontsize',ftsz);
legend('M_x','M_y','M_z');
xlabel('Time (s)','fontsize',ftsz);
xlim([0 xlimmax]);
ylim([-20 20]);

%%
% figure(1);
% set(gcf,'color','w');
% subplot(3,1,1)
% plot(T(:,1),Frecov1(:,1))
% line([0 12.3],[0 0],'color','k');
% ylabel('F_x (N)')
% subplot(3,1,2)
% plot(T(:,1),Frecov1(:,2))
% ylabel('F_y (N)')
% subplot(3,1,3)
% plot(T(:,1),Frecov1(:,3))
% ylabel('F_z (N)')
% xlabel('time (s)');
% set(gcf,'Units','inches');
% set(gcf,'Position',[1 1 14 8]);
% 
% figure(2);
% set(gcf,'color','w');
% subplot(3,1,1)
% plot(T(:,1),Frecov1(:,4))
% ylabel('M_x (mN*m)')
% subplot(3,1,2)
% plot(T(:,1),Frecov1(:,5))
% ylabel('M_y (mN*m)')
% subplot(3,1,3)
% plot(T(:,1),Frecov1(:,6))
% ylabel('M_z (mN*m)')
% xlabel('time (s)');
% set(gcf,'Units','inches');
% set(gcf,'Position',[1 1 14 8]);

%legend('Fx','Fy','Fz')
%subplot(2,1,2)
%plot(Frecov1(:,4:6))
%legend('Mx','My','Mz')
%set(gcf,'Units','inches');
%set(gcf,'Position',[1 1 14 16]);
