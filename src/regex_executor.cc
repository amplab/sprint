#include "regex_executor.h"

#include <algorithm>

pull_star::RegExExecutor::RegExExecutor(const dsl::TextIndex *text_idx,
                                        RegEx *regex) {
  text_idx_ = text_idx;
  regex_ = regex;
}

pull_star::RegExExecutor::~RegExExecutor() {
}

void pull_star::RegExExecutor::getResults(RegExResult& result) {
  result = final_result_;
}

pull_star::BBExecutor::BBExecutor()
    : RegExExecutor(NULL, NULL) {
}

pull_star::BBExecutor::BBExecutor(const dsl::TextIndex *text_idx,
                                  pull_star::RegEx* regex)
    : RegExExecutor(text_idx, regex) {
}

void pull_star::BBExecutor::execute() {
  compute(final_result_, regex_);
}

void pull_star::BBExecutor::compute(RegExResult& result, RegEx* regex) {
  switch (regex->getType()) {
    case RegExType::Blank: {
      break;
    }
    case RegExType::Primitive: {
      switch (((RegExPrimitive *) regex)->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram: {
          regexMgram(result, (RegExPrimitive *) regex);
          break;
        }
        case RegExPrimitiveType::Range:
        case RegExPrimitiveType::Dot: {
          std::string primitive = ((RegExPrimitive *) regex)->getPrimitive();
          if (primitive == ".") {
            primitive = "";
            for (char c = 32; c < 127; c++) {
              if (c == '\n')
                continue;
              primitive += c;
            }
          }
          for (auto c : primitive) {
            RegExPrimitive char_primitive(std::string(1, c));
            regexMgram(result, &char_primitive);
          }
          break;
        }
      }
      break;
    }
    case RegExType::Union: {
      RegExResult first_res, second_res;
      compute(first_res, ((RegExUnion *) regex)->getFirst());
      compute(second_res, ((RegExUnion *) regex)->getSecond());
      regexUnion(result, first_res, second_res);
      break;
    }
    case RegExType::Concat: {
      RegExResult left_res, right_res;
      compute(left_res, ((RegExConcat *) regex)->getLeft());
      compute(right_res, ((RegExConcat *) regex)->getRight());
      regexConcat(result, left_res, right_res);
      break;
    }
    case RegExType::Repeat: {
      RegExResult internal_res;
      compute(internal_res, ((RegExRepeat *) regex)->getInternal());
      regexRepeat(result, internal_res,
                  ((RegExRepeat *) regex)->getRepeatType());
      break;
    }
  }
}

void pull_star::BBExecutor::regexMgram(RegExResult& result,
                                       RegExPrimitive* regex) {
  std::string mgram = regex->getPrimitive();
  std::vector<int64_t> offsets;
  text_idx_->search(offsets, mgram);
  for (auto offset : offsets) {
    result.insert(OffsetLength(offset, mgram.length()));
  }
}

void pull_star::BBExecutor::regexUnion(RegExResult& union_results,
                                       RegExResult& first,
                                       RegExResult& second) {
  std::set_union(first.begin(), first.end(), second.begin(), second.end(),
                 std::inserter(union_results, union_results.begin()));
}

void pull_star::BBExecutor::regexConcat(RegExResult& concat_result,
                                        RegExResult& left, RegExResult& right) {
  RegExResultIterator left_it, right_it;
  for (left_it = left.begin(), right_it = right.begin();
      left_it != left.end() && right_it != right.end(); left_it++) {
    while (right_it != right.end()
        && right_it->first < left_it->first + left_it->second)
      right_it++;
    if (right_it == right.end())
      break;

    if (right_it->first == left_it->first + left_it->second) {
      concat_result.insert(
          OffsetLength(left_it->first, left_it->second + right_it->second));
    }
  }
}

