
#setwd("/Users/fatmaal-ali/Box Sync/Fatma-Scavenger/plots/results/6-1-results/study2/10runs")
#setwd("/Users/fatmaal-ali/Box Sync/Fatma-Scavenger/plots/results/6-1-results/study2/parsed-10runs-30-flows")
setwd("/Users/fatmaal-ali/Box Sync/Fatma-Scavenger/plots/results/6-1-results/study2/30runs/parsed")
#---------------------------read data----------------------------

#=========read background loss utilization=========

temp = list.files(pattern="bkg-loss-caseA-BE-bRate-1Mbps.*\\.txt")
loss_be_1m = lapply(temp,read.table) 
names(loss_be_1m) <- c("l1", "l12","l2","l3","l4","l6")
#c("l1", "l1.2","l1.6","l2.3","l35","l4.3")
temp = list.files(pattern="bkg-loss-caseA-BE-bRate-50Mbps.*\\.txt")
loss_be_50m = lapply(temp,read.table) 
names(loss_be_50m) <- c("l1", "l12","l2","l3","l4","l6")

temp = list.files(pattern="bkg-loss-caseA-BE-bRate-110Mbps.*\\.txt")
loss_be_110m = lapply(temp,read.table) 
names(loss_be_110m) <- c("l1", "l12","l2","l3","l4","l6")

temp = list.files(pattern="bkg-loss-caseA-BE-bRate-200Mbps.*\\.txt")
loss_be_200m = lapply(temp,read.table) 
names(loss_be_200m) <- c("l1", "l12","l2","l3","l4","l6")

loss_1m_mean =c (mean(loss_be_1m$l1[,1]), mean(loss_be_1m$l2[,1]),mean(loss_be_1m$l3[,1]),mean(loss_be_1m$l4[,1]),mean(loss_be_1m$l6[,1]),mean(loss_be_1m$l12[,1]))
loss_1m_sd =c (sd(loss_be_1m$l1[,1]), sd(loss_be_1m$l2[,1]),sd(loss_be_1m$l3[,1]),sd(loss_be_1m$l4[,1]),sd(loss_be_1m$l6[,1]),sd(loss_be_1m$l12[,1]))

loss_50m_mean =c (mean(loss_be_50m$l1[,1]), mean(loss_be_50m$l2[,1]),mean(loss_be_50m$l3[,1]),mean(loss_be_50m$l4[,1]),mean(loss_be_50m$l6[,1]),mean(loss_be_50m$l12[,1]))
loss_50m_sd =c (sd(loss_be_50m$l1[,1]), sd(loss_be_50m$l2[,1]),sd(loss_be_50m$l3[,1]),sd(loss_be_50m$l4[,1]),sd(loss_be_50m$l6[,1]),sd(loss_be_50m$l12[,1]))

loss_110m_mean =c (mean(loss_be_110m$l1[,1]), mean(loss_be_110m$l2[,1]),mean(loss_be_110m$l3[,1]),mean(loss_be_110m$l4[,1]),mean(loss_be_110m$l6[,1]),mean(loss_be_110m$l12[,1]))
loss_110m_sd =c (sd(loss_be_110m$l1[,1]), sd(loss_be_110m$l2[,1]),sd(loss_be_110m$l3[,1]),sd(loss_be_110m$l4[,1]),sd(loss_be_110m$l6[,1]),sd(loss_be_110m$l12[,1]))

loss_200m_mean =c (mean(loss_be_200m$l1[,1]), mean(loss_be_200m$l2[,1]),mean(loss_be_200m$l3[,1]),mean(loss_be_200m$l4[,1]),mean(loss_be_200m$l6[,1]),mean(loss_be_200m$l12[,1]))
loss_200m_sd =c (sd(loss_be_200m$l1[,1]), sd(loss_be_200m$l2[,1]),sd(loss_be_200m$l3[,1]),sd(loss_be_200m$l4[,1]),sd(loss_be_200m$l6[,1]),sd(loss_be_200m$l12[,1]))


bkg_loss_sws=c(0,0,0,0,0,0)

#------x-axis mean arrival rate -------
#eMean=c(1,1.2,1.6,2.3,4.3,35)
eMean=c(1,2,3,4,6,12)
rev.default
eMean=rev(log(1/eMean))

col=c(palette(rainbow(9)))
col=c("red","#FFAA00","green","#00AAFF")
#burst=c("0.073", "0.135", "0.25", "0.298", "0.367", "0.406","0.432", "0.572")
burst=c("0.073", "0.25", "0.406", "0.572")
plot.new()
#define a grid with 2 rows, 2 col., mar – A numeric vector of length 4, which sets the margin sizes in the following order: bottom, left, top, and right

#-------bkg loss-----------

par(mar=c(5,5,1,1))
par(cex.lab=1.3)
par(cex.axis=1.3)
pshape=c(1,2,3,4)
#burst=c("0.073", "0.250", "0.406", "0.572")
burst=c("0.073", "0.298", "0.432", "0.572")
plot.new()
plot(eMean,rev(loss_200m_mean),type="b",col=col[1],
     xlab="",
     ylab="",
     ylim=range(0,2.1),
     #     xlim=range(0.5,0.95),
     lty=2,pch=pshape[1],lwd=3) 
#z=1.645
z=1.96
mtext(expression(paste("Log"[e]*" EF-Arrival Rate (/sec)")),side=1,line=3,cex=1.3)
arrows(eMean, rev(loss_200m_mean-((loss_200m_sd/sqrt(10))*z)), eMean, rev(loss_200m_mean+((loss_200m_sd/sqrt(10))*z)), length=0.05, angle=90, code=3)
lines(eMean,rev(loss_110m_mean), type="b",col=col[2],lw=3,lty=2,pch=pshape[2])
arrows(eMean, rev(loss_110m_mean-((loss_110m_sd/sqrt(10))*z)), eMean, rev(loss_110m_mean+((loss_110m_sd/sqrt(10))*z)), length=0.05, angle=90, code=3)
lines(eMean,rev(loss_50m_mean), type="b",col=col[3],lw=3,lty=2,pch=pshape[3])
arrows(eMean, rev(loss_50m_mean-((loss_50m_sd/sqrt(10))*z)), eMean, rev(loss_50m_mean+((loss_50m_sd/sqrt(10))*z)), length=0.05, angle=90, code=3)
lines(eMean,rev(loss_1m_mean), type="b",col=col[4],lw=3,lty=2,pch=pshape[4])
arrows(eMean, rev(loss_1m_mean-((loss_1m_sd/sqrt(10))*z)), eMean, rev(loss_1m_mean+((loss_1m_sd/sqrt(10))*z)), length=0.05, angle=90, code=3)
mtext("Packet Loss Rate (%)",side=2,line=2.1,cex=1.3)

#legend("topleft", legend=burst, fill=col, ncol=2, bty="n",cex=0.8)
legend("topleft", rev(burst), lty=2, lwd=2,col=c("red","#FFAA00","green","#00AAFF") ,cex=1.3,pch=pshape)
