#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/pullstar-config.sh"
. "$sbin/load-env.sh"

if [ "$BINDIR" = "" ]; then
    BINDIR="$PULLSTAR_BUILD_DIR/thrift/bin"
fi

bin="$BINDIR"
bin="`cd "$bin"; pwd`"

if [ "$LOG_PATH" = "" ]; then
	LOG_PATH="$PULLSTAR_HOME/log"
fi

mkdir -p $LOG_PATH

if [ "$AGGREGATOR_PORT" = "" ]; then
    AGGREGATOR_PORT=11000
fi

if [ "$NUM_SHARDS" = "" ]; then
    NUM_SHARDS=1
fi

mkdir -p $LOG_PATH

nohup "$bin/rxaggregator" -p $AGGREGATOR_PORT -n "$NUM_SHARDS" 2>"$LOG_PATH/aggregator.log" > /dev/null &
