#include "text/suffix_array_index.h"

#include <math.h>
#include <climits>
#include <iostream>

#include "utils.h"

dsl::SuffixArrayIndex::SuffixArrayIndex() {
  sa_ = NULL;
  input_ = NULL;
  size_ = 0;
}

dsl::SuffixArrayIndex::SuffixArrayIndex(const char *input, size_t size,
                                        SuffixArray* suffix_array) {
  sa_ = suffix_array;
  input_ = input;
  size_ = size;
}

dsl::SuffixArrayIndex::SuffixArrayIndex(const char *input, size_t size)
    : SuffixArrayIndex(input, size, new dsl::SuffixArray(input, size)) {
}

dsl::SuffixArrayIndex::SuffixArrayIndex(const std::string& input,
                                        SuffixArray* suffix_array)
    : SuffixArrayIndex(input.c_str(), input.length() + 1, suffix_array) {

}

dsl::SuffixArrayIndex::SuffixArrayIndex(const std::string& input)
    : SuffixArrayIndex(input.c_str(), input.length() + 1) {
}

int32_t dsl::SuffixArrayIndex::compare(const std::string& query,
                                       uint64_t pos) const {
  for (uint64_t i = pos, q_pos = 0; i < pos + query.length(); i++, q_pos++) {
    if (input_[i % size_] != query[q_pos])
      return query[q_pos] - input_[i % size_];
  }
  return 0;
}

std::pair<int64_t, int64_t> dsl::SuffixArrayIndex::getRange(
    const std::string& query) const {
  int64_t st = size_ - 1;
  int64_t sp = 0;
  int64_t s;
  while (sp < st) {
    s = (sp + st) / 2;
    if (compare(query, sa_->at(s)) > 0)
      sp = s + 1;
    else
      st = s;
  }

  int64_t et = size_ - 1;
  int64_t ep = sp - 1;
  int64_t e;

  while (ep < et) {
    e = ceil((double) (ep + et) / 2);
    if (compare(query, sa_->at(e)) == 0)
      ep = e;
    else
      et = e - 1;
  }

  return std::pair<int64_t, int64_t>(sp, ep);
}

void dsl::SuffixArrayIndex::search(std::vector<int64_t>& results,
                                   const std::string& query) const {
  std::pair<int64_t, int64_t> range = getRange(query);
  if (range.second < range.first) {
    return;
  }

  for (uint64_t i = range.first; i <= range.second; i++) {
    results.push_back(sa_->at(i));
  }
}

int64_t dsl::SuffixArrayIndex::count(const std::string& query) const {
  std::pair<int64_t, int64_t> range = getRange(query);
  return range.second - range.first + 1;
}

bool dsl::SuffixArrayIndex::contains(const std::string& query) const {
  std::pair<int64_t, int64_t> range = getRange(query);
  return (range.second >= range.first);
}

char dsl::SuffixArrayIndex::charAt(uint64_t i) const {
  return input_[i];
}

size_t dsl::SuffixArrayIndex::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&size_), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  out.write(reinterpret_cast<const char *>(input_), size_ * sizeof(char));
  out_size += size_ * sizeof(char);

  out_size += sa_->serialize(out);

  return out_size;
}

size_t dsl::SuffixArrayIndex::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&size_), sizeof(uint64_t));
  in_size += sizeof(uint64_t);
  char *input = new char[size_];
  in.read(reinterpret_cast<char *>(input), size_ * sizeof(char));
  in_size += size_ * sizeof(char);

  input_ = input;

  sa_ = new dsl::SuffixArray();
  in_size += sa_->deserialize(in);

  return in_size;
}

dsl::AugmentedSuffixArrayIndex::AugmentedSuffixArrayIndex()
    : SuffixArrayIndex() {
  lcp_l_ = NULL;
  lcp_r_ = NULL;
}

dsl::AugmentedSuffixArrayIndex::AugmentedSuffixArrayIndex(
    const char* input, size_t size, SuffixArray* suffix_array,
    BitmapArray* lcp_l, BitmapArray *lcp_r)
    : SuffixArrayIndex(input, size, suffix_array) {
  lcp_l_ = lcp_l;
  lcp_r_ = lcp_r;
}

dsl::AugmentedSuffixArrayIndex::AugmentedSuffixArrayIndex(const char* input,
                                                          size_t size)
    : SuffixArrayIndex(input, size) {
  constructLcp();
}

dsl::AugmentedSuffixArrayIndex::AugmentedSuffixArrayIndex(
    const std::string& input, SuffixArray* suffix_array, BitmapArray* lcp_l,
    BitmapArray* lcp_r)
    : AugmentedSuffixArrayIndex(input.c_str(), input.length() + 1, suffix_array,
                                lcp_l, lcp_r) {
}

dsl::AugmentedSuffixArrayIndex::AugmentedSuffixArrayIndex(
    const std::string& input)
    : AugmentedSuffixArrayIndex(input.c_str(), input.length() + 1) {
}

