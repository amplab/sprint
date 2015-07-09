#ifndef PULL_STAR_BENCH_REGEX_BENCH_H_
#define PULL_STAR_BENCH_REGEX_BENCH_H_

#include "benchmark.h"
#include "text_index.h"

namespace pull_star_bench {

class RegExBench : public dsl_bench::Benchmark {
 public:
  RegExBench(const std::string& input_file, bool construct, int data_structure);

  void benchRegex(const std::string& query_file,
                  const std::string& result_path);

 private:
  dsl::TextIndex *text_idx_;
};

}

#endif // PULL_STAR_BENCH_REGEX_BENCH_H_
