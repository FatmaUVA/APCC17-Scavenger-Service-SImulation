#M/G/1
#caseB, lc=lp=1Gbps
#poisson arrival flows
#lambda was chosen so that rho will not be larger than 1
lambda1 <- seq(0.022,0.036,by=0.002)
#link headroom
l=600e6
#service_time follows pareto (based on file size)
file_mean = c(8*2e9,8*200e6) #2GB,200MB
serv_mean=file_mean/l
alpha=4
alpha2=2.1
xm = (serv_mean*(alpha-1))/alpha
#xm2 = (serv_mean*(alpha2-1))/alpha2
#change xm, then use xm and alpha to get new mean and new rho
xm2=10
serv_mean2=(xm2*alpha2)/(alpha2-1)
serv_var=( (xm/(alpha-1))^2 ) * ( alpha/(alpha-2) )
serv_var2=( (xm2/(alpha2-1))^2 ) * ( alpha2/(alpha2-2) )
mu=1/serv_mean
mu_alpha2=1/serv_mean2
#traffic intensity
rho1=lambda1/mu[1]
rho2=lambda1/mu[2]
rho1_alpha2=lambda1/mu_alpha2
#expected number of EF
E_n1 = (rho1/2) + ( (rho1+(((lambda1^2)*serv_var[1])) ) / (2*(1-rho1)) )
E_n2 = (rho2/2) + ( (rho2+(((lambda1^2)*serv_var[2])) ) / (2*(1-rho2)) )
E_n1_alpha2 = (rho1_alpha2/2) + ( (rho1_alpha2+(((lambda1^2)*serv_var2)) ) / (2*(1-rho1_alpha2/2)) )
E_n2
#average EF throughput
thru1=600/E_n1
thru1
thru1=600/E_n1
thru2=c(600,600,600,600,600,600,600,600)
thru3=600/E_n1_alpha2

#plot
par(mar=c(4.2,4.2,2,4.2))
par(cex.lab=1.4)
par(cex.axis=1.4)
plot(lambda1,thru1,
     col = "deepskyblue4",
     ylim = range(0,600),
     xlab = "",
     ylab = "Average Throughput (Mpbs)",type="b",pch=3,lty=1,lwd=4)
lines(lambda1,thru2,type="b",pch=5,lty=1,lwd=4,col="green4")
lines(lambda1,thru3,type="b",pch=6,lty=1,lwd=4,col="red")
mtext(expression(paste("EF-Arrival Rate (/sec)")),side=1,line=3,cex=1.4)

par(new=TRUE)
plot(lambda1,rho1*100,type="b",col="deepskyblue4",xaxt="n",yaxt="n",xlab="",ylab=""
     ,ylim=range(0,100)
     ,lty=3,lwd=4,pch=3)
lines(lambda1,rho2*100,type="b",pch=5,lty=3,lwd=4,col="green4")
lines(lambda1,rho1_alpha2*100,type="b",pch=6,lty=3,lwd=4,col="red")
axis(4)
mtext("EF Link Utilization (%)",side=4,line=3,cex=1.4)