void pull_star::BBExecutor::regexRepeat(RegExResult& repeat_result,
                                        RegExResult& internal,
                                        RegExRepeatType repeat_type, int min,
                                        int max) {
  switch (repeat_type) {
    case RegExRepeatType::ZeroOrMore:
    case RegExRepeatType::OneOrMore: {
      // FIXME: .* is equivalent to .+ for now
      size_t concat_size;
      RegExResult concat_res;
      repeat_result = concat_res = internal;

      do {
        RegExResult concat_temp_res;
        regexConcat(concat_temp_res, concat_res, internal);
        concat_res = concat_temp_res;

        concat_size = concat_res.size();

        RegExResult repeat_temp_res;
        regexUnion(repeat_temp_res, repeat_result, concat_res);
        repeat_result = repeat_temp_res;
      } while (concat_size);
      break;
    }
    case RegExRepeatType::MinToMax: {
      size_t concat_size;
      RegExResult concat_res, min_res;
      min_res = concat_res = internal;
      size_t num_repeats = 1;

      // Get to min repeats
      while (num_repeats < min) {
        RegExResult concat_temp_res;
        regexConcat(concat_temp_res, concat_res, internal);
        concat_res = concat_temp_res;

        num_repeats++;

        if (concat_res.size() == 0)
          return;
      }

      do {
        RegExResult concat_temp_res;
        regexConcat(concat_temp_res, concat_res, internal);
        concat_res = concat_temp_res;

        concat_size = concat_res.size();

        RegExResult repeat_temp_res;
        regexUnion(repeat_temp_res, repeat_result, concat_res);
        repeat_result = repeat_temp_res;

        num_repeats++;
      } while (concat_size && num_repeats < max);

      break;
    }
  }
}

pull_star::PSExecutor::PSExecutor(const dsl::TextIndex* text_idx, RegEx* regex)
    : RegExExecutor(text_idx, regex) {
}

void pull_star::PSExecutor::execute() {
  compute(tokens_, regex_);
  for (Token token : tokens_) {
    std::vector<int64_t> results;
    text_idx_->search(results, token);
    for (int64_t offset : results) {
      final_result_.insert(OffsetLength(offset, token.length()));
    }
  }
}

void pull_star::PSExecutor::regexUnion(TokenSet &union_tokens, TokenSet first,
                                       TokenSet second) {
  std::set_union(first.begin(), first.end(), second.begin(), second.end(),
                 std::inserter(union_tokens, union_tokens.begin()));
}

void pull_star::PSExecutor::regexRepeatOneOrMore(TokenSet &repeat_tokens,
                                                 RegEx *regex) {
  TokenSet tokens;
  compute(tokens, regex);
  if (tokens.empty())
    return;
  regexUnion(repeat_tokens, repeat_tokens, tokens);

  for (auto token : tokens)
    regexRepeatOneOrMore(repeat_tokens, regex, token);
}

void pull_star::PSExecutor::regexRepeatOneOrMore(TokenSet &repeat_tokens,
                                                 RegEx *regex,
                                                 Token previous_token) {
  TokenSet concat_tokens;
  regexConcat(concat_tokens, regex, previous_token);
  if (concat_tokens.empty())
    return;
  regexUnion(repeat_tokens, repeat_tokens, concat_tokens);

  for (auto concat_token : concat_tokens)
    regexRepeatOneOrMore(repeat_tokens, regex, concat_token);
}

void pull_star::PSExecutor::regexRepeatMinToMax(TokenSet &repeat_tokens,
                                                RegEx *regex, int min,
                                                int max) {
  min = (min > 0) ? min - 1 : 0;
  max = (max > 0) ? max - 1 : 0;

  TokenSet tokens;
  compute(tokens, regex);
  if (tokens.empty())
    return;

  if (!min)
    regexUnion(repeat_tokens, repeat_tokens, tokens);

  if (max)
    for (auto token : tokens)
      regexRepeatMinToMax(repeat_tokens, regex, token, min, max);
}

void pull_star::PSExecutor::regexRepeatMinToMax(TokenSet &repeat_tokens,
                                                RegEx *regex,
                                                Token previous_token, int min,
                                                int max) {
  min = (min > 0) ? min - 1 : 0;
  max = (max > 0) ? max - 1 : 0;

  TokenSet concat_tokens;
  regexConcat(concat_tokens, regex, previous_token);
  if (concat_tokens.empty())
    return;

  if (!min)
    regexUnion(repeat_tokens, repeat_tokens, concat_tokens);

  if (max)
    for (auto concat_token : concat_tokens)
      regexRepeatMinToMax(repeat_tokens, regex, concat_token, min, max);
}

pull_star::PSFwdExecutor::PSFwdExecutor(const dsl::TextIndex* text_idx,
                                        RegEx* regex)
    : PSExecutor(text_idx, regex) {
}

