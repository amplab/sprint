#include "text_index_bench.h"

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <streambuf>

#include "text/suffix_array_index.h"
#include "text/suffix_tree_index.h"
#include "text/compressed_suffix_tree.h"
#include "text/ngram_index.h"

dsl_bench::TextIndexBench::TextIndexBench(const std::string& input_file,
                                          bool construct, int data_structure)
    : Benchmark() {
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
    } else if (data_structure == 4) {
      text_idx_ = new dsl::NGramIndex(input_text);

      // Serialize to disk for future use.
      std::ofstream out(input_file + ".ngm");
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
    } else if (data_structure == 4) {
      std::ifstream input_stream(input_file + ".ngm");
      text_idx_ = new dsl::NGramIndex();
      text_idx_->deserialize(input_stream);
      input_stream.close();
    } else {
      fprintf(stderr, "Data structure %d not supported yet.\n", data_structure);
      exit(0);
    }
  }
}

void dsl_bench::TextIndexBench::benchSearch(
    const std::string& query_file, const std::string& result_path) const {
  std::vector<std::string> queries = readQueryFile(query_file);
  std::ofstream result_stream(result_path);

  for (auto query : queries) {
    std::vector<int64_t> results;
    time_t start = get_timestamp();
    text_idx_->search(results, query);
    time_t end = get_timestamp();
    time_t tot = end - start;
    result_stream << results.size() << "\t" << tot << "\n";
    result_stream.flush();
  }

  result_stream.close();
}

void dsl_bench::TextIndexBench::benchCount(
    const std::string& query_file, const std::string& result_path) const {
  std::vector<std::string> queries = readQueryFile(query_file);

  std::ofstream result_stream(result_path);

  for (auto query : queries) {
    time_t start = get_timestamp();
    int64_t result = text_idx_->count(query);
    time_t end = get_timestamp();
    time_t tot = end - start;
    result_stream << result << "\t" << tot << "\n";
    result_stream.flush();
  }

  result_stream.close();
}

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [-t type] [-q query_file] [-r res_file] [file]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 8) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  bool construct = true;
  std::string type = "latency-search";
  std::string query_file = "queries.txt";
  std::string res_file = "res.txt";
  int data_structure = 0;

  while ((c = getopt(argc, argv, "m:t:q:r:d:")) != -1) {
    switch (c) {
      case 'm': {
        construct = atoi(optarg);
        break;
      }
      case 't': {
        type = std::string(optarg);
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
      default: {
        fprintf(stderr, "Unsupported option %c.\n", (char) c);
        exit(0);
      }
    }
  }

  if (optind == argc) {
    print_usage(argv[0]);
    return -1;
  }

  std::string input_file = std::string(argv[optind]);
  dsl_bench::TextIndexBench bench(input_file, construct, data_structure);

  if (type == "latency-search") {
    bench.benchSearch(query_file, res_file);
  } else if (type == "latency-count") {
    bench.benchCount(query_file, res_file);
  } else {
    fprintf(stderr, "Unsupported type %s.\n", type.c_str());
    exit(0);
  }

  return 0;
}
