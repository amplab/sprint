#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>

#include "regex.h"
#include "text_index.h"
#include "suffix_tree.h"
#include "benchmark.h"

void print_usage(char *exec) {
  fprintf(
      stderr,
      "Usage: %s [-m mode] [file]\n",
      exec);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 4) {
    print_usage(argv[0]);
    return -1;
  }

  int c;
  bool construct = true;

  while ((c = getopt(argc, argv, "m:")) != -1) {
    switch (c) {
      case 'm': {
        construct = atoi(optarg);
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

  dsl::TextIndex *text_idx_;
  std::string input_file = std::string(argv[optind]);
  std::ifstream input_stream(input_file);
  if (construct) {
    const std::string input_text((std::istreambuf_iterator<char>(input_stream)),
                                 std::istreambuf_iterator<char>());
    text_idx_ = new dsl::SuffixTree(input_text);

    // Serialize to disk for future use.
    std::ofstream out(input_file + ".st");
    text_idx_->serialize(out);
    out.close();
  } else {
    text_idx_ = new dsl::SuffixTree();
    text_idx_->deserialize(input_stream);
  }
  input_stream.close();

  while(true) {
    std::string query;
    std::cout << "rxshell> ";
    std::cin >> query;
    pull_star::RegularExpression regex(query, text_idx_);
    std::set<std::pair<size_t, size_t>> results;
    time_t start = dsl_bench::Benchmark::get_timestamp();
    regex.execute();
    time_t end = dsl_bench::Benchmark::get_timestamp();
    time_t tot = (end - start) / 1000;
    regex.explain();
    regex.showResults(10);
    std::cerr << "Query [" << query << "] took " << tot << " ms.\n";
  }

  return 0;
}
