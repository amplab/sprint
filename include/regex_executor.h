#ifndef PULL_STAR_REGEX_EXECUTOR_H_
#define PULL_STAR_REGEX_EXECUTOR_H_

#include <set>

#include "text_index.h"
#include "regex_types.h"

namespace pull_star {
class RegExExecutor {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResult;
  typedef RegExResult::iterator RegExResultIterator;

  RegExExecutor(dsl::TextIndex *text_idx, pull_star::RegEx *regex);

  virtual ~RegExExecutor();

  virtual void execute() = 0;

  virtual void getResults(RegExResult &result);

 protected:
  dsl::TextIndex *text_idx_;
  RegEx *regex_;
  std::set<OffsetLength> final_result_;
};

class BBExecutor : public RegExExecutor {
 public:
  BBExecutor(dsl::TextIndex *text_idx, pull_star::RegEx* regex);

  void execute();
 private:
  void compute(RegExResult& result, RegEx* regex);
  void regexMgram(RegExResult& result, RegExPrimitive* regex);
  void regexUnion(RegExResult& union_result, RegExResult& first,
                  RegExResult& second);
  void regexConcat(RegExResult& concat_result, RegExResult& left,
                   RegExResult& right);
  void regexRepeat(RegExResult& repeat_result, RegExResult& internal,
                   RegExRepeatType repeat_type, int min = -1, int max = -1);
};

}

#endif /* PULL_STAR_REGEX_EXECUTOR_H_ */