void dsl::AugmentedSuffixArrayIndex::constructLcp() {

  uint64_t N = size_;

  // Populate inverse suffix array
  uint64_t *isa = new uint64_t[N];
  for (uint64_t i = 0; i < N; i++) {
    isa[sa_->at(i)] = i;
  }

  // Populate the LCP array
  uint64_t *lcp = new uint64_t[N]();
  uint64_t lcp_val = 0;
  uint64_t max_lcp_val = 0;
  for (uint64_t i = 0; i < N - 1; i++) {
    uint64_t pos = isa[i];
    uint64_t j = sa_->at(pos - 1);
    while (i + lcp_val < N - 1 && j + lcp_val < N - 1
        && input_[i + lcp_val] == input_[j + lcp_val]) {
      lcp_val++;
    }
    if (pos != 0) {
      lcp[pos - 1] = lcp_val;
      if (lcp_val > max_lcp_val)
        max_lcp_val = lcp_val;
    }
    if (lcp_val > 0)
      lcp_val--;
  }
  delete[] isa;

  // Populate the LCP-L and LCP-R arrays
  uint64_t *lcp_l = new uint64_t[N - 1];
  uint64_t *lcp_r = new uint64_t[N - 1];

  precomputeLcp(lcp, lcp_l, lcp_r, 0, N);
  delete[] lcp;

  lcp_l_ = new BitmapArray(lcp_l, N - 1, Utils::int_log_2(max_lcp_val + 1));
  delete[] lcp_l;

  lcp_r_ = new BitmapArray(lcp_r, N - 1, Utils::int_log_2(max_lcp_val + 1));
  delete[] lcp_r;
}

uint64_t dsl::AugmentedSuffixArrayIndex::precomputeLcp(uint64_t *lcp,
                                                       uint64_t *lcp_l,
                                                       uint64_t *lcp_r,
                                                       uint64_t l, uint64_t r) {
  if (l == r - 1) {
    return lcp[l];
  }

  uint64_t c = (l + r) / 2;

  lcp_l[c - 1] = precomputeLcp(lcp, lcp_l, lcp_r, l, c);
  lcp_r[c - 1] = precomputeLcp(lcp, lcp_l, lcp_r, c, r);
  return MIN(lcp_l[c - 1], lcp_r[c - 1]);
}

uint64_t dsl::AugmentedSuffixArrayIndex::lcpStr(const std::string& query,
                                                uint64_t i) const {
  for (uint64_t l = 0; l < query.length(); l++) {
    if (input_[(i + l) % size_] != query[l])
      return l;
  }
  return query.length();
}

int64_t dsl::AugmentedSuffixArrayIndex::getFirstOccurrence(
    const std::string& query) const {
  int64_t lp = 0;
  int64_t rp = size_;
  uint64_t l = lcpStr(query, sa_->at(lp));
  uint64_t r = lcpStr(query, sa_->at(rp));
  uint64_t m;

  while (rp - lp > 1) {
    int64_t mp = (lp + rp) / 2;
    if (l >= r) {
      if (lcp_l_->at(mp - 1) >= l) {
        m = l + lcpStr(query.substr(l), sa_->at(mp) + l);
      } else {
        m = lcp_l_->at(mp - 1);
      }
    } else {
      if (lcp_r_->at(mp - 1) >= r) {
        m = r + lcpStr(query.substr(r), sa_->at(mp) + r);
      } else {
        m = lcp_r_->at(mp - 1);
      }
    }
    if (m == query.length() || query[m] <= input_[sa_->at(mp) + m]) {
      rp = mp;
      r = m;
    } else {
      lp = mp;
      l = m;
    }
  }

  return rp;
}

std::pair<int64_t, int64_t> dsl::AugmentedSuffixArrayIndex::getRange(
    const std::string& query) const {

  int64_t sp = getFirstOccurrence(query);
  std::string end_query = std::string(query);
  end_query[end_query.length() - 1] = end_query[end_query.length() - 1] + 1;
  int64_t ep = getFirstOccurrence(end_query) - 1;

  return std::pair<int64_t, int64_t>(sp, ep);
}

size_t dsl::AugmentedSuffixArrayIndex::serialize(std::ostream& out) {
  size_t out_size = 0;

  out_size += SuffixArrayIndex::serialize(out);
  out_size += lcp_l_->serialize(out);
  out_size += lcp_r_->serialize(out);

  return out_size;
}

size_t dsl::AugmentedSuffixArrayIndex::deserialize(std::istream& in) {
  size_t in_size = 0;

  in_size += SuffixArrayIndex::deserialize(in);
  lcp_l_ = new BitmapArray();
  lcp_r_ = new BitmapArray();
  in_size += lcp_l_->deserialize(in);
  in_size += lcp_r_->deserialize(in);

  return in_size;
}
