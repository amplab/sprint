#ifndef PULL_STAR_REGEX_H_
#define PULL_STAR_REGEX_H_

#include <set>

#include "text/text_index.h"
#include "regex_types.h"

namespace pull_star {

class RegularExpression {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResults;
  typedef RegExResults::iterator RegExResultsIterator;
  typedef enum {
    BlackBox = 0,
    PullStar = 1
  } ExecutorType;

  RegularExpression(std::string regex, dsl::TextIndex *text_idx,
                    ExecutorType ex_type = ExecutorType::PullStar);

  void execute();
  void subQuery(RegExResults &result, RegEx *r);
  void explain();
  void showResults(size_t limit);
  void getResults(RegExResults &results);

 private:
  void wildCard(RegExResults &left, RegExResults &right);
  void explainSubExpression(RegEx *re);
  void parse();

  std::string regex_;
  std::vector<RegEx *> sub_expressions_;
  dsl::TextIndex *text_idx_;
  ExecutorType ex_type_;

  RegExResults r_results;
};

}

#endif // PULL_STAR_REGEX_H_
