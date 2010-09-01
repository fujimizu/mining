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

#ifndef MF_UTIL_H_
#define MF_UTIL_H_

#include <string>
#include <vector>

namespace mf {

const unsigned int DEFAULT_SEED = 12345;  ///< default seed value
const std::string DELIMITER("\t");        ///< delimiter string


/**
 * Compare pair items.
 * @param left  item
 * @param right item
 * @return return true if left_value > right_value
 */
template<typename KeyType, typename ValueType>
bool greater_pair(const std::pair<KeyType, ValueType> &left,
                  const std::pair<KeyType, ValueType> &right) {
  if (left.second > right.second) {
    return true;
  } else if (left.second == right.second) {
    return left.first > right.first;
  } else {
    return false;
  }
}

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

/**
 * Set seed for random number generator.
 * @param seed seed
 */
void mysrand(unsigned int seed);

/**
 * Get random number.
 * @param seed pointer of seed
 * @return random number
 */
int myrand(unsigned int *seed);

} /* namespace mf */

#endif  // MF_UTIL_H_
