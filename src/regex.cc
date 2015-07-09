#include "regex.h"

#include <iostream>

#include "regex_parser.h"
#include "regex_executor.h"

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

pull_star::RegularExpression::RegularExpression(std::string regex,
                                                dsl::TextIndex *text_idx) {
  this->regex_ = regex;
  this->text_idx_ = text_idx;
  parse();
}

void pull_star::RegularExpression::execute() {
  std::vector<RegExResults> subresults;
  for (auto subexp : sub_expressions_) {
    RegExResults subresult;
    subQuery(subresult, subexp);
    subresults.push_back(subresult);
  }

  while (subresults.size() != 1) {
    wildCard(subresults[0], subresults[1]);
    subresults.erase(subresults.begin());
  }

  r_results = subresults[0];
}

void pull_star::RegularExpression::wildCard(RegExResults &left,
                                            RegExResults &right) {
  RegExResults wildcard_res;
  RegExResultsIterator left_it, right_it;
  for (left_it = left.begin(); left_it != left.end(); left_it++) {
    OffsetLength search_candidate(left_it->first + left_it->second, 0);
    RegExResultsIterator first_entry = right.lower_bound(search_candidate);
    for (right_it = first_entry; right_it != right.end(); right_it++) {
      size_t offset = left_it->first;
      size_t length = right_it->first - left_it->first + right_it->second;
      wildcard_res.insert(OffsetLength(offset, length));
    }
  }
  right = wildcard_res;
}

void pull_star::RegularExpression::subQuery(RegExResults &result, RegEx *r) {
  BBExecutor executor(text_idx_, r);
  executor.execute();
  executor.getResults(result);
}

void pull_star::RegularExpression::explain() {
  fprintf(stderr, "***");
  for (auto subexp : sub_expressions_) {
    explainSubExpression(subexp);
    fprintf(stderr, "***\n");
  }
}

void pull_star::RegularExpression::showResults(size_t limit) {
  if (limit <= 0)
    limit = r_results.size();
  limit = MIN(limit, r_results.size());
  RegExResultsIterator it;
  size_t i;
  fprintf(stdout, "Showing %zu of %zu results.\n", limit, r_results.size());
  fprintf(stdout, "{");
  for (it = r_results.begin(), i = 0; i < limit; i++, it++) {
    fprintf(stdout, "%zu => %zu, ", it->first, it->second);
  }
  fprintf(stdout, "...}\n");
}

void pull_star::RegularExpression::getResults(RegExResults &results) {
  results = r_results;
}

void pull_star::RegularExpression::explainSubExpression(RegEx *re) {
  switch (re->getType()) {
    case RegExType::Blank: {
      fprintf(stderr, "<blank>");
      break;
    }
    case RegExType::Primitive: {
      RegExPrimitive *p = ((RegExPrimitive *) re);
      fprintf(stderr, "\"%s\"", p->getPrimitive().c_str());
      break;
    }
    case RegExType::Repeat: {
      fprintf(stderr, "REPEAT(");
      explainSubExpression(((RegExRepeat *) re)->getInternal());
      fprintf(stderr, ")");
      break;
    }
    case RegExType::Concat: {
      fprintf(stderr, "(");
      explainSubExpression(((RegExConcat *) re)->getLeft());
      fprintf(stderr, " CONCAT ");
      explainSubExpression(((RegExConcat *) re)->getRight());
      fprintf(stderr, ")");
      break;
    }
    case RegExType::Union: {
      fprintf(stderr, "(");
      explainSubExpression(((RegExUnion *) re)->getFirst());
      fprintf(stderr, " OR ");
      explainSubExpression(((RegExUnion *) re)->getSecond());
      fprintf(stderr, ")");
      break;
    }
  }
}

void pull_star::RegularExpression::parse() {
  // TODO: Right now this assumes we don't have nested ".*" operators
  // It would be nice to allow .* anywhere.
  std::string delimiter = ".*";

  size_t pos = 0;
  std::string subexp;

  while ((pos = regex_.find(delimiter)) != std::string::npos) {
    subexp = regex_.substr(0, pos);
    RegExParser parser((char *) subexp.c_str());
    sub_expressions_.push_back(parser.parse());
    regex_.erase(0, pos + delimiter.length());
  }

  subexp = regex_;
  RegExParser parser((char *) subexp.c_str());
  sub_expressions_.push_back(parser.parse());
}
