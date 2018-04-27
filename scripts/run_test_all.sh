#!/bin/bash

while read params; do
	sleep 1s
	echo "Starting with $params"
	./run_test.sh $params
	sleep 1s
	echo "Done"
done < $1

echo "Finished"
