#include "suffix_tree.h"

#include "suffix_array.h"

dsl::SuffixTree::SuffixTree() {
  input_ = NULL;
  size_ = 0;
  root_ = NULL;
}
dsl::SuffixTree::SuffixTree(const char* input, size_t size) {
  input_ = input;
  size_ = size;
  construct();
}

dsl::SuffixTree::SuffixTree(const std::string& input)
    : SuffixTree(input.c_str(), input.length() + 1) {
}

dsl::SuffixTree::~SuffixTree() {
  deleteTree(root_);
  delete root_;
}

void dsl::SuffixTree::deleteTree(st::Node* node) {
  if (node->is_leaf_) {
    return;
  }

  for (auto child : ((st::InternalNode *) node)->children_) {
    deleteTree(child);
    delete child;
  }
}

void dsl::SuffixTree::construct() {

  // First construct a suffix array and lcp array
  fprintf(stderr, "Constructing SA...\n");
  SuffixArray *sa = new SuffixArray(input_, size_);
  uint32_t N = size_;

  // Populate inverse suffix array
  fprintf(stderr, "Constructing ISA...\n");
  uint32_t *isa = new uint32_t[N];
  for (uint32_t i = 0; i < N; i++) {
    isa[sa->at(i)] = i;
  }

  // Populate the LCP array
  fprintf(stderr, "Constructing LCP...\n");
  uint64_t *_lcp = new uint64_t[N]();
  uint32_t lcp_val = 0;
  uint32_t max_lcp_val = 0;
  for (uint32_t i = 0; i < N - 1; i++) {
    uint32_t pos = isa[i];
    uint32_t j = sa->at(pos - 1);
    while (i + lcp_val < N - 1 && j + lcp_val < N - 1
        && input_[i + lcp_val] == input_[j + lcp_val]) {
      lcp_val++;
    }
    _lcp[pos] = lcp_val;
    if (lcp_val > max_lcp_val)
      max_lcp_val = lcp_val;
    if (lcp_val > 0)
      lcp_val--;
  }
  delete[] isa;

  fprintf(stderr, "Deleted ISA...\n");

  BitmapArray *lcp = new BitmapArray(_lcp, N,
                                     Utils::int_log_2(max_lcp_val + 1));
  delete[] _lcp;

  // Now we have the LCP array and the Suffix array.
  // We start constructing the Suffix Tree.

  fprintf(stderr, "Constructing ST...\n");
  // Initialize the root with the first leaf child
  root_ = new st::InternalNode();

#ifdef DEBUG_CONSTRUCT
  fprintf(stderr, "Inserting suffix i=0: (%u, %u)...\n", sa->at(0), N - 1);
  fprintf(stderr, "Inserting segment (%u, %u) in new leaf.\n", sa->at(0), N - 1);
#endif

  st::LeafNode *child = new st::LeafNode(sa->at(0));
  uint32_t last_leaf_id = root_->addChild(sa->at(0), N - 1, child);

  st::LeafNode *last_leaf_node = child;
  uint32_t last_leaf_depth = N - sa->at(0);
  for (uint32_t i = 0; i < N - 1; i++) {
#ifdef DEBUG_CONSTRUCT
    fprintf(stderr, "Inserting suffix i=%u: (%u, %u)...\n", i + 1, sa->at(i + 1), N - 1);
#endif

    // Traverse up the right-most path
    st::InternalNode *current_node = (st::InternalNode *) last_leaf_node
        ->parent_;
    uint32_t path_length = last_leaf_depth
        - current_node->edgeLength(last_leaf_id);
    while (path_length > lcp->at(i + 1)) {
      current_node = (st::InternalNode *) current_node->parent_;
      path_length -= current_node->edgeLength(current_node->leftmostChildId());
    }

    assert(path_length <= lcp->at(i + 1));
    assert(!current_node->is_leaf_);
#ifdef DEBUG_CONSTRUCT
    fprintf(stderr, "path_length = %u, lcp(i+1) = %u\n", path_length, lcp->at(i + 1));
#endif

    if (path_length == lcp->at(i + 1)) {
      st::LeafNode *leaf = new st::LeafNode(sa->at(i + 1));
#ifdef DEBUG_CONSTRUCT
      fprintf(stderr, "path_length == lcp(i+1): Inserting segment (%u, %u) in new leaf.\n", sa->at(i + 1) + lcp->at(i + 1), N - 1);
#endif
      last_leaf_id = current_node->addChild(sa->at(i + 1) + lcp->at(i + 1),
                                            N - 1, leaf);

      last_leaf_node = leaf;
      last_leaf_depth = N - sa->at(i + 1);
    } else {
      // Fetch old node info
      uint32_t old_node_id = current_node->leftmostChildId();
      st::Node *old_node = current_node->children_[old_node_id];
      uint32_t old_node_path_length = path_length
          + current_node->edgeLength(old_node_id);

#ifdef DEBUG_CONSTRUCT
      assert(path_length < lcp->at(i + 1));
      uint32_t old_node_start = current_node->start_[old_node_id];
      uint32_t old_node_end = current_node->end_[old_node_id];
      fprintf(stderr, "path_length < lcp(i+1): Deleting segment (%u, %u)...\n", old_node_start, old_node_end);
#endif

      // Delete old node
      current_node->removeChild(old_node_id);

      // Add new nodes
      st::InternalNode *new_node = new st::InternalNode();

#ifdef DEBUG_CONSTRUCT
      fprintf(stderr, "path_length < lcp(i+1): Adding new internal segment (%u, %u)...\n", sa->at(i) + path_length, sa->at(i) + lcp->at(i + 1) - 1);
#endif
      current_node->addChild(sa->at(i) + path_length,
                             sa->at(i) + lcp->at(i + 1) - 1, new_node);

#ifdef DEBUG_CONSTRUCT
      fprintf(stderr, "path_length < lcp(i+1): Adding back partial old segment (%u, %u)...\n", sa->at(i) + lcp->at(i + 1), sa->at(i) + old_node_path_length - 1);
#endif
      new_node->addChild(sa->at(i) + lcp->at(i + 1),
                         sa->at(i) + old_node_path_length - 1, old_node);

      st::LeafNode *leaf = new st::LeafNode(sa->at(i + 1));
#ifdef DEBUG_CONSTRUCT
      fprintf(stderr, "path_length < lcp(i+1): Adding new leaf segment (%u, %u)...\n", sa->at(i + 1) + lcp->at(i + 1), N - 1);
#endif
      last_leaf_id = new_node->addChild(sa->at(i + 1) + lcp->at(i + 1), N - 1,
                                        leaf);

      last_leaf_node = leaf;
      last_leaf_depth = N - sa->at(i + 1);
    }
  }

  fprintf(stderr, "Deleting SA...\n");
  delete sa;
  fprintf(stderr, "Deleting LCP...\n");
  delete lcp;

#ifdef DEBUG_VERIFY
  display();
#endif

}

