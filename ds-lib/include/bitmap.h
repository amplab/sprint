#ifndef DSL_BITMAP_H_
#define DSL_BITMAP_H_

#include <cstdint>
#include <iostream>

#include "utils.h"

namespace dsl {

#define GETBITVAL(data, i) GETBIT((data)[(i) / 64], (i) % 64)
#define SETBITVAL(data, i) SETBIT((data)[(i) / 64], (i) % 64)
#define CLRBITVAL(data, i) CLRBIT((data)[(i) / 64], (i) % 64)

class Bitmap {
 public:
  Bitmap();
  Bitmap(uint64_t num_bits);
  ~Bitmap();

  void clear();

  void setBit(uint64_t i);
  void unsetBit(uint64_t i);
  bool getBit(uint64_t i);

  void putValPos(uint64_t pos, uint64_t val, uint8_t bits);
  uint64_t getValPos(uint64_t pos, uint8_t bits);

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

  uint64_t *data_;
  uint64_t size_;
};

}

#endif
