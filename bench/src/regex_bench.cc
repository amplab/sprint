#include "regex_bench.h"

#include <unistd.h>
#include <cstdlib>
#include <cstdio>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include "text/compressed_suffix_tree.h"
#include "text/suffix_tree_index.h"
#include "text/suffix_array_index.h"

pull_star_bench::RegExBench::RegExBench(const std::string& input_file,
                                        bool construct, int data_structure,
                                        int executor_type)
    : dsl_bench::Benchmark() {
  if (construct) {
    std::ifstream input_stream(input_file);
    const std::string input_text((std::istreambuf_iterator<char>(input_stream)),
                                 std::istreambuf_iterator<char>());
    input_stream.close();
    if (data_structure == 0) {
      text_idx_ = new dsl::SuffixTreeIndex(input_text);

      // Serialize to disk for future use.
      std::ofstream out(input_file + ".st");
      text_idx_->serialize(out);
      out.close();
    } else if (data_structure == 1) {
      text_idx_ = new dsl::CompressedSuffixTree(input_text, input_file);
    } else if (data_structure == 2) {
      text_idx_ = new dsl::SuffixArrayIndex(input_text);

      // Serialize to disk for future use.
      std::ofstream out(input_file + ".sa");
      text_idx_->serialize(out);
      out.close();
    } else if (data_structure == 3) {
      text_idx_ = new dsl::AugmentedSuffixArrayIndex(input_text);

      // Serialize to disk for future use.
      std::ofstream out(input_file + ".asa");
      text_idx_->serialize(out);
      out.close();
    } else {
      fprintf(stderr, "Data structure %d not supported yet.\n", data_structure);
      exit(0);
    }
  } else {
    if (data_structure == 0) {
      std::ifstream input_stream(input_file + ".st");
      text_idx_ = new dsl::SuffixTreeIndex();
      text_idx_->deserialize(input_stream);
      input_stream.close();
    } else if (data_structure == 1) {
      std::ifstream input_stream(input_file);
      const std::string input_text(
          (std::istreambuf_iterator<char>(input_stream)),
          std::istreambuf_iterator<char>());
      text_idx_ = new dsl::CompressedSuffixTree(input_text, input_file, false);
      input_stream.close();
    } else if (data_structure == 2) {
      std::ifstream input_stream(input_file + ".sa");
      text_idx_ = new dsl::SuffixArrayIndex();
      text_idx_->deserialize(input_stream);
      input_stream.close();
    } else if (data_structure == 3) {
      std::ifstream input_stream(input_file + ".asa");
      text_idx_ = new dsl::AugmentedSuffixArrayIndex();
      text_idx_->deserialize(input_stream);
      input_stream.close();
    } else {
      fprintf(stderr, "Data structure %d not supported yet.\n", data_structure);
      exit(0);
    }
  }
  executor_type_ =
      static_cast<pull_star::RegularExpression::ExecutorType>(executor_type);
}

pull_star_bench::RegExBench::RegExBench(int executor_type) {
  executor_type_ =
      static_cast<pull_star::RegularExpression::ExecutorType>(executor_type);
  text_idx_ = NULL;
}

void pull_star_bench::RegExBench::benchRegex(const std::string& query_file,
                                             const std::string& result_path) {
  std::vector<std::string> queries = readQueryFile(query_file);
  std::ofstream result_stream(result_path);

  for (auto query : queries) {
    fprintf(stderr, "Benchmarking query [%s]\n", query.c_str());
    pull_star::RegularExpression regex(query, text_idx_, executor_type_);
    std::set<std::pair<size_t, size_t>> results;
    time_t start = get_timestamp();
    regex.execute();
    regex.getResults(results);
    time_t end = get_timestamp();
    time_t tot = end - start;
    result_stream << results.size() << "\t" << tot << "\n";
    fprintf(stderr, "Query result size = %zu, time = %llu us\n", results.size(), tot);
  }

  result_stream.close();
}

