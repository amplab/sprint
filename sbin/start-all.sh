# Starts the aggregator and all shards/

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

# Start Shards
echo "Starting shards..."
"$sbin/start-shards.sh"

# Wait for some time
sleep 1

# Start Aggregator
echo "Starting aggregator..."
"$sbin/start-aggregator.sh"
