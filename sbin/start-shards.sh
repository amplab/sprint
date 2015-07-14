#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

. "$sbin/pullstar-config.sh"
. "$sbin/load-env.sh"

if [ "$BINDIR" == "" ]; then
    BINDIR="$PULLSTAR_BUILD_DIR/thrift/bin"
fi

bin=$BINDIR
bin="`cd "$bin"; pwd`"

if [ "$DATA_PATH" = "" ]; then
    DATA_PATH="$PULLSTAR_HOME/dat"
fi

if [ "$LOG_PATH" = "" ]; then
    LOG_PATH="$PULLSTAR_HOME/log"
fi

mkdir -p $LOG_PATH

if [ "$SHARD_PORT" = "" ]; then
    SHARD_PORT=11001
fi

if [ "$NUM_SHARDS" = "" ]; then
    NUM_SHARDS=1
fi

if [ "$DATA_STRUCTURE" = "" ]; then
    DATA_STRUCTURE=1
fi

if [ "$EXECUTOR_TYPE" = "" ]; then
    EXECUTOR_TYPE=1
fi

limit=$(($NUM_SHARDS - 1))
for i in `seq 0 $limit`; do
    port=$(($SHARD_PORT + $i))
    data_file="$DATA_PATH/data_${i}"
    nohup "$bin/rxshard" -m 1 -p $port -d ${DATA_STRUCTURE} -e ${EXECUTOR_TYPE} $data_file 2>"$LOG_PATH/shard_${i}.log" > /dev/null &
done
