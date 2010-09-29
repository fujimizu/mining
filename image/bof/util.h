//
// Utility functions
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#ifndef BOF_UTIL_H_
#define BOF_UTIL_H_

#include <sstream>
#include <string>
#include <vector>

namespace bof {

/**
 * Split a string by a delimiter string.
 * @param s input string to be splited
 * @param delimiter delimiter string
 * @param splited splited strings
 */
void split_string(const std::string &s, const std::string &delimiter,
                  std::vector<std::string> &splited);

/**
 * Join strings by a delimiter string.
 * @param splited splited strings
 * @param delimiter delimiter string
 * @return a joined string
 */
std::string join_strings(const std::vector<std::string> &splited,
                         const std::string &delimiter);

} /* namespace bof */

#endif
