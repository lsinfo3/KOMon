#!/bin/bash

# CLI Parameters
maxi=${1:-100} # Number of packets per sample
samplesize=${2:-1} # Number of samples
interval=${3:-1}
pps=${4:-NAs}
packetsize=${5:-NA}
artificial_delay=${6:-0}
rand=${7:-0}
rep=$8

# In total N = maxi x samplesize are going to be sampled

# Actual thing
(>&2 echo "Starting measurement period")

((i=1))
((totalsamples=1))

# Set the samplesize for this run
echo $samplesize > /proc/vnfinfo_udp

# Start the vnf
if [[ $artificial_delay -eq 0 ]]; then
	sudo nice -n -20 taskset -c 3 ~/2017-kernel-based-processing-times/code/dummy-vnf/udp-mirror -s $samplesize -f -p $pps -b $packetsize -i $interval -w "VNF_${pps}pps_${packetsize}byte_${interval}_0_${maxi}_${samplesize}_${rand}_${rep}.log" &
else
	if [[ ! $rand -eq 0 ]]; then
		sudo nice -n -20 taskset -c 3 ~/2017-kernel-based-processing-times/code/dummy-vnf/udp-mirror -s $samplesize -d ${artificial_delay} -p $pps -b $packetsize -i $interval -w "VNF_${pps}pps_${packetsize}byte_${interval}_${artificial_delay}_${maxi}_${samplesize}_${rand}_${rep}.log" -r $rand &
	else
		sudo nice -n -20 taskset -c 3 ~/2017-kernel-based-processing-times/code/dummy-vnf/udp-mirror -s $samplesize -d ${artificial_delay} -p $pps -b $packetsize -i $interval -w "VNF_${pps}pps_${packetsize}byte_${interval}_${artificial_delay}_${maxi}_${samplesize}_${rand}_${rep}.log" &
	fi


fi

(>&2 echo "VNF started")

# Wait a little for good measure
sleep 5s

# Start the monitoring loop
while [[ $maxi -eq -1 ]] || [[ $i -le $maxi ]]; do
	(>&2 echo "$i/$maxi")
	if [[ $res != *"ACTIVE"* ]]; then
		echo "$i;$((16#$(cat /proc/net/udp | grep 0:04D2 | awk '{ print $5 }' | cut -d':' -f2)))" >> "BUFFER_${pps}pps_${packetsize}byte_${interval}_${artificial_delay}_${maxi}_${samplesize}_${rand}_${rep}.log"
		echo start > /proc/vnfinfo_udp
		~/2017-kernel-based-processing-times/scripts/monitor_vnf.sh # This sends SIGUSR1 to the vnf and triggers a sample of $samplesize
	fi
	sleep $(python -c "import numpy; print $interval+numpy.random.exponential(scale=$interval)")
	#sleep "$interval"s
	res=$(cat /proc/vnfinfo_udp | tail -n1)
	# The module is not active, so there is nothing to do.
	if [[ $res == *"WAITING"* ]]; then
		(>&2 echo "Module not active! Exit.")
		exit
	# The module is currently actively sampling packets, but has not yet seen a sufficient number of packets
	elif [[ $res == *"ACTIVE"* ]]; then
		(>&2 echo "Not enough packets monitored. Waiting one more round.")
		# TODO: Does it make sense to use the distribution based sleep timer here?
		# sleep $(python -c "import numpy; print numpy.random.exponential(scale=$interval)")
		sleep "$interval"s
	# The module is done sampling and the results can be parsed
	else
	    # Print the current sample
	    ((samplenumber=1))
	    for sample in $(echo "$res" | tr ';' ' '); do
		echo "$totalsamples;$i;$samplenumber;$pps;$packetsize;$interval;$artificial_delay;$sample" >> "KERNEL_${pps}pps_${packetsize}byte_${interval}_${artificial_delay}_${maxi}_${samplesize}_${rand}_${rep}.log"
		((samplenumber=$samplenumber+1))
		((totalsamples=$totalsamples+1))
	    done
	    # Increase number of total samples
	    ((i=$i+1))
	fi
done

# Wait a little for good measure
sleep 5s

# Make sure all counters are reset
# Reset the samplesize to cbsize
echo stop > /proc/vnfinfo_udp
echo -1 > /proc/vnfinfo_udp

# Finally kill the VNF
pid=$(ps aux | grep .\[u]dp-mirror | awk '{ print $2 }')
while [[ ! $pid == "" ]]; do
	for p in $pid; do
		(>&2 echo "Killing $p")
		sudo kill -2 $p
	done
	sleep 1s
	pid=$(ps aux | grep .\[u]dp-mirror | awk '{ print $2 }')
done

# Tell the user we are done
(>&2 echo "Finished")
