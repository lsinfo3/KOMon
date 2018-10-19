#!/bin/bash
reps=5
for i in $(seq 1 $reps); do
	while read params; do
		sleep 1s
		echo "Starting with $params"
		./run_test.sh $params $i
		sleep 1s
		echo "Done"
	done < $1
done

echo "Finished"
