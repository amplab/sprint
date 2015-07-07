#ifndef PULL_STAR_REGEX_PARSER_H_
#define PULL_STAR_REGEX_PARSER_H_

#include <exception>
#include "regex_types.h"
namespace pull_star {

// Grammar:
//   <regex> ::= <term> '|' <regex>
//            |  <term>
//
//   <term> ::= { <factor> }
//
//   <factor> ::= <base> { '*' | '+' | '{' <num> ',' <num> ')' }
//
//   <base> ::= <mgram>
//            |  '(' <regex> ')'
//
//   <mgram> ::= <char> | '\' <char> { <mgram> }
//   <num> ::= <digit> { <num> }

class ParseException : public std::exception {
  virtual const char* what() const throw ();
};

class RegExParser {
 private:
  char *expression_;
  RegEx *blank_regex_;

 public:
  RegExParser(char *exp);
  ~RegExParser();

  RegEx *parse();

 private:
  char peek();
  void eat(char c);
  char next();
  bool more();
  RegEx *regex();
  RegEx *concat(RegEx *a, RegEx *b);
  RegEx *term();
  RegEx *factor();
  RegEx *base();
  char nextChar();
  int nextInt();
  RegEx* primitive();
  RegEx* parsePrimitives(std::string p_str);
  RegEx* nextPrimitive(std::string &p_str);
  RegExPrimitiveType getPrimitiveType(std::string mgram);
};

}

#endif // PULL_STAR_REGEX_PARSER_H_
