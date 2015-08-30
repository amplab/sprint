#include "text/suffix_tree_index.h"

dsl::SuffixTreeIndex::SuffixTreeIndex() {
  st_ = NULL;
}

dsl::SuffixTreeIndex::SuffixTreeIndex(const char* input, size_t size) {
  st_ = new CompactSuffixTree(input, size);
}

dsl::SuffixTreeIndex::SuffixTreeIndex(const std::string& input)
    : SuffixTreeIndex(input.c_str(), input.length() + 1) {
}

void dsl::SuffixTreeIndex::search(std::vector<int64_t>& results, const std::string& query) const {
  st::CompactNode* subtree_root = st_->walkTree(query);
  if(subtree_root == NULL) return;
  st_->getOffsets(results, subtree_root);
}

int64_t dsl::SuffixTreeIndex::count(const std::string& query) const {
  st::CompactNode* subtree_root = st_->walkTree(query);
  if(subtree_root == NULL) return 0;
  return st_->countLeaves(subtree_root);
  return 1;
}

bool dsl::SuffixTreeIndex::contains(const std::string& query) const {
  return st_->walkTree(query) != NULL;
}

char dsl::SuffixTreeIndex::charAt(uint64_t i) const {
  return st_->charAt(i);
}

size_t dsl::SuffixTreeIndex::serialize(std::ostream& out) {
  return st_->serialize(out);
  return 0;
}

size_t dsl::SuffixTreeIndex::deserialize(std::istream& in) {
  st_ = new CompactSuffixTree();
  return st_->deserialize(in);
  return 0;
}
