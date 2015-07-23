#include "regex.h"

#include <iostream>

#include "regex_utils.h"
#include "regex_parser.h"
#include "regex_executor.h"

#define BB_PARTIAL_SCAN

pull_star::RegularExpression::RegularExpression(std::string regex,
                                                dsl::TextIndex *text_idx,
                                                ExecutorType ex_type) {
  this->regex_ = regex;
  this->text_idx_ = text_idx;
  this->ex_type_ = ex_type;
  getSubexpressions();
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

void pull_star::RegularExpression::subQuery(RegExResults &result,
                                            std::string& sub_expression) {
  if (ex_type_ == ExecutorType::BlackBox) {
#ifdef BB_PARTIAL_SCAN
    std::vector<std::string> sub_sub_expressions;
    std::string sub_sub_expression = "";
    size_t i = 0;
    while (i < sub_expression.length()) {
      if (sub_expression[i] == '[') {
        if (sub_sub_expression != "") {
          sub_sub_expressions.push_back(sub_sub_expression);
          sub_sub_expression = "";
        }
        std::string range = "";
        for (; sub_expression[i] != ']'; i++) {
          if (sub_expression[i] == '-') {
            for (char c = sub_expression[i - 1] + 1; c < sub_expression[i + 1];
                c++) {
              range += c;
            }
            i++;
          }
          range += sub_expression[i];
        }
        range += sub_expression[i++];
        if (sub_expression[i] == '+' || sub_expression[i] == '*') {
          range += sub_expression[i++];
        }
        sub_sub_expressions.push_back(range);
      } else if (sub_expression[i] == '.') {
        if (sub_sub_expression != "") {
          sub_sub_expressions.push_back(sub_sub_expression);
          sub_sub_expression = "";
        }
        sub_sub_expressions.push_back(".");
        i++;
      } else {
        sub_sub_expression += sub_expression[i];
        i++;
      }
    }

    if (sub_sub_expression != "") {
      sub_sub_expressions.push_back(sub_sub_expression);
    }

    // Sequentially go through the list of sub-sub-expressions
    std::string last_token = "";
    int32_t last_token_id = -1;
    RegExResults last_results;
    for (size_t i = 0; i < sub_sub_expressions.size(); i++) {
      std::string ssexp = sub_sub_expressions[i];
      if (ssexp[0] == '[' || ssexp[0] == '.') {
        if(last_token_id == -1) {
          continue;
        }
        RegExResults range_results;
        if (ssexp == ".") {
          for (RegExResultsIterator it = last_results.begin();
              it != last_results.end(); it++) {
            range_results.insert(OffsetLength(it->first, it->second + 1));
          }
        } else if(ssexp[ssexp.length() - 1] == '+') {
          std::string range = ssexp.substr(1, ssexp.length() - 3);
          for (RegExResultsIterator it = last_results.begin();
                        it != last_results.end(); it++) {
            size_t start_pos = it->first + it->second - 1;
            char c;
            size_t len = 1;
            while(true) {
              c = text_idx_->charAt(start_pos + len);
              if (range.find(c) != std::string::npos) {
                range_results.insert(OffsetLength(it->first, it->second + len));
              } else {
                break;
              }
              len++;
            }
          }
        } else if(ssexp[ssexp.length() - 1] == '+') {
          std::string range = ssexp.substr(1, ssexp.length() - 3);
          range_results.insert(last_results.begin(), last_results.end());
          for (RegExResultsIterator it = last_results.begin();
                        it != last_results.end(); it++) {
            size_t start_pos = it->first + it->second - 1;
            char c;
            size_t len = 1;
            while(true) {
              c = text_idx_->charAt(start_pos + len);
              if (range.find(c) != std::string::npos) {
                range_results.insert(OffsetLength(it->first, it->second + len));
              } else {
                break;
              }
              len++;
            }
          }
        } else {
          std::string range = ssexp.substr(1, ssexp.length() - 2);
          for (RegExResultsIterator it = last_results.begin();
              it != last_results.end(); it++) {
            size_t cur_pos = it->first + it->second;
            char c = text_idx_->charAt(cur_pos);
            if (range.find(c) != std::string::npos) {
              range_results.insert(OffsetLength(it->first, it->second + 1));
            }
          }
        }
        last_results = range_results;
      } else {

        bool backtrack = false;
        if (last_token_id == -1) {
          backtrack = true;
        }

        last_token_id = i;
        last_token = ssexp;

        RegExResults cur_results;
        RegExParser p((char *) ssexp.c_str());
        RegEx *r = p.parse();
        BBExecutor executor(text_idx_, r);
        executor.execute();
        executor.getResults(cur_results);

        if (backtrack) {
          last_results = cur_results;
          for (int32_t j = i - 1; j >= 0; j--) {
            ssexp = sub_sub_expressions[j];
            if (ssexp[ssexp.length() - 1] == '*')
              continue;

            RegExResults range_results;
            if (ssexp == ".") {
              for (RegExResultsIterator it = last_results.begin();
                  it != last_results.end(); it++) {
                range_results.insert(
                    OffsetLength(it->first - 1, it->second + 1));
              }
            } else if(ssexp[ssexp.length() - 1] == '+') {
              std::string range = ssexp.substr(1, ssexp.length() - 2);
              for (RegExResultsIterator it = last_results.begin(); it != last_results.end(); it++) {
                size_t cur_pos = it->first - 1;
                char c = text_idx_->charAt(cur_pos);
                if (range.find(c) != std::string::npos) {
                  range_results.insert(
                      OffsetLength(it->first - 1, it->second + 1));
                }
              }
            } else if(ssexp[ssexp.length() - 1] == '*') {
              std::string range = ssexp.substr(1, ssexp.length() - 2);
              range_results.insert(last_results.begin(), last_results.end());
              for (RegExResultsIterator it = last_results.begin(); it != last_results.end(); it++) {
                size_t cur_pos = it->first - 1;
                char c = text_idx_->charAt(cur_pos);
                if (range.find(c) != std::string::npos) {
                  range_results.insert(
                      OffsetLength(it->first - 1, it->second + 1));
                }
              }
            } else {
              std::string range = ssexp.substr(1, ssexp.length() - 2);
              for (RegExResultsIterator it = last_results.begin();
                  it != last_results.end(); it++) {
                size_t cur_pos = it->first - 1;
                char c = text_idx_->charAt(cur_pos);
                if (range.find(c) != std::string::npos) {
                  range_results.insert(
                      OffsetLength(it->first - 1, it->second + 1));
                }
              }
            }
            last_results = range_results;
          }
        } else {
          RegExResults concat_results;
          RegExResultsIterator left_it, right_it;
          for (left_it = last_results.begin(), right_it = cur_results.begin();
              left_it != last_results.end() && right_it != cur_results.end();
              left_it++) {
            while (right_it != cur_results.end()
                && right_it->first < left_it->first + left_it->second)
              right_it++;
            if (right_it == cur_results.end())
              break;

            if (right_it->first == left_it->first + left_it->second) {
              concat_results.insert(
                  OffsetLength(left_it->first,
                               left_it->second + right_it->second));
            }
          }
          last_results = concat_results;
        }
      }
    }
    result = last_results;
#else
    RegExParser p((char *) sub_expression.c_str());
    RegEx *r = p.parse();
    BBExecutor executor(text_idx_, r);
    executor.execute();
    executor.getResults(result);
#endif
  } else {
    RegExParser p((char *) sub_expression.c_str());
    RegEx *r = p.parse();
    if (isSuffixed(r) || !isPrefixed(r)) {
      PSBwdExecutor executor(text_idx_, r);
      executor.execute();
      executor.getResults(result);
    } else {
      PSFwdExecutor executor(text_idx_, r);
      executor.execute();
      executor.getResults(result);
    }
  }
}

void pull_star::RegularExpression::explain() {
  if (ex_type_ == ExecutorType::BlackBox)
    return;
  fprintf(stderr, "***");
  for (auto subexp : sub_expressions_) {
    RegExParser p((char*) subexp.c_str());
    RegEx *r = p.parse();
    explainSubExpression(r);
    fprintf(stderr, "***");
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

void pull_star::RegularExpression::getSubexpressions() {
  // TODO: Right now this assumes we don't have nested ".*" operators
  // It would be nice to allow .* anywhere.
  Utils::split(sub_expressions_, regex_, ".*");
}

bool pull_star::RegularExpression::isPrefixed(RegEx* re) {
  switch (re->getType()) {
    case RegExType::Blank:
      return false;
    case RegExType::Primitive:
      return (((RegExPrimitive *) re)->getPrimitiveType()
          == RegExPrimitiveType::Mgram);
    case RegExType::Repeat:
      return isPrefixed(((RegExRepeat *) re)->getInternal());
    case RegExType::Concat:
      return isPrefixed(((RegExConcat *) re)->getLeft());
    case RegExType::Union:
      return isPrefixed(((RegExUnion *) re)->getFirst())
          && isPrefixed(((RegExUnion *) re)->getSecond());
  }
}

bool pull_star::RegularExpression::isSuffixed(RegEx* re) {
  switch (re->getType()) {
    case RegExType::Blank:
      return false;
    case RegExType::Primitive:
      return (((RegExPrimitive *) re)->getPrimitiveType()
          == RegExPrimitiveType::Mgram);
    case RegExType::Repeat:
      return isSuffixed(((RegExRepeat *) re)->getInternal());
    case RegExType::Concat:
      return isSuffixed(((RegExConcat *) re)->getRight());
    case RegExType::Union:
      return isSuffixed(((RegExUnion *) re)->getFirst())
          && isPrefixed(((RegExUnion *) re)->getSecond());
  }
}
