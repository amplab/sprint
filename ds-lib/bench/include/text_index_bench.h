#ifndef DSL_BENCH_SUFFIX_TREE_BENCH_H_
#define DSL_BENCH_SUFFIX_TREE_BENCH_H_

#include <string>
#include <vector>

#include "text/text_index.h"
#include "benchmark.h"

namespace dsl_bench {

class TextIndexBench : public Benchmark {
 public:
  /**
   * Constructor for SuffixTree benchmark.
   */
  TextIndexBench(const std::string& input_file, bool construct,
                 int data_structure);

  /**
   * Benchmark search operation on SuffixTree.
   */
  void benchSearch(const std::string& query_file,
                   const std::string& result_path) const;

  /**
   * Benchmark count operation on SuffixTree.
   */
  void benchCount(const std::string& query_file,
                  const std::string& result_path) const;

 private:
  dsl::TextIndex *text_idx_;
};

}

#endif // DSL_BENCH_SUFFIX_TREE_BENCH_H_
