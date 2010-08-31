//
// Utility functions
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include <cstdlib>
#include <sstream>
#include "util.h"

namespace mf {

/**
 * Set seed for random number generator.
 */
void mysrand(unsigned int seed) {
#if (_POSIX_C_SOURCE >= 1) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
  // do nothing
#else
  srand(seed);
#endif
}

/**
 * Get random number.
 */
int myrand(unsigned int *seed) {
#if (_POSIX_C_SOURCE >= 1) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
  return rand_r(seed);
#else
  return rand();
#endif
}

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

} /* namespace mf */
