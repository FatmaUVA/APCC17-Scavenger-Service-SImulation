
stime=14400 #(4 hrs)
count=0

for caseX in 'A'
do
    for i in  12.000000 #1.000000 4.000000 #12.000000
    do
       for j in  1 50   #1 10 30 50 70 90 110 200 300 400 500 done 1 7 500 10 110 300 30 90 400
       do
	 no_flows=`python -c "from math import ceil; print ceil(400/(0.2*$j))"`
	 unit=Mbps
	 bRate=$j$unit #flow rate for each burst
         log_dir=6-18-results-burst-30runs-queue-10k-node$1
	 mkdir -p $log_dir/eMean-$i/log
	 for k in 'BE' #'SWS'
	 do
	   for seed in `seq $2 $3`
	   do
             count=$((count+1))
             if [ $count -eq 3 ]; then
                count=0
        	./waf --run "scratch/caseX-bkg-no-flow-30 --randSeed=$seed --service=$k --eMean=$i --caseX=$caseX --mean_flow_arrival=$no_flows --mean_rate=$bRate --log_dir=$log_dir" > $log_dir/eMean-$i/log/log-case$caseX-$k-eMean-$i-bRate-$j-seed-$seed.txt 
	     else
                ./waf --run "scratch/caseX-bkg-no-flow-30 --randSeed=$seed --service=$k --eMean=$i --caseX=$caseX --mean_flow_arrival=$no_flows --mean_rate=$bRate --log_dir=$log_dir" > $log_dir/eMean-$i/log/log-case$caseX-$k-eMean-$i-bRate-$j-seed-$seed.txt &
             fi
		sleep 3
	    	echo done initiating $k eMean $i bRate $j seed $seed
	   done
	 done
	done
    done
done