int32_t dsl::SuffixTree::getChildId(st::InternalNode *node, char c) {
  // Linear search for now; can replace by binary search
  for (int32_t i = 0; i < node->start_.size(); i++) {
#ifdef DEBUG_QUERY
    fprintf(stderr, "i: %u, path label start: [%c]\n", i, input_[node->start_[i]] == '\0' ? '$' : input_[node->start_[i]]);
#endif
    if (input_[node->start_[i]] == c)
      return i;
  }

  return -1;
}

dsl::st::InternalNode* dsl::SuffixTree::getRoot() {
  return root_;
}

dsl::st::Node* dsl::SuffixTree::walkTree(const std::string& query) {
  st::InternalNode *current_node = root_;
  uint32_t pos = 0;
  while (true) {
    if (pos == query.length()) {
      break;
    }

#ifdef DEBUG_QUERY
    fprintf(stderr, "At pos = %u\n", pos);
#endif

    int32_t child_id = getChildId(current_node, query[pos]);
    uint32_t start_pos = current_node->start_[child_id];
    uint32_t end_pos = current_node->end_[child_id];
    current_node = (st::InternalNode*) current_node->children_[child_id];

    if (child_id == -1) {
#ifdef DEBUG_QUERY
      fprintf(stderr, "Could not find child node for %c\n", query[pos]);
#endif
      return NULL;
    }
#ifdef DEBUG_QUERY
    else {
      fprintf(stderr, "child_id = %u\n", child_id);
    }
    fprintf(stderr, "Found node with start_pos = %u, end_pos = %u\n", start_pos, end_pos);
#endif

    for (uint32_t i = start_pos; i <= end_pos && pos < query.length(); i++) {
      if (input_[i] != query[pos]) {
#ifdef DEBUG_QUERY
        fprintf(stderr, "Could not match %c with %c, i=%u, pos=%u\n", input_[i], query[pos], i, pos);
#endif
        return NULL;
      }
      pos++;
    }
  }

  return current_node;
}

