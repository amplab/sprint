#include "bitmap_array.h"

#include <cassert>

dsl::BitmapArray::BitmapArray()
    : Bitmap() {
  num_elements_ = 0;
  bit_width_ = 0;
}

dsl::BitmapArray::BitmapArray(uint64_t num_elements, uint8_t bit_width)
    : Bitmap(num_elements * bit_width) {
  num_elements_ = num_elements;
  bit_width_ = bit_width;
}

dsl::BitmapArray::BitmapArray(uint64_t *elements, uint64_t num_elements,
                              uint8_t bit_width)
    : Bitmap(num_elements * bit_width) {
  num_elements_ = num_elements;
  bit_width_ = bit_width;

  for (uint64_t i = 0; i < num_elements_; i++) {
    insert(i, elements[i]);
  }
}

dsl::BitmapArray::~BitmapArray() {
}

void dsl::BitmapArray::insert(uint64_t i, uint64_t value) {
  uint64_t s = i * bit_width_, e = i * bit_width_ + (bit_width_ - 1);
  if ((s / 64) == (e / 64)) {
    data_[s / 64] |= (value << (63 - e % 64));
  } else {
    data_[s / 64] |= (value >> (e % 64 + 1));
    data_[e / 64] |= (value << (63 - e % 64));
  }
}

uint64_t dsl::BitmapArray::at(uint64_t i) {
  uint64_t val;
  assert(i >= 0);
  uint64_t s = i * bit_width_, e = i * bit_width_ + (bit_width_ - 1);
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

uint64_t dsl::BitmapArray::operator [](uint64_t i) {
  return at(i);
}

size_t dsl::BitmapArray::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&num_elements_), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  out.write(reinterpret_cast<const char *>(&bit_width_), sizeof(uint8_t));
  out_size += sizeof(uint8_t);

  out_size += Bitmap::serialize(out);

  return out_size;
}

size_t dsl::BitmapArray::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&num_elements_), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  in.read(reinterpret_cast<char *>(&bit_width_), sizeof(uint8_t));
  in_size += sizeof(uint8_t);

  in_size += Bitmap::deserialize(in);

  return in_size;
}
