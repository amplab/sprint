#ifndef PULL_STAR_INDEX_COMPRESSED_SUFFIX_TREE_H_
#define PULL_STAR_INDEX_COMPRESSED_SUFFIX_TREE_H_

#include "text_index.h"
#include "SSTree.h"

namespace dsl {

class CompressedSuffixTree : public dsl::TextIndex {
 public:
  CompressedSuffixTree();
  CompressedSuffixTree(const std::string &input);

  std::vector<int64_t> search(const std::string& query) const;
  int64_t count(const std::string& query) const;

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

 private:
  SSTree *cst_;
};

}
#endif // PULL_STAR_INDEX_COMPRESSED_SUFFIX_TREE_H_
