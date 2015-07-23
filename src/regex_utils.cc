#include "utils.h"

void pull_star::Utils::split(std::vector<std::string>& results,
                             const std::string& str,
                             const std::string& delimiter) {

  size_t pos = 0;
  std::string substr;
  std::string str_copy(str);

  while ((pos = str_copy.find(delimiter)) != std::string::npos) {
    substr = str_copy.substr(0, pos);
    results.push_back(substr);
    str_copy.erase(0, pos + delimiter.length());
  }

  results.push_back(str_copy);
}
