#include "index/compressed_suffix_tree.h"

dsl::CompressedSuffixTree::CompressedSuffixTree() {
  cst_ = NULL;
}

dsl::CompressedSuffixTree::CompressedSuffixTree(const std::string& input,
                                                const std::string& input_path,
                                                bool construct) {

  uint8_t* data = (uint8_t*) input.c_str();
  uint64_t size = input.length() + 1;

  if (construct) {
    cst_ = new SSTree(data, size, false, 0, SSTree::io_action::save_to,
                      input_path.c_str());
  } else {
    cst_ = new SSTree(data, size, false, 0, SSTree::io_action::load_from,
                      input_path.c_str());
  }
}

std::vector<int64_t> dsl::CompressedSuffixTree::search(
    const std::string& query) const {
  std::vector<int64_t> results;
  uint64_t node = cst_->search((unsigned char *) query.c_str(), query.length());
  if (node == 0) {
    return results;
  }
  uint64_t leftmost_leaf = cst_->leftmost(node);
  uint64_t sa_idx = cst_->leftrank(leftmost_leaf);
  uint64_t count = cst_->numberofleaves(node);
  results.reserve(count);
  for (uint64_t i = 0; i < count; i++) {
    results.push_back(cst_->sa->lookup(sa_idx + i));
  }
  return results;
}

int64_t dsl::CompressedSuffixTree::count(const std::string& query) const {
  uint64_t node = cst_->search((unsigned char *) query.c_str(), query.length());
  return node == 0 ? 0 : cst_->numberofleaves(node);
}

bool dsl::CompressedSuffixTree::contains(const std::string& query) const {
  return cst_->search((unsigned char *) query.c_str(), query.length()) != 0;
}

size_t dsl::CompressedSuffixTree::serialize(std::ostream& out) {
  // Doesn't support manual serialization
  fprintf(stderr, "Manual serialization not supported.");
  fprintf(stderr, "Set construct flag to true in the constructor.");
  return 0;
}

size_t dsl::CompressedSuffixTree::deserialize(std::istream& in) {
  // Doesn't support manual deserialization
  fprintf(stderr, "Manual serialization not supported.");
  fprintf(stderr, "Set construct flag to false in the constructor.");
  return 0;
}
