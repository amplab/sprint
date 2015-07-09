#include "index/compressed_suffix_tree.h"

dsl::CompressedSuffixTree::CompressedSuffixTree() {
  cst_ = NULL;
}

dsl::CompressedSuffixTree::CompressedSuffixTree(const std::string& input) {
  uint8_t* data = (uint8_t*) input.c_str();
  uint64_t size = input.length() + 1;
  cst_ = new SSTree(data, size);
}

std::vector<int64_t> dsl::CompressedSuffixTree::search(const std::string& query) const {
  uint64_t node = cst_->search((unsigned char *)query.c_str(), query.length());
  uint64_t leftmost_leaf = cst_->leftmost(node);
  uint64_t sa_idx = cst_->leftrank(leftmost_leaf);
  uint64_t count = cst_->numberofleaves(node);
  std::vector<int64_t> results;
  results.reserve(count);
  for (uint64_t i = 0; i < count; i++) {
    results.push_back(cst_->sa->lookup(sa_idx + i));
  }
  return results;
}

int64_t dsl::CompressedSuffixTree::count(const std::string& query) const {
  uint64_t node = cst_->search((unsigned char *)query.c_str(), query.length());
  return cst_->numberofleaves(node);
}

size_t dsl::CompressedSuffixTree::serialize(std::ostream& out) {
  // TODO: Add serialization code
  return 0;
}

size_t dsl::CompressedSuffixTree::deserialize(std::istream& in) {
  // TODO: Add deserialization code
  return 0;
}
