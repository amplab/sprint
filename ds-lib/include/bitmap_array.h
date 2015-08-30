#ifndef DSL_BITMAP_ARRAY_H_
#define DSL_BITMAP_ARRAY_H_

#include "bitmap.h"

namespace dsl {

class BitmapArray : public Bitmap {
 public:
  BitmapArray();
  virtual ~BitmapArray();
  BitmapArray(uint64_t num_elements, uint8_t bit_width);
  BitmapArray(uint64_t *elements, uint64_t num_elements, uint8_t bit_width);

  void insert(uint64_t i, uint64_t value);
  uint64_t at(uint64_t i);
  uint64_t operator[](uint64_t i);

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

  uint64_t num_elements_;
  uint8_t bit_width_;
};

}

#endif
