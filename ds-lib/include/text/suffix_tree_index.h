#ifndef DSL_TEXT_SUFFIX_TREE_INDEX_H_
#define DSL_TEXT_SUFFIX_TREE_INDEX_H_

#include "text/text_index.h"
#include "suffix_tree.h"

namespace dsl {

class SuffixTreeIndex : public TextIndex {
 public:
  SuffixTreeIndex();
  SuffixTreeIndex(const char *input, size_t size);
  SuffixTreeIndex(const std::string& input);

  virtual void search(std::vector<int64_t>& result, const std::string& query) const;
  virtual int64_t count(const std::string& query) const;
  virtual bool contains(const std::string& query) const;

  char charAt(uint64_t i) const;

  virtual size_t serialize(std::ostream& out);
  virtual size_t deserialize(std::istream& in);

 private:
  CompactSuffixTree *st_;
};
}

#endif // DSL_TEXT_SUFFIX_TREE_INDEX_H_