void pull_star_bench::RegExBench::benchRegex(
    pull_star_thrift::AggregatorClient& client, const std::string& query_file,
    const std::string& result_path) {

  std::vector<std::string> queries = readQueryFile(query_file);
  std::ofstream result_stream(result_path);

  uint32_t q_id = 0;
  for (auto query : queries) {
    fprintf(stderr, "Benchmarking query [%s]\n", query.c_str());
    for(uint32_t i = 0; i < 10; i++) {
      std::set<int64_t> results;
      time_t start = get_timestamp();
      client.regexSearch(results, query);
      time_t end = get_timestamp();
      time_t tot = end - start;
      result_stream << q_id << "\t" << i << "\t" << results.size() << "\t" << tot << "\n";
      fprintf(stderr, "Iteration %u, query %u, result size = %zu, time = %llu us\n", i, q_id, results.size(), tot);
    }
    q_id++;
  }

  result_stream.close();
}

void pull_star_bench::RegExBench::benchSearch(const std::string& query_file,
                                             const std::string& result_path) {
  std::vector<std::string> queries = readQueryFile(query_file);
  std::ofstream result_stream(result_path);

  for (auto query : queries) {
    std::vector<int64_t> results;
    time_t start = get_timestamp();
    text_idx_->search(results, query);
    time_t end = get_timestamp();
    time_t tot = end - start;
    result_stream << results.size() << "\t" << tot << "\n";
  }

  result_stream.close();
}

void pull_star_bench::RegExBench::benchSearch(
    pull_star_thrift::AggregatorClient& client, const std::string& query_file,
    const std::string& result_path) {

  std::vector<std::string> queries = readQueryFile(query_file);
  std::ofstream result_stream(result_path);

  uint64_t q_id = 0;
  for (auto query : queries) {
    std::vector<int64_t> results;
    time_t start = get_timestamp();
    client.search(results, query);
    time_t end = get_timestamp();
    time_t tot = end - start;
    result_stream << q_id << "\t" << results.size() << "\t" << tot << "\n";
    q_id++;
  }

  result_stream.close();
}

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-t] [-m mode] [-q query_file] [-r res_file] [-d data_structure] [-e executor_type] [file]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 15) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  bool construct = true;
  bool thrift = false;
  std::string query_file = "queries.txt";
  std::string res_file = "res.txt";
  int data_structure = 0;
  int executor_type = 1;
  int port = 11000;

  while ((c = getopt(argc, argv, "m:tq:r:d:e:p:")) != -1) {
    switch (c) {
      case 'm': {
        construct = atoi(optarg);
        break;
      }
      case 't': {
        thrift = true;
        break;
      }
      case 'q': {
        query_file = std::string(optarg);
        break;
      }
      case 'r': {
        res_file = std::string(optarg);
        break;
      }
      case 'd': {
        data_structure = atoi(optarg);
        break;
      }
      case 'e': {
        executor_type = atoi(optarg);
        break;
      }
      case 'p': {
        port = atoi(optarg);
        break;
      }
      default: {
        fprintf(stderr, "Unsupported option %c.\n", (char) c);
        exit(0);
      }
    }
  }

  pull_star_bench::RegExBench* bench;
  if (!thrift) {

    if (optind == argc) {
      print_usage(argv[0]);
      return -1;
    }

    fprintf(
        stderr,
        "Benchmarking data-structure %d with construct = %d executor type %d\n",
        data_structure, construct, executor_type);

    std::string input_file = std::string(argv[optind]);
    bench = new pull_star_bench::RegExBench(input_file, construct,
                                            data_structure, executor_type);
    bench->benchRegex(query_file, res_file);
  } else {

    fprintf(stderr, "Benchmarking thrift mode...\n");
    using namespace ::apache::thrift::protocol;
    using namespace ::apache::thrift::transport;

    try {
      boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      pull_star_thrift::AggregatorClient client(protocol);
      transport->open();
      bench = new pull_star_bench::RegExBench(executor_type);
      bench->benchRegex(client, query_file, res_file);
    } catch (std::exception& e) {
      fprintf(stderr, "Error in establishing connection: %s\n", e.what());
    }
  }

  return 0;
}
