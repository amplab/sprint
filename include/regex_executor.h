#ifndef PULL_STAR_REGEX_EXECUTOR_H_
#define PULL_STAR_REGEX_EXECUTOR_H_

#include <set>

#include "text/text_index.h"
#include "regex_types.h"

namespace pull_star {
class RegExExecutor {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResult;
  typedef RegExResult::iterator RegExResultIterator;

  RegExExecutor(const dsl::TextIndex *text_idx, pull_star::RegEx *regex);

  virtual ~RegExExecutor();

  virtual void execute() = 0;

  virtual void getResults(RegExResult &result);

 protected:
  const dsl::TextIndex *text_idx_;
  RegEx *regex_;
  std::set<OffsetLength> final_result_;
};

class BBExecutor : public RegExExecutor {
 public:
  BBExecutor(const dsl::TextIndex *text_idx, pull_star::RegEx* regex);

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

class PSExecutor : public RegExExecutor {
 public:
  typedef std::set<std::string> TokenSet;
  typedef std::string Token;
  typedef TokenSet::iterator ResultIterator;

  PSExecutor(const dsl::TextIndex* s_core, RegEx *re);

  void execute();

 protected:
  virtual void compute(TokenSet &tokens, RegEx *regex) = 0;

  void regexUnion(TokenSet &union_tokens, TokenSet first, TokenSet second);

  virtual void regexConcat(TokenSet &concat_tokens, RegEx *regex,
                           Token next_token) = 0;

  virtual void regexRepeatOneOrMore(TokenSet &repeat_tokens, RegEx *regex);

  virtual void regexRepeatOneOrMore(TokenSet &repeat_tokens, RegEx *regex,
                                    Token next_token);

  virtual void regexRepeatMinToMax(TokenSet &repeat_tokens, RegEx *regex,
                                   int min, int max);

  virtual void regexRepeatMinToMax(TokenSet &repeat_tokens, RegEx *regex,
                                   Token next_token, int min, int max);

  TokenSet tokens_;
};

class PSFwdExecutor : public PSExecutor {

 public:
  PSFwdExecutor(const dsl::TextIndex* s_core, RegEx *re);

 private:
  void compute(TokenSet &tokens, RegEx *regex);

  void regexConcat(TokenSet &concat_tokens, RegEx *regex, Token left_token);
};

class PSBwdExecutor : public PSExecutor {
 public:
  PSBwdExecutor(const dsl::TextIndex* s_core, RegEx *re);

 private:
  void compute(TokenSet &tokens, RegEx *regex);

  void regexConcat(TokenSet &concat_tokens, RegEx *regex, Token right_token);
};

}

#endif /* PULL_STAR_REGEX_EXECUTOR_H_ */
