#ifndef DSL_TEXT_SUFFIX_ARRAY_INDEX_H_
#define DSL_TEXT_SUFFIX_ARRAY_INDEX_H_

#include "text/text_index.h"
#include "suffix_array.h"

namespace dsl {
class SuffixArrayIndex : public TextIndex {
 public:
  SuffixArrayIndex();
  SuffixArrayIndex(const std::string& input);
  SuffixArrayIndex(const std::string& input, SuffixArray* suffix_array);
  SuffixArrayIndex(const char* input, size_t size);
  SuffixArrayIndex(const char* input, size_t size, SuffixArray* suffix_array);

  void search(std::vector<int64_t>& results, const std::string& query) const;
  int64_t count(const std::string& query) const;
  bool contains(const std::string& query) const;

  char charAt(uint64_t i) const;

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

 protected:
  virtual std::pair<int64_t, int64_t> getRange(const std::string& query) const;
  int32_t compare(const std::string& query, uint64_t pos) const;

  SuffixArray *sa_;
  const char* input_;
  size_t size_;
};

class AugmentedSuffixArrayIndex : public SuffixArrayIndex {
 public:
  AugmentedSuffixArrayIndex();
  AugmentedSuffixArrayIndex(const std::string& input);
  AugmentedSuffixArrayIndex(const std::string& input, SuffixArray* suffix_array,
                            BitmapArray* lcp_l, BitmapArray* lcp_r);
  AugmentedSuffixArrayIndex(const char* input, size_t size);
  AugmentedSuffixArrayIndex(const char* input, size_t size,
                            SuffixArray* suffix_array, BitmapArray* lcp_l,
                            BitmapArray* lcp_r);

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

 private:
  void constructLcp();
  uint64_t precomputeLcp(uint64_t *lcp, uint64_t *lcp_l, uint64_t *lcp_r,
                         uint64_t l, uint64_t r);
  std::pair<int64_t, int64_t> getRange(const std::string& query) const;

  uint64_t lcpStr(const std::string& query, uint64_t i) const;
  int64_t getFirstOccurrence(const std::string& query) const;

  BitmapArray *lcp_l_;
  BitmapArray *lcp_r_;

};

}

#endif // DSL_TEXT_SUFFIX_ARRAY_INDEX_H_
