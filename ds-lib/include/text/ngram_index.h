#ifndef DSL_TEXT_NGRAM_INDEX_H_
#define DSL_TEXT_NGRAM_INDEX_H_

#include <map>
#include <cstring>

#include "bitmap_array.h"
#include "text/text_index.h"

namespace dsl {

namespace ngram {
  struct NGramComparator {
    NGramComparator() {
      n_ = 3;
    }

    NGramComparator(uint32_t n) {
      n_ = n;
    }

    bool operator() (char *a, char *b) const {
      for(size_t i = 0; i < n_; i++) {
        if(a[i] < b[i]) return true;
        else if(a[i] > b[i]) return false;
      }
      return false;
    }

    uint32_t n_;
  };
};

class NGramIndex : public TextIndex {
public:
  typedef std::map<char*, BitmapArray*, ngram::NGramComparator> NGramMap;

  NGramIndex();
  NGramIndex(const std::string& input, uint32_t n = 3);
  NGramIndex(const char* input, size_t size, uint32_t n = 3);

  void search(std::vector<int64_t>& results, const std::string& query) const;
  int64_t count(const std::string& query) const;
  bool contains(const std::string& query) const;

  char charAt(uint64_t i) const;

  size_t serialize(std::ostream& out);
  size_t deserialize(std::istream& in);

private:
  void constructNGramIndex();
  bool match(const char* str1, const char* str2, size_t len) const;
  bool nGramStartsWith(char* ngram, char *substr) const;

  const char* input_;
  size_t size_;
  uint32_t n_;
  NGramMap map_;
};
}

#endif