void pull_star::PSFwdExecutor::compute(TokenSet &tokens, RegEx *regex) {
  switch (regex->getType()) {
    case RegExType::Blank: {
      break;
    }
    case RegExType::Primitive: {
      RegExPrimitive *primitive = (RegExPrimitive *) regex;
      switch (primitive->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram: {
          tokens.insert(primitive->getPrimitive());
          break;
        }
        case RegExPrimitiveType::Dot: {
          for (char c = 32; c < 127; c++) {
            if (c == '\n')
              continue;

            std::string token = std::string(1, c);
            if (text_idx_->contains(token)) {
              tokens.insert(token);
            }
          }
          break;
        }
        case RegExPrimitiveType::Range: {
          for (char c : primitive->getPrimitive()) {
            std::string token = std::string(1, c);
            if (text_idx_->contains(token)) {
              tokens.insert(token);
            }
          }
          break;
        }
      }
      break;
    }
    case RegExType::Union: {
      TokenSet first_res, second_res;
      compute(first_res, ((RegExUnion *) regex)->getFirst());
      compute(second_res, ((RegExUnion *) regex)->getSecond());
      regexUnion(tokens, first_res, second_res);
      break;
    }
    case RegExType::Concat: {
      TokenSet left_results;
      compute(left_results, ((RegExConcat *) regex)->getLeft());
      for (auto left_result : left_results) {
        TokenSet temp;
        regexConcat(temp, ((RegExConcat *) regex)->getRight(), left_result);
        regexUnion(tokens, tokens, temp);
      }
      break;
    }
    case RegExType::Repeat: {
      RegExRepeat *rep_r = ((RegExRepeat *) regex);
      switch (rep_r->getRepeatType()) {
        case RegExRepeatType::ZeroOrMore: {
          regexRepeatOneOrMore(tokens, rep_r->getInternal());
          break;
        }
        case RegExRepeatType::OneOrMore: {
          regexRepeatOneOrMore(tokens, rep_r->getInternal());
          break;
        }
        case RegExRepeatType::MinToMax: {
          regexRepeatMinToMax(tokens, rep_r->getInternal(), rep_r->getMin(),
                              rep_r->getMax());
        }
      }
      break;
    }
  }
}

void pull_star::PSFwdExecutor::regexConcat(TokenSet &concat_tokens,
                                           RegEx *regex, Token left_token) {
  switch (regex->getType()) {
    case RegExType::Blank: {
      break;
    }
    case RegExType::Primitive: {
      RegExPrimitive *primitive = (RegExPrimitive *) regex;
      switch (primitive->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram: {
          std::string token = left_token + primitive->getPrimitive();
          if (text_idx_->contains(token)) {
            concat_tokens.insert(token);
          }

          break;
        }
        case RegExPrimitiveType::Dot: {
          for (char c = 32; c < 127; c++) {
            if (c == '\n')
              continue;
            std::string token = left_token + c;
            if (text_idx_->contains(token)) {
              concat_tokens.insert(token);
            }
          }
          break;
        }
        case RegExPrimitiveType::Range: {
          for (char c : primitive->getPrimitive()) {
            std::string token = left_token + c;
            if (text_idx_->contains(token)) {
              concat_tokens.insert(token);
            }
          }
          break;
        }
      }
      break;
    }
    case RegExType::Union: {
      TokenSet res1, res2;
      regexConcat(res1, ((RegExUnion *) regex)->getFirst(), left_token);
      regexConcat(res2, ((RegExUnion *) regex)->getSecond(), left_token);
      regexUnion(concat_tokens, res1, res2);
      break;
    }
    case RegExType::Concat: {
      TokenSet right_left_results;
      regexConcat(right_left_results, ((RegExConcat *) regex)->getLeft(),
                  left_token);
      for (auto right_left_result : right_left_results) {
        TokenSet temp;
        regexConcat(temp, ((RegExConcat *) regex)->getRight(),
                    right_left_result);
        regexUnion(concat_tokens, concat_tokens, temp);
      }
      break;
    }
    case RegExType::Repeat: {
      RegExRepeat *rep_r = ((RegExRepeat *) regex);
      switch (rep_r->getRepeatType()) {
        case RegExRepeatType::ZeroOrMore: {
          regexRepeatOneOrMore(concat_tokens, rep_r->getInternal(), left_token);
          concat_tokens.insert(left_token);
          break;
        }
        case RegExRepeatType::OneOrMore: {
          regexRepeatOneOrMore(concat_tokens, rep_r->getInternal(), left_token);
          break;
        }
        case RegExRepeatType::MinToMax: {
          regexRepeatMinToMax(concat_tokens, rep_r->getInternal(), left_token,
                              rep_r->getMin(), rep_r->getMax());
        }
      }
      break;
    }
  }
}

pull_star::PSBwdExecutor::PSBwdExecutor(const dsl::TextIndex* text_idx,
                                        RegEx* regex)
    : PSExecutor(text_idx, regex) {
}

