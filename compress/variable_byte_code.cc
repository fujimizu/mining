//
// Variable Byte code
// http://nlp.stanford.edu/IR-book/html/htmledition/variable-byte-codes-1.html
//

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <ctime>

void variable_byte_encode_number(uint64_t num, std::vector<uint64_t> &encoded) {
  for (;;) {
    encoded.push_back(num % 128);
    if (num < 128) break;
    num /= 128;
  }
  encoded[0] += 128;
}

char *variable_byte_encode(const std::vector<uint64_t> &numbers) {
  if (numbers.size() == 0) return NULL;
  // size of numbers
  std::vector<uint64_t> bytes, numenc;
  variable_byte_encode_number(numbers.size(), numenc);
  copy(numenc.rbegin(), numenc.rend(), std::back_inserter(bytes));
  numenc.clear();

  for (size_t i = 0; i < numbers.size(); i++) {
    variable_byte_encode_number(numbers[i], numenc);
    copy(numenc.rbegin(), numenc.rend(), std::back_inserter(bytes));
    numenc.clear();
  }
  char *buf = new char[bytes.size()];
  copy(bytes.begin(), bytes.end(), buf);
  std::cout << "Size(encoded): " << sizeof(char) * bytes.size() << std::endl;
  return buf;
}

void variable_byte_decode(const char *ptr, std::vector<uint64_t> &numbers) {
  // size of numbers
  uint64_t size = 0;
  uint64_t c;
  do {
    c = *(unsigned char *)ptr++;
    size = (c < 128) ? 128 * size + c : 128 * size + (c - 128);
  } while (c < 128);

  uint64_t cnt = 0;
  while (cnt < size) {
    uint64_t n = 0;
    do {
      c = *(unsigned char *)ptr++;
      n = (c < 128) ? 128 * n + c : 128 * n + (c - 128);
    } while (c < 128);
    numbers.push_back(n);
    cnt++;
  }
}

char *compress_diff(const std::vector<uint64_t> &numbers) {
  std::vector<uint64_t> diff;
  uint64_t prev = 0;
  for (size_t i = 0; i < numbers.size(); i++) {
    diff.push_back(numbers[i] - prev);
    prev = numbers[i];
  }
  return variable_byte_encode(diff);
}

void decompress_diff(const char *ptr, std::vector<uint64_t> &numbers) {
  variable_byte_decode(ptr, numbers);
  for (size_t i = 1; i < numbers.size(); i++) numbers[i] += numbers[i-1];
}

void random_numbers(size_t size, uint64_t max, std::vector<uint64_t> &numbers) {
  std::map<uint64_t, bool> check;
  size_t cnt = 0;
  while (cnt < size) {
    uint64_t num = static_cast<uint64_t>(rand()) % max;
    if (check.find(num) == check.end()) {
      numbers.push_back(num);
      check[num] = true;
      cnt++;
    }
  }
  std::sort(numbers.begin(), numbers.end());
}

int main(int argc, char **argv) {
  srand(static_cast<unsigned int>(time(NULL)));
  std::vector<uint64_t> numbers;
  size_t size = 10;
  random_numbers(size, size * 100, numbers);

  std::cout << "Size(input):   " << sizeof(uint64_t) * size << std::endl;
  std::cout << "Input:   ";
  for (size_t i = 0; i < numbers.size(); i++) {
    if (i != 0) std::cout << " ";
    std::cout << numbers[i];
  }
  std::cout << std::endl;

  char *encoded = compress_diff(numbers);
  //char *encoded = variable_byte_encode(numbers);

  std::vector<uint64_t> decoded;
  decompress_diff(encoded, decoded);
  //variable_byte_decode(encoded, decoded);
  delete [] encoded;

  std::cout << "Decoded: ";
  for (size_t i = 0; i < decoded.size(); i++) {
    if (i != 0) std::cout << " ";
    std::cout << decoded[i];
  }
  std::cout << std::endl;
  return 0;
}
