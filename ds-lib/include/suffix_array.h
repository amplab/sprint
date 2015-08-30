#ifndef DSL_SUFFIX_ARRAY_H_
#define DSL_SUFFIX_ARRAY_H_

#include "bitmap_array.h"

namespace dsl {

class SuffixArray : public BitmapArray {
 public:
  SuffixArray();
  SuffixArray(const std::string& input);
  SuffixArray(const char* input, size_t size);
  SuffixArray(uint64_t* suffix_array, size_t size);
};

}

#endif // DSL_SUFFIX_ARRAY_H_
