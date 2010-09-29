//
// Utility functions
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#include "util.h"

namespace bof {
/**
 * Split a string by a delimiter string.
 */
void split_string(const std::string &s, const std::string &delimiter,
                  std::vector<std::string> &splited) {
  for (size_t i = s.find_first_not_of(delimiter); i != std::string::npos; ) {
    size_t j = s.find_first_of(delimiter, i);
    if (j != std::string::npos) {
      splited.push_back(s.substr(i, j-i));
      i = s.find_first_not_of(delimiter, j+1);
    } else {
      splited.push_back(s.substr(i, s.size()));
      break;
    }
  }
}

/**
 * Join strings by a delimiter string.
 * @param splited splited strings
 * @param delimiter delimiter string
 * @return a joined string
 */
std::string join_strings(const std::vector<std::string> &splited,
                         const std::string &delimiter) {
  std::stringstream ss;
  for (size_t i = 0; i < splited.size(); i++) {
    if (i > 0) ss << delimiter;
    ss << splited[i];
  }
  return ss.str();
}

} /* namespace bof */