void dsl::SuffixTree::getOffsets(std::vector<int64_t> &results,
                                 st::Node* node) {

  if (node->is_leaf_) {
    results.push_back(((st::LeafNode *) node)->offset_);
    return;
  }

  for (auto child : ((st::InternalNode *) node)->children_) {
    getOffsets(results, child);
  }
}

int64_t dsl::SuffixTree::countLeaves(st::Node *node) {
  if (node->is_leaf_) {
    return 1;
  }

  int64_t count = 0;
  for (auto child : ((st::InternalNode *) node)->children_) {
    count += countLeaves(node);
  }

  return count;
}

size_t dsl::SuffixTree::writeNode(std::ostream& out, st::Node *node) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&(node->is_leaf_)), sizeof(bool));
  out_size += sizeof(bool);

  if (node->is_leaf_) {
    st::LeafNode *lnode = (st::LeafNode *) node;
    out.write(reinterpret_cast<const char *>(&(lnode->offset_)),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);
  } else {
    st::InternalNode *inode = (st::InternalNode *) node;
    uint8_t node_size = inode->children_.size();
    out.write(reinterpret_cast<const char *>(&(node_size)),
              sizeof(uint8_t));
    out_size += sizeof(uint8_t);
    for (uint32_t i = 0; i < node_size; i++) {
      out.write(reinterpret_cast<const char *>(&inode->start_[i]),
                sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }
    for (uint32_t i = 0; i < node_size; i++) {
      out.write(reinterpret_cast<const char *>(&inode->end_[i]),
                sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }
    for (uint32_t i = 0; i < node_size; i++) {
      out_size += writeNode(out, inode->children_[i]);
    }
  }

  return out_size;
}

dsl::st::Node* dsl::SuffixTree::readNode(std::istream& in, size_t *in_size) {
  bool is_leaf;
  in.read(reinterpret_cast<char *>(&is_leaf), sizeof(bool));
  *in_size = (*in_size) + sizeof(bool);
  st::Node *node;
  if (is_leaf) {
    uint32_t offset;
    in.read(reinterpret_cast<char *>(&offset), sizeof(uint32_t));
    *in_size = (*in_size) + sizeof(uint32_t);
    node = new st::LeafNode(offset);
  } else {
    node = new st::InternalNode();
    st::InternalNode *inode = ((st::InternalNode *) node);
    uint8_t node_size;
    in.read(reinterpret_cast<char *>(&node_size), sizeof(uint8_t));
    *in_size = (*in_size) + sizeof(uint8_t);
    inode->start_.reserve(node_size);
    for (uint32_t i = 0; i < node_size; i++) {
      uint32_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(uint32_t));
      *in_size = (*in_size) + sizeof(uint32_t);
      inode->start_.push_back(val);
    }
    inode->end_.reserve(node_size);
    for (uint32_t i = 0; i < node_size; i++) {
      uint32_t val;
      in.read(reinterpret_cast<char *>(&val), sizeof(uint32_t));
      *in_size = (*in_size) + sizeof(uint32_t);
      inode->end_.push_back(val);
    }
    inode->children_.reserve(node_size);
    for (uint32_t i = 0; i < node_size; i++) {
      inode->children_[i] = readNode(in, in_size);
    }
  }

  return node;
}

size_t dsl::SuffixTree::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&size_), sizeof(uint32_t));
  out_size += sizeof(uint32_t);

  out.write(reinterpret_cast<const char *>(input_), size_ * sizeof(char));
  out_size += size_ * sizeof(char);

  out_size += writeNode(out, root_);

  return out_size;
}

