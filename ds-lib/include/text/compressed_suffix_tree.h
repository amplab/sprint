#ifndef DSL_COMPRESSED_SUFFIX_TREE_H_
#define DSL_COMPRESSED_SUFFIX_TREE_H_

#include "SSTree.h"
#include "text/text_index.h"

namespace dsl {

class CompressedSuffixTree : public dsl::TextIndex {
 public:
  CompressedSuffixTree();
  CompressedSuffixTree(const std::string &input, const std::string& input_path, bool construct = true);

  void search(std::vector<int64_t>& results, const std::string& query) const;
  int64_t count(const std::string& query) const;
  bool contains(const std::string& query) const;

  char charAt(uint64_t i) const;

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

 private:
  SSTree *cst_;
};

}
#endif // DSL_COMPRESSED_SUFFIX_TREE_H_
