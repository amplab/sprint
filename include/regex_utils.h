#ifndef PULL_STAR_REGEX_UTILS_H_
#define PULL_STAR_REGEX_UTILS_H_

#include <vector>
#include <string>

namespace pull_star {

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

class Utils {
 public:
  static void split(std::vector<std::string>& results, const std::string& str,
                    const std::string& delimiter);
};

}

#endif // PULL_STAR_REGEX_UTILS_H_