void pull_star::PSBwdExecutor::compute(TokenSet &tokens, RegEx *regex) {
  switch (regex->getType()) {
    case RegExType::Blank: {
      break;
    }
    case RegExType::Primitive: {
      RegExPrimitive *primitive = (RegExPrimitive *) regex;
      switch (primitive->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram: {
          tokens.insert(primitive->getPrimitive());
          break;
        }
        case RegExPrimitiveType::Dot: {
          for (char c = 32; c < 127; c++) {
            if (c == '\n')
              continue;

            std::string token = std::string(1, c);
            if (text_idx_->contains(token)) {
              tokens.insert(token);
            }
          }
          break;
        }
        case RegExPrimitiveType::Range: {
          for (char c : primitive->getPrimitive()) {
            std::string token = std::string(1, c);
            if (text_idx_->contains(token)) {
              tokens.insert(token);
            }
          }
          break;
        }
      }
      break;
    }
    case RegExType::Union: {
      TokenSet first_res, second_res;
      compute(first_res, ((RegExUnion *) regex)->getFirst());
      compute(second_res, ((RegExUnion *) regex)->getSecond());
      regexUnion(tokens, first_res, second_res);
      break;
    }
    case RegExType::Concat: {
      TokenSet right_results;
      compute(right_results, ((RegExConcat *) regex)->getRight());
      for (auto right_result : right_results) {
        TokenSet temp;
        regexConcat(temp, ((RegExConcat *) regex)->getLeft(), right_result);
        regexUnion(tokens, tokens, temp);
      }
      break;
    }
    case RegExType::Repeat: {
      RegExRepeat *rep_r = ((RegExRepeat *) regex);
      switch (rep_r->getRepeatType()) {
        case RegExRepeatType::ZeroOrMore: {
          regexRepeatOneOrMore(tokens, rep_r->getInternal());
          break;
        }
        case RegExRepeatType::OneOrMore: {
          regexRepeatOneOrMore(tokens, rep_r->getInternal());
          break;
        }
        case RegExRepeatType::MinToMax: {
          regexRepeatMinToMax(tokens, rep_r->getInternal(), rep_r->getMin(),
                              rep_r->getMax());
        }
      }
      break;
    }
  }
}

void pull_star::PSBwdExecutor::regexConcat(TokenSet &concat_tokens,
                                           RegEx *regex, Token right_token) {
  switch (regex->getType()) {
    case RegExType::Blank: {
      break;
    }
    case RegExType::Primitive: {
      RegExPrimitive *primitive = (RegExPrimitive *) regex;
      switch (primitive->getPrimitiveType()) {
        case RegExPrimitiveType::Mgram: {
          std::string token = primitive->getPrimitive() + right_token;
          if (text_idx_->contains(token)) {
            concat_tokens.insert(token);
          }

          break;
        }
        case RegExPrimitiveType::Dot: {
          for (char c = 32; c < 127; c++) {
            if (c == '\n')
              continue;
            std::string token = c + right_token;
            if (text_idx_->contains(token)) {
              concat_tokens.insert(token);
            }
          }
          break;
        }
        case RegExPrimitiveType::Range: {
          for (char c : primitive->getPrimitive()) {
            std::string token = c + right_token;
            if (text_idx_->contains(token)) {
              concat_tokens.insert(token);
            }
          }
          break;
        }
      }
      break;
    }
    case RegExType::Union: {
      TokenSet res1, res2;
      regexConcat(res1, ((RegExUnion *) regex)->getFirst(), right_token);
      regexConcat(res2, ((RegExUnion *) regex)->getSecond(), right_token);
      regexUnion(concat_tokens, res1, res2);
      break;
    }
    case RegExType::Concat: {
      TokenSet left_right_results;
      regexConcat(left_right_results, ((RegExConcat *) regex)->getRight(),
                  right_token);
      for (auto left_right_result : left_right_results) {
        TokenSet temp;
        regexConcat(temp, ((RegExConcat *) regex)->getLeft(),
                    left_right_result);
        regexUnion(concat_tokens, concat_tokens, temp);
      }
      break;
    }
    case RegExType::Repeat: {
      RegExRepeat *rep_r = ((RegExRepeat *) regex);
      switch (rep_r->getRepeatType()) {
        case RegExRepeatType::ZeroOrMore: {
          regexRepeatOneOrMore(concat_tokens, rep_r->getInternal(),
                               right_token);
          concat_tokens.insert(right_token);
          break;
        }
        case RegExRepeatType::OneOrMore: {
          regexRepeatOneOrMore(concat_tokens, rep_r->getInternal(),
                               right_token);
          break;
        }
        case RegExRepeatType::MinToMax: {
          regexRepeatMinToMax(concat_tokens, rep_r->getInternal(), right_token,
                              rep_r->getMin(), rep_r->getMax());
        }
      }
      break;
    }
  }
}