size_t dsl::SuffixTree::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&size_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  char *input = new char[size_];
  in.read(reinterpret_cast<char *>(input), size_ * sizeof(char));
  in_size += size_ * sizeof(char);
  input_ = input;

  root_ = (st::InternalNode *) readNode(in, &in_size);

  return in_size;
}

dsl::CompactSuffixTree::CompactSuffixTree() {
  root_ = NULL;
  input_ = NULL;
  size_ = 0;
  num_internal_nodes_ = 0;
  num_leaf_nodes_ = 0;
}

dsl::CompactSuffixTree::CompactSuffixTree(const char* input, uint32_t size) {
  input_ = input;
  size_ = size;
  num_internal_nodes_ = 0;
  num_leaf_nodes_ = 0;
  SuffixTree *st = new SuffixTree(input, size);
  fprintf(stderr, "Compacting Suffix Tree...\n");
  root_ = new st::CompactInternalNode(st->getRoot());
  fprintf(stderr, "Deleting Original Suffix Tree...\n");
  delete st;
}

dsl::CompactSuffixTree::CompactSuffixTree(const std::string& input)
    : CompactSuffixTree(input.c_str(), input.length()) {
}

dsl::CompactSuffixTree::~CompactSuffixTree() {
  deleteTree(root_);
  delete root_;
}

void dsl::CompactSuffixTree::deleteTree(st::CompactNode *node) {
  if (node->is_leaf_) {
    return;
  }

  st::CompactInternalNode *internal_node = (st::CompactInternalNode *) node;
  for (uint32_t i = 0; i < internal_node->size_; i++) {
    st::CompactNode* child = internal_node->children_[i];
    delete child;
  }
}

int32_t dsl::CompactSuffixTree::getChildId(st::CompactInternalNode *node,
                                           char c) {
  // Binary search for character
  int32_t low = 0, high = node->size_ - 1, mid_point = 0;
  while(low <= high) {
    mid_point = low + (high - low) / 2;
    if(c == input_[node->start_[mid_point]]) {
      return mid_point;
    } else if(c < input_[node->start_[mid_point]]) {
      high = mid_point - 1;
    } else {
      low = mid_point + 1;
    }
  }

  return -1;
}

dsl::st::CompactNode* dsl::CompactSuffixTree::walkTree(
    const std::string& query) {
  st::CompactInternalNode *current_node = root_;
  uint32_t pos = 0;
  while (true) {
    if (pos == query.length()) {
      break;
    }

#ifdef DEBUG_QUERY
    fprintf(stderr, "At pos = %u\n", pos);
#endif

    int32_t child_id = getChildId(current_node, query[pos]);
    uint32_t start_pos = current_node->start_[child_id];
    uint32_t end_pos = current_node->end_[child_id];
    current_node = (st::CompactInternalNode*) current_node->children_[child_id];

    if (child_id == -1) {
#ifdef DEBUG_QUERY
      fprintf(stderr, "Could not find child node for %c\n", query[pos]);
#endif
      return NULL;
    }
#ifdef DEBUG_QUERY
    else {
      fprintf(stderr, "child_id = %u\n", child_id);
    }
    fprintf(stderr, "Found node with start_pos = %u, end_pos = %u\n", start_pos, end_pos);
#endif

    for (uint32_t i = start_pos; i <= end_pos && pos < query.length(); i++) {
      if (input_[i] != query[pos]) {
#ifdef DEBUG_QUERY
        fprintf(stderr, "Could not match %c with %c, i=%u, pos=%u\n", input_[i], query[pos], i, pos);
#endif
        return NULL;
      }
      pos++;
    }
  }

  return current_node;
}

void dsl::CompactSuffixTree::getOffsets(std::vector<int64_t>& results,
                                        st::CompactNode* node) {
  if (node->is_leaf_) {
#ifdef DEBUG_QUERY
    fprintf(stderr, "Hit a leaf node with offset %u\n", ((st::CompactLeafNode *) node)->offset_);
#endif
    results.push_back(((st::CompactLeafNode *) node)->offset_);
    return;
  }

  st::CompactInternalNode *internal_node = ((st::CompactInternalNode *) node);
  for (uint32_t i = 0; i < internal_node->size_; i++) {
    st::CompactNode* child = internal_node->children_[i];
    getOffsets(results, child);
  }
}

