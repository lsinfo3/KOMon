#!/bin/bash

#echo "$((16#$(cat /proc/net/udp | grep 4:E268 | awk '{ print $5 }' | cut -d: -f2)))"

netstat --udp -an | grep 132.187.12.132:6789 | head -n1 | awk '{ print $2 }'
