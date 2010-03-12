// NMF (Non-negative Matrix Factorization)
//
// Requirement:
//  - Eigen (http://eigen.tuxfamily.org/index.php?title=Main_Page)

#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <Eigen/Array>
#include <Eigen/Sparse>
#include <iostream>

USING_PART_OF_NAMESPACE_EIGEN
using namespace Eigen;

/* NMF class */
class Nmf {
 public:
  typedef std::map<std::string, uint32_t> KeyMap;
  typedef MatrixXf Mat;
  typedef SparseMatrix<double, RowMajor> SMat;

  Nmf() : matrix_(10, 10) { }

  void read_file(const char *filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
      fprintf(stderr, "cannot open %s\n", filename);
      exit(1);
    }
    KeyMap keymap;
    uint32_t maxcol = 0;
    uint32_t row = 0;
    std::string line;
    std::vector<std::string> splited;
    while (getline(ifs, line)) {
      splitstring(line, "\t", splited);
      if (splited.size() % 2 != 1) {
        fprintf(stderr, "format error: %s\n", line.c_str());
        continue;
      }
      for (size_t i = 1; i < splited.size(); i += 2) {
        KeyMap::iterator kit = keymap.find(splited[i]);
        uint32_t col;
        if (kit != keymap.end()) {
          col = kit->second;
        } else {
          col = maxcol;
          keymap[splited[i]] = maxcol++;
        }
        double point = 0.0;
        point = atof(splited[i+1].c_str());
        if (point != 0) {
          matrix_.fill(row, col) = point;
        }
      }
      splited.clear();
      row++;
    }
    std::cout << matrix_ << std::endl;
  }

  void factorize(size_t ncluster, size_t niter = 50) {
    Mat U(matrix_.rows(), ncluster);
    Mat V(ncluster, matrix_.cols());
    U.setRandom(matrix_.rows(), ncluster);
    U.normalize();
    V.setRandom(ncluster, matrix_.cols());
  }

 private:
  SMat matrix_;

  double difcost(SMat m1, SMat m2) {
    assert(m1.cols() == m2.cols() && m1.rows() == m2.rows());
    double distance = 0.0;
    for (size_t i = 0; i < m1.cols(); i++) {
      for (size_t j = 0; j < m1.rows(); j++) {
        double diff = m1.coeff(i, j) - m2.coeff(i, j);
        distance += diff * diff;
      }
    }
    return distance;
  }

  size_t splitstring(std::string s, const std::string &delimiter,
                     std::vector<std::string> &splited) {
    size_t cnt = 0;
    for (size_t p = 0; (p = s.find(delimiter)) != s.npos; ) {
      splited.push_back(s.substr(0, p));
      ++cnt;
      s = s.substr(p + delimiter.size());
    }
    splited.push_back(s);
    ++cnt;
    return cnt;
  }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "%s data ncluster\n", argv[0]);
    exit(1);
  }
  Nmf nmf;
  nmf.read_file(argv[1]);
  nmf.factorize(atoi(argv[2]));
  return 0;
}
