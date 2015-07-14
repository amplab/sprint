#include "Shard.h"

#include "suffix_tree.h"
#include "compressed_suffix_tree.h"
#include "regex.h"

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

class ShardHandler : virtual public pull_star_thrift::ShardIf {
 public:
  ShardHandler(std::string input_file, int data_structure, bool construct,
               int executor_type) {
    input_file_ = input_file;
    data_structure_ = data_structure;
    construct_ = construct;
    text_idx_ = NULL;
    executor_type_ =
        static_cast<pull_star::RegularExpression::ExecutorType>(executor_type);
  }

  int32_t init() {
    if (construct_) {
      fprintf(stderr, "Constructing data-structures for file %s...\n",
              input_file_.c_str());
      std::ifstream input_stream(input_file_);
      const std::string input_text(
          (std::istreambuf_iterator<char>(input_stream)),
          std::istreambuf_iterator<char>());
      input_stream.close();
      if (data_structure_ == 0) {
        fprintf(stderr, "Constructing suffix tree...\n");
        text_idx_ = new dsl::SuffixTree(input_text);

        // Serialize to disk for future use.
        std::ofstream out(input_file_ + ".st");
        text_idx_->serialize(out);
        out.close();
      } else if (data_structure_ == 1) {
        fprintf(stderr, "Constructing compressed suffix tree...\n");
        text_idx_ = new dsl::CompressedSuffixTree(input_text, input_file_);
      } else {
        fprintf(stderr, "Data structure %d not supported yet.\n",
                data_structure_);
        exit(0);
      }
      fprintf(stderr, "Finished constructing!\n");
    } else {
      fprintf(stderr, "Loading data-structures for file %s...\n",
              input_file_.c_str());
      if (data_structure_ == 0) {
        fprintf(stderr, "Loading suffix tree from file...\n");
        std::ifstream input_stream(input_file_ + ".st");
        text_idx_ = new dsl::SuffixTree();
        text_idx_->deserialize(input_stream);
        input_stream.close();
      } else if (data_structure_ == 1) {
        fprintf(stderr, "Loading compressed suffix tree from file...\n");
        std::ifstream input_stream(input_file_);
        const std::string input_text(
            (std::istreambuf_iterator<char>(input_stream)),
            std::istreambuf_iterator<char>());
        fprintf(stderr, "Read text of size = %zu bytes\n", input_text.length());
        text_idx_ = new dsl::CompressedSuffixTree(input_text, input_file_,
                                                  false);
        input_stream.close();
      }
      fprintf(stderr, "Finished loading!\n");
    }

    return 0;
  }

  void regexSearch(std::set<int64_t> & _return, const std::string& query) {
    std::set<std::pair<size_t, size_t>> results;
    pull_star::RegularExpression regex(query, text_idx_, executor_type_);
    regex.execute();
    regex.getResults(results);
    for (auto res : results) {
      _return.insert(res.first);
    }
  }

 private:
  dsl::TextIndex* text_idx_;
  std::string input_file_;
  int data_structure_;
  bool construct_;
  pull_star::RegularExpression::ExecutorType executor_type_;

};

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-d data-structure] [-p port] [-e executor-type] [file]\n",
      exec);
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 10) {
    print_usage(argv[0]);
    return -1;
  }

  fprintf(stderr, "Command line: ");
  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "%s ", argv[i]);
  }
  fprintf(stderr, "\n");

  int c;
  uint32_t mode = 0, port = 11001, data_structure = 1, executor_type = 1;
  while ((c = getopt(argc, argv, "m:d:p:e:")) != -1) {
    switch (c) {
      case 'm':
        mode = atoi(optarg);
        break;
      case 'd':
        data_structure = atoi(optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'e':
        executor_type = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unrecognized option %c.", c);
        exit(0);
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string filename = std::string(argv[optind]);
  bool construct = (mode == 0) ? true : false;

  boost::shared_ptr<ShardHandler> handler(
      new ShardHandler(filename, data_structure, construct, executor_type));
  boost::shared_ptr<TProcessor> processor(
      new pull_star_thrift::ShardProcessor(handler));
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
    fprintf(stderr, "Unexpected exception at shard server: %s\n", e.what());
  }
  return 0;
}
