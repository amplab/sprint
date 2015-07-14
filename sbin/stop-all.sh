#!/usr/bin/env bash

sbin=`dirname "$0"`
sbin=`cd "$sbin"; pwd`

# Stop master
echo "Stopping aggregator..."
"$sbin/stop-aggregator.sh"

# Stop workers
echo "Stopping shards..."
"$sbin/stop-shards.sh"
