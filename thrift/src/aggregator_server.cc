#include "Aggregator.h"

#include "Shard.h"

#include <unistd.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/concurrency/PosixThreadFactory.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

class AggregatorHandler : virtual public pull_star_thrift::AggregatorIf {
 public:
  AggregatorHandler(int32_t num_shards, bool auto_init = true) {
    num_shards_ = num_shards;
    if (auto_init) {
      init();
    }
  }

  int32_t init() {
    fprintf(stderr, "Attempting to connect with %d shard(s)...\n", num_shards_);
    bool success;
    do {
      success = true;
      for (int32_t i = 0; i < num_shards_; i++) {
        int port = 11001 + i;
        try {
          boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
          boost::shared_ptr<TTransport> transport(
              new TBufferedTransport(socket));
          boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
          pull_star_thrift::ShardClient client(protocol);
          transport->open();
          fprintf(stderr, "Connected to QueryServer %u!\n", i);
          client.send_init();
          shard_clients_.push_back(client);
          shard_transports_.push_back(transport);
        } catch (std::exception& e) {
          fprintf(stderr, "Could not connect to client! Error message: %s\n",
                  e.what());
          success = false;
          break;
        }
      }

      if (!success) {
        for (auto transport : shard_transports_) {
          transport->close();
        }
        shard_transports_.clear();
        shard_clients_.clear();
        fprintf(stderr, "Will retry to connect to client in 30 seconds...\n");
        sleep(30);
      } else {
        for (auto client : shard_clients_) {
          int32_t status = client.recv_init();
          if(status != 0) {
            fprintf(stderr, "Initialization failed!\n");
            return 0;
          }
        }
      }
    } while (!success);

    fprintf(stderr, "Connected to all %d shard(s)!\n", num_shards_);

    return 0;
  }

  void regexSearch(std::set<int64_t> & _return, const std::string& query) {
    for (auto client : shard_clients_) {
      client.send_regexSearch(query);
    }

    for (auto client : shard_clients_) {
      std::set<int64_t> res;
      client.recv_regexSearch(res);
      _return.insert(res.begin(), res.end());
    }
  }

  void search(std::vector<int64_t> & _return, const std::string& query) {
    for (auto client : shard_clients_) {
      client.send_search(query);
    }

    for (auto client : shard_clients_) {
      std::vector<int64_t> res;
      client.recv_search(res);
      _return.insert(_return.begin(), res.begin(), res.end());
    }
  }

 private:
  int32_t num_shards_;
  std::vector<pull_star_thrift::ShardClient> shard_clients_;
  std::vector<boost::shared_ptr<TTransport>> shard_transports_;
};

void print_usage(char *exec) {
  fprintf(stderr, "Usage: %s [-p port] [-n num-shards] [file]\n", exec);
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 6) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  uint32_t port = 11000, num_shards = 1;
  while ((c = getopt(argc, argv, "p:n:")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
      case 'n':
        num_shards = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unrecognized option %c.", c);
        exit(0);
    }
  }

  boost::shared_ptr<AggregatorHandler> handler(
      new AggregatorHandler(num_shards));
  boost::shared_ptr<TProcessor> processor(
      new pull_star_thrift::AggregatorProcessor(handler));
  try {
    boost::shared_ptr<TServerSocket> server_transport(new TServerSocket(port));
    boost::shared_ptr<TBufferedTransportFactory> transport_factory(
        new TBufferedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocol_factory(
        new TBinaryProtocolFactory());
    TThreadedServer server(processor, server_transport, transport_factory,
                           protocol_factory);
    server.serve();
  } catch (std::exception& e) {
    fprintf(stderr, "Unexpected exception at aggregator server: %s\n",
            e.what());
  }

  return 0;
}
