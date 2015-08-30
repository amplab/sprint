#ifndef DSL_SUFFIX_TREE_H_
#define DSL_SUFFIX_TREE_H_

#include <cassert>
#include <vector>

#include "bitmap_array.h"

namespace dsl {

namespace st {

struct Node {
  Node(bool is_leaf) {
    is_leaf_ = is_leaf;
    parent_ = NULL;
  }
  Node* parent_;
  bool is_leaf_;
};

struct InternalNode : public Node {
  InternalNode()
      : Node(false) {
  }

  uint32_t edgeLength(uint32_t child_id) {
    return end_.at(child_id) - start_.at(child_id) + 1;
  }

  uint32_t leftmostChildId() {
    return children_.size() - 1;
  }

  uint32_t addChild(uint32_t start, uint32_t end, st::Node *child) {
    assert(end >= start);

    start_.push_back(start);
    end_.push_back(end);
    children_.push_back(child);
    child->parent_ = this;
    return children_.size() - 1;
  }

  void removeChild(uint32_t child_id) {
    assert(child_id < children_.size());

    start_.erase(start_.begin() + child_id);
    end_.erase(end_.begin() + child_id);
    children_.erase(children_.begin() + child_id);
  }

  std::vector<uint32_t> start_;
  std::vector<uint32_t> end_;
  std::vector<Node *> children_;
};

struct LeafNode : public Node {
  LeafNode(uint32_t offset)
      : Node(true) {
    offset_ = offset;
  }
  uint32_t offset_;
};

struct CompactNode {
  CompactNode(bool is_leaf) {
    is_leaf_ = is_leaf;
  }
  virtual ~CompactNode() {
  }

  virtual size_t size() = 0;
  bool is_leaf_;
};

struct CompactLeafNode : CompactNode {
  CompactLeafNode(uint32_t offset)
      : CompactNode(true) {
    offset_ = offset;
  }

  CompactLeafNode(LeafNode *node)
      : CompactNode(true) {
    offset_ = node->offset_;
  }

  size_t size() {
    return sizeof(uint32_t) + sizeof(uint8_t);
  }

  uint32_t offset_;
};

struct CompactInternalNode : CompactNode {
  CompactInternalNode()
      : CompactNode(false) {
    size_ = 0;
    start_ = NULL;
    end_ = NULL;
    children_ = NULL;
  }

  CompactInternalNode(InternalNode *node)
      : CompactNode(false) {
    assert(node->children_.size() < 256);

    size_ = node->children_.size();
    start_ = new uint32_t[size_];
    end_ = new uint32_t[size_];
    std::copy(node->start_.begin(), node->start_.end(), start_);
    std::copy(node->end_.begin(), node->end_.end(), end_);
    children_ = new CompactNode*[size_];
    for (uint8_t i = 0; i < size_; i++) {
      if (node->children_[i]->is_leaf_) {
        children_[i] = new st::CompactLeafNode(((LeafNode *) node->children_[i]));
      } else {
        children_[i] = new st::CompactInternalNode(((InternalNode *) node->children_[i]));
      }
    }
  }

  size_t size() {
    size_t subtree_size = 2 * sizeof(uint8_t) + size_ * (sizeof(uint32_t) * 2 + sizeof(CompactNode *));
    for(uint8_t i = 0; i < size_; i++) {
      subtree_size += children_[i]->size();
    }
    return subtree_size;
  }

  uint8_t size_;
  uint32_t* start_;
  uint32_t* end_;
  CompactNode** children_;
};
}

class SuffixTree {
 public:
  SuffixTree();
  SuffixTree(const std::string& input);
  SuffixTree(const char *input, size_t size);
  ~SuffixTree();

  st::Node* walkTree(const std::string& query);
  void getOffsets(std::vector<int64_t> &results, st::Node* node);
  int64_t countLeaves(st::Node* node);
  st::InternalNode* getRoot();

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

#ifdef DEBUG_CONSTRUCT
  void display() {
    show(root_, 0);
  }

  void show(st::Node* node, uint32_t level) {
    fprintf(stderr, "Level: %u, IsLeaf?: %d\n", level, node->is_leaf_);
    if (node->is_leaf_)
    return;

    st::InternalNode* int_node = (st::InternalNode*) node;
    for (uint32_t i = 0; i < int_node->children_.size(); i++) {
      fprintf(stderr, "Path Label: ");
      print(int_node->start_[i], int_node->end_[i]);
      show(int_node->children_[i], level + 1);
    }
  }

  void print(uint32_t s, uint32_t e) {
    for (uint32_t i = s; i <= e; i++) {
      fprintf(stderr, "%c", input_[i] == '\0' ? '$' : input_[i]);
    }
    fprintf(stderr, "\n");
  }

#endif

 private:
  void construct();
  int32_t getChildId(st::InternalNode *node, char c);
  void deleteTree(st::Node *node);

  size_t writeNode(std::ostream& out, st::Node* node);
  st::Node *readNode(std::istream& in, size_t *in_size);

  st::InternalNode *root_;
  const char* input_;
  size_t size_;
};

class CompactSuffixTree {
 public:
  CompactSuffixTree();
  CompactSuffixTree(const char *input, uint32_t size);
  CompactSuffixTree(const std::string& input);
  ~CompactSuffixTree();

  st::CompactNode* walkTree(const std::string& query);
  void getOffsets(std::vector<int64_t>& results, st::CompactNode* node);
  int64_t countLeaves(st::CompactNode* node);

  char charAt(uint64_t i) const;

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

 private:
  int32_t getChildId(st::CompactInternalNode *node, char c);
  void deleteTree(st::CompactNode *node);

  size_t writeNode(std::ostream& out, st::CompactNode* node);
  st::CompactNode *readNode(std::istream& in, size_t *in_size);

  st::CompactInternalNode* root_;
  const char* input_;
  uint32_t size_;

  uint64_t num_internal_nodes_;
  uint64_t num_leaf_nodes_;
};

}

#endif // DSL_SUFFIX_TREE_H_
