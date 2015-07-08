#ifndef PULL_STAR_REGEX_H_
#define PULL_STAR_REGEX_H_

#include <set>

#include "regex_types.h"
#include "suffix_tree.h"

namespace pull_star {

class RegularExpression {
 public:
  typedef std::pair<size_t, size_t> OffsetLength;
  typedef std::set<OffsetLength> RegExResults;
  typedef RegExResults::iterator RegExResultsIterator;

  RegularExpression(std::string regex, dsl::SuffixTree *text_idx);

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
  dsl::SuffixTree *text_idx_;

  RegExResults r_results;
};

}

#endif // PULL_STAR_REGEX_H_
