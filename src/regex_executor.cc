#include "regex_executor.h"

pull_star::RegExExecutor::RegExExecutor(dsl::TextIndex *text_idx,
                                        RegEx *regex) {
  text_idx_ = text_idx;
  regex_ = regex;
}

pull_star::RegExExecutor::~RegExExecutor() {
}

void pull_star::RegExExecutor::getResults(RegExResult& result) {
  result = final_result_;
}

pull_star::BBExecutor::BBExecutor(dsl::TextIndex *text_idx,
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
          if (primitive == '.') {
            primitive =
                " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
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
  std::vector<int64_t> offsets = text_idx_->search(mgram);
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
    while (right_it != right.end() && right_it->first <= left_it->first)
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
