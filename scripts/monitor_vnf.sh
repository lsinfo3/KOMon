#!/bin/bash

# Send SIGUSR1 to the udp-mirror process, which triggers a sample
kill -10 $(ps aux | grep .\[u]dp-mirror | awk '{ print $2 }')