int64_t dsl::CompactSuffixTree::countLeaves(st::CompactNode* node) {
  if (node->is_leaf_) {
    return 1;
  }

  int64_t count = 0;
  st::CompactInternalNode *internal_node = ((st::CompactInternalNode *) node);
  for (uint32_t i = 0; i < internal_node->size_; i++) {
    st::CompactNode* child = internal_node->children_[i];
    count += countLeaves(child);
  }

  return count;
}

char dsl::CompactSuffixTree::charAt(uint64_t i) const {
  return input_[i];
}

size_t dsl::CompactSuffixTree::writeNode(std::ostream& out,
                                         st::CompactNode *node) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&(node->is_leaf_)), sizeof(bool));
  out_size += sizeof(bool);

  if (node->is_leaf_) {
    st::CompactLeafNode *lnode = (st::CompactLeafNode *) node;
    out.write(reinterpret_cast<const char *>(&(lnode->offset_)),
              sizeof(uint32_t));
    out_size += sizeof(uint32_t);
  } else {
    st::CompactInternalNode *inode = (st::CompactInternalNode *) node;
    out.write(reinterpret_cast<const char *>(&(inode->size_)), sizeof(uint8_t));
    out_size += sizeof(uint8_t);
    for (uint32_t i = 0; i < inode->size_; i++) {
      out.write(reinterpret_cast<const char *>(&inode->start_[i]),
                sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }
    for (uint32_t i = 0; i < inode->size_; i++) {
      out.write(reinterpret_cast<const char *>(&inode->end_[i]),
                sizeof(uint32_t));
      out_size += sizeof(uint32_t);
    }
    for (uint32_t i = 0; i < inode->size_; i++) {
      out_size += writeNode(out, inode->children_[i]);
    }
  }

  return out_size;
}

dsl::st::CompactNode* dsl::CompactSuffixTree::readNode(std::istream& in,
                                                       size_t *in_size) {
  bool is_leaf;
  in.read(reinterpret_cast<char *>(&is_leaf), sizeof(bool));
  *in_size = (*in_size) + sizeof(bool);
  if (is_leaf) {
    uint32_t offset;
    in.read(reinterpret_cast<char *>(&offset), sizeof(uint32_t));
    *in_size = (*in_size) + sizeof(uint32_t);
    num_leaf_nodes_++;
    return new st::CompactLeafNode(offset);
  } else {
    num_internal_nodes_++;
    st::CompactInternalNode *inode = new st::CompactInternalNode();
    in.read(reinterpret_cast<char *>(&inode->size_), sizeof(uint8_t));
    *in_size = (*in_size) + sizeof(uint8_t);
    inode->start_ = new uint32_t[inode->size_];
    for (uint32_t i = 0; i < inode->size_; i++) {
      in.read(reinterpret_cast<char *>(&inode->start_[i]), sizeof(uint32_t));
      *in_size = (*in_size) + sizeof(uint32_t);
    }
    inode->end_ = new uint32_t[inode->size_];
    for (uint32_t i = 0; i < inode->size_; i++) {
      in.read(reinterpret_cast<char *>(&inode->end_[i]), sizeof(uint32_t));
      *in_size = (*in_size) + sizeof(uint32_t);
    }
    inode->children_ = new st::CompactNode*[inode->size_]();
    for (uint32_t i = 0; i < inode->size_; i++) {
      inode->children_[i] = readNode(in, in_size);
    }
    return inode;
  }
}

size_t dsl::CompactSuffixTree::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&size_), sizeof(uint32_t));
  out_size += sizeof(uint32_t);

  out.write(reinterpret_cast<const char *>(input_), size_ * sizeof(char));
  out_size += size_ * sizeof(char);

  out_size += writeNode(out, root_);

  return out_size;
}

size_t dsl::CompactSuffixTree::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&size_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  char *input = new char[size_];
  in.read(reinterpret_cast<char *>(input), size_ * sizeof(char));
  in_size += size_ * sizeof(char);
  input_ = input;

  root_ = (st::CompactInternalNode *) readNode(in, &in_size);

  return in_size;
}
