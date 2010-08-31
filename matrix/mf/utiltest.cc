//
// Tests for Utility functions
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

#include <gtest/gtest.h>
#include "util.h"

/* split_string */
TEST(UtilTest, SplitStringTest) {
  std::string input;
  std::vector<std::string> splited;

  // space delimiter
  input = "This is a pen";
  mf::split_string(input, " ", splited);
  EXPECT_EQ(4, splited.size());
  EXPECT_EQ("This", splited[0]);
  EXPECT_EQ("is",   splited[1]);
  EXPECT_EQ("a",    splited[2]);
  EXPECT_EQ("pen",  splited[3]);
  splited.clear();

  // tab delimiter
  input = "This\tis\ta\tpen";
  mf::split_string(input, "\t", splited);
  EXPECT_EQ(4, splited.size());
  EXPECT_EQ("This", splited[0]);
  EXPECT_EQ("is",   splited[1]);
  EXPECT_EQ("a",    splited[2]);
  EXPECT_EQ("pen",  splited[3]);
  splited.clear();

  // space delimiter
  input = "あい うえ おか";
  mf::split_string(input, " ", splited);
  EXPECT_EQ(3, splited.size());
  EXPECT_EQ("あい", splited[0]);
  EXPECT_EQ("うえ",   splited[1]);
  EXPECT_EQ("おか",    splited[2]);
  splited.clear();

  // tab delimiter
  input = "あい\tうえ\tおか";
  mf::split_string(input, "\t", splited);
  EXPECT_EQ(3, splited.size());
  EXPECT_EQ("あい", splited[0]);
  EXPECT_EQ("うえ",   splited[1]);
  EXPECT_EQ("おか",    splited[2]);
  splited.clear();
}

TEST(UtilTest, JoinStringsTest) {
  std::vector<std::string> splited;
  splited.push_back("a");
  splited.push_back("bc");
  splited.push_back("def");
  splited.push_back("gh");
  std::string joined = mf::join_strings(splited, mf::DELIMITER);
  EXPECT_EQ("a\tbc\tdef\tgh", joined);
}

int main(int argc, char **argv) {
  srand((unsigned int)time(NULL));
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
