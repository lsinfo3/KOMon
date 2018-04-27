#!/bin/bash

# CLI Parameters
((maxi=${1:-100})) # Number of packets per sample
((samplesize=${2:-1})) # Number of samples

# In total N = maxi x samplesize are going to be sampled

# Config
scale=2 # Mean of exponential distribution determining the inter sample time
pps=1000 # Load as configured at the traffic generator
packetsize=random # Framesize as configured at the traffic generator

# Actual thing
(>&2 echo "Starting measurement period")

((i=1))
((totalsamples=1))

# Set the samplesize for this run
echo $samplesize > /proc/vnfinfo_udp

while [[ $maxi -eq -1 ]] || [[ $i -le $maxi ]]; do
	(>&2 echo "$i/$maxi")
	if [[ $res != *"ACTIVE"* ]]; then
		echo start > /proc/vnfinfo_udp
	fi
	sleep $(python -c "import numpy; print numpy.random.exponential(scale=$scale)")
	res=$(cat /proc/vnfinfo_udp | tail -n1)
	# The module is not active, so there is nothing to do.
	if [[ $res == *"WAITING"* ]]; then
		(>&2 echo "Module not active! Exit.")
		exit
	# The module is currently actively sampling packets, but has not yet seen a sufficient number of packets
	elif [[ $res == *"ACTIVE"* ]]; then
		(>&2 echo "Not enough packets monitored. Waiting one more round.")
		# TODO: Does it make sense to use the distribution based sleep timer here?
		sleep $(python -c "import numpy; print numpy.random.exponential(scale=$scale)")
	# The module is done sampling and the results can be parsed
	else
	    # Increase number of total samples
	    ((i=$i+1))
	    # Print the current sample
	    ((samplenumber=1))
	    for sample in $(echo "$res" | tr ';' ' '); do
		echo "$totalsamples;$i;$samplenumber;$pps;$packetsize;$scale;$sample"
		((samplenumber=$samplenumber+1))
		((totalsamples=$totalsamples+1))
	    done
	fi
done

# Make sure all counters are reset
# Reset the samplesize to cbsize
echo stop > /proc/vnfinfo_udp
echo -1 > /proc/vnfinfo_udp
(>&2 echo "Finished")
