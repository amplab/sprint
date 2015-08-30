#include "text/ngram_index.h"

#include <cassert>

dsl::NGramIndex::NGramIndex() {
  input_ = NULL;
  size_ = 0;
  n_ = 0;
}

dsl::NGramIndex::NGramIndex(const char *input, size_t size, uint32_t n) {
  input_ = input;
  size_ = size;
  n_ = n;
  constructNGramIndex();
}

dsl::NGramIndex::NGramIndex(const std::string& input, uint32_t n)
    : NGramIndex(input.c_str(), input.length(), n) {
}

void dsl::NGramIndex::constructNGramIndex() {
  std::map<char *, std::vector<uint64_t>, ngram::NGramComparator> ngram_map(
      (ngram::NGramComparator(n_)));
  for (size_t i = 0; i < size_ - n_ + 1; i++) {
    char* ngram = strndup(input_ + i, n_);
    assert(ngram != NULL);
    ngram_map[ngram].push_back(i);
  }

  uint8_t num_bits = Utils::int_log_2(size_ + 1);
  map_ = NGramMap((ngram::NGramComparator(n_)));
  for (auto& entry : ngram_map) {
    map_[entry.first] = new BitmapArray(&entry.second[0], entry.second.size(), num_bits);
  }
  ngram_map.clear();

#ifdef DEBUG_CONSTRUCT
  for (auto& entry : map_) {
    fprintf(stderr, "[%s]: ", entry.first);
    for (uint64_t i = 0; i < entry.second->num_elements_; i++) {
      fprintf(stderr, "%llu, ", entry.second->at(i));
    }
    fprintf(stderr, "\n");
  }
#endif
}

bool dsl::NGramIndex::match(const char* str1, const char* str2, size_t len) const {
  size_t pos = 0;
  while(pos < len) {
    if(str1[pos] != str2[pos]) return false;
    pos++;
  }
  return true;
}

bool dsl::NGramIndex::nGramStartsWith(char* ngram, char *substr) const {
  size_t pos = 0;
#ifdef DEBUG_QUERY
  fprintf(stderr, "%s starts with %s?\n", ngram, substr);
#endif
  while (substr[pos] != '\0') {
    if (substr[pos] != ngram[pos])
      return false;
    pos++;
  }
  return true;
}

void dsl::NGramIndex::search(std::vector<int64_t>& results,
                             const std::string& query) const {
  char* query_str = (char *) query.c_str();
  size_t query_len = query.length();
  if (query_len == n_) {
    // Exact match, return matched results
    if (map_.find(query_str) == map_.end())
      return;
    BitmapArray *res_exact = map_.at(query_str);
    for (uint64_t i = 0; i < res_exact->num_elements_; i++) {
      results.push_back(res_exact->at(i));
    }
  } else if (query_len < n_) {
    // Query is smaller than n, aggregate results from multiple entries
    auto it = map_.lower_bound(query_str);
    while (it != map_.end() && nGramStartsWith(it->first, query_str)) {
      BitmapArray *res_partial = it->second;
      for (uint64_t i = 0; i < res_partial->num_elements_; i++) {
        results.push_back(res_partial->at(i));
      }
      it++;
    }
  } else {
    // Query is larger than n, must filter out results from ngram index
    if (map_.find(query_str) == map_.end())
      return;
    size_t len_extra = strlen(query_str);
    BitmapArray *res_extra = map_.at(query_str);
    for (uint64_t i = 0; i < res_extra->num_elements_; i++) {
      uint64_t offset = res_extra->at(i);
      if (match(query_str + n_, input_ + offset + n_, query_len - n_))
        results.push_back(offset);
    }
  }
}

int64_t dsl::NGramIndex::count(const std::string& query) const {
  char* query_str = (char *) query.c_str();
  size_t query_len = query.length();
  int64_t count;
  if (query_len == n_) {
    // Exact match, return matched results
    if (map_.find(query_str) == map_.end())
      return 0;
    BitmapArray *res_exact = map_.at(query_str);
    count = res_exact->num_elements_;
  } else if (query_len < n_) {
    // Query is smaller than n, aggregate results from multiple entries
    auto it = map_.lower_bound(query_str);
    while (nGramStartsWith(it->first, query_str)) {
      BitmapArray *res_partial = it->second;
      count += res_partial->num_elements_;
      it++;
    }
  } else {
    // Query is larger than n, must filter out results from ngram index
    if (map_.find(query_str) == map_.end())
      return 0;
    size_t len_extra = strlen(query_str);
    BitmapArray *res_extra = map_.at(query_str);
    for (uint64_t i = 0; i < res_extra->num_elements_; i++) {
      uint64_t offset = res_extra->at(i);
      if (std::strncmp(query_str + n_, input_ + offset + n_, query_len - n_)
          == 0)
        count++;
    }
  }
  return count;
}

bool dsl::NGramIndex::contains(const std::string& query) const {
  char* query_str = (char *) query.c_str();
  size_t query_len = query.length();
  if (query_len >= n_) {
    return map_.find(query_str) != map_.end();
  }
  auto it = map_.lower_bound(query_str);
  return nGramStartsWith(it->first, query_str);
}

char dsl::NGramIndex::charAt(uint64_t i) const {
  return input_[i];
}

size_t dsl::NGramIndex::serialize(std::ostream& out) {
  size_t out_size = 0;

  out.write(reinterpret_cast<const char *>(&size_), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  out.write(reinterpret_cast<const char *>(input_), size_ * sizeof(char));
  out_size += size_ * sizeof(char);

  out.write(reinterpret_cast<const char *>(&n_), sizeof(uint32_t));
  out_size += sizeof(uint32_t);

  size_t map_size = map_.size();
  out.write(reinterpret_cast<const char *>(&map_size), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  for (auto& entry : map_) {
    out.write(reinterpret_cast<const char *>(entry.first), n_ * sizeof(char));
    out_size += n_ * sizeof(char);

    out_size += entry.second->serialize(out);
  }

  return out_size;
}

size_t dsl::NGramIndex::deserialize(std::istream& in) {
  size_t in_size = 0;

  in.read(reinterpret_cast<char *>(&size_), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  char *input = new char[size_];
  in.read(reinterpret_cast<char *>(input), size_ * sizeof(char));
  in_size += size_ * sizeof(char);
  input_ = input;

  in.read(reinterpret_cast<char *>(&n_), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  size_t map_size;
  in.read(reinterpret_cast<char *>(&map_size), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  map_ = NGramMap((ngram::NGramComparator(n_)));
  for (size_t i = 0; i < map_size; i++) {
    char *ngram = new char[n_];
    in.read(reinterpret_cast<char *>(ngram), n_ * sizeof(char));
    in_size += n_ * sizeof(char);

    BitmapArray *offsets = new BitmapArray();
    in_size += offsets->deserialize(in);

    map_[ngram] = offsets;
  }

#ifdef DEBUG_CONSTRUCT
  for (auto& entry : map_) {
    fprintf(stderr, "[%s]: ", entry.first);
    for (uint64_t i = 0; i < entry.second->num_elements_; i++) {
      fprintf(stderr, "%llu, ", entry.second->at(i));
    }
    fprintf(stderr, "\n");
  }
#endif

  return in_size;
}
