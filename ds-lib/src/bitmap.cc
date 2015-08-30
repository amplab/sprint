#include "bitmap.h"

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>

dsl::Bitmap::Bitmap() {
  data_ = NULL;
  size_ = 0;
}

dsl::Bitmap::Bitmap(uint64_t num_bits) {
  assert(num_bits > 0);
  data_ = new uint64_t[BITS2BLOCKS(num_bits)]();
  size_ = num_bits;
}

dsl::Bitmap::~Bitmap() {
  if(data_) {
    delete[] data_;
    data_ = NULL;
  }
}

void dsl::Bitmap::clear() {
  memset((void *) data_, 0, BITS2BLOCKS(size_) * sizeof(uint64_t));
}

void dsl::Bitmap::setBit(uint64_t i) {
  SETBITVAL(data_, i);
}

void dsl::Bitmap::unsetBit(uint64_t i) {
  CLRBITVAL(data_, i);
}

bool dsl::Bitmap::getBit(uint64_t i) {
  return GETBITVAL(data_, i);
}

void dsl::Bitmap::putValPos(uint64_t pos, uint64_t val, uint8_t bits) {
  uint64_t s = pos, e = pos + (bits - 1);
  if ((s / 64) == (e / 64)) {
    data_[s / 64] |= (val << (63 - e % 64));
  } else {
    data_[s / 64] |= (val >> (e % 64 + 1));
    data_[e / 64] |= (val << (63 - e % 64));
  }
}

uint64_t dsl::Bitmap::getValPos(uint64_t pos, uint8_t bits) {
  if (bits == 0)
    return 0;
  assert(pos >= 0);
  uint64_t val;
  uint64_t s = pos, e = pos + (bits - 1);
  if ((s / 64) == (e / 64)) {
    val = data_[s / 64] << (s % 64);
    val = val >> (63 - e % 64 + s % 64);
  } else {
    uint64_t val1 = data_[s / 64] << (s % 64);
    uint64_t val2 = data_[e / 64] >> (63 - e % 64);
    val1 = val1 >> (s % 64 - (e % 64 + 1));
    val = val1 | val2;
  }

  return val;
}

size_t dsl::Bitmap::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&size_), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  for (uint64_t i = 0; i < BITS2BLOCKS(size_); i++) {
    out.write(reinterpret_cast<const char *>(&data_[i]), sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  return out_size;
}

size_t dsl::Bitmap::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&size_), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  data_ = new uint64_t[BITS2BLOCKS(size_)];
  for (uint64_t i = 0; i < BITS2BLOCKS(size_); i++) {
    in.read(reinterpret_cast<char *>(&data_[i]), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
  }

  return in_size;
}
