//
// NMF (Non-negative Matrix Factorization)
//
// Requirement:
//  - Eigen (http://eigen.tuxfamily.org/index.php?title=Main_Page)
//
// Format of input data:
//   document_id1 \t key1-1 \t value1-1 \t key1-2 \t value1-2 \t ...\n
//   document_id2 \t key2-1 \t value2-1 \t key2-2 \t value2-2 \t ...\n
//   ...
//
// Build:
//   % g++ -Wall -O3 nmf.cc -o nmf
//

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <Eigen/Array>
#include <Eigen/Sparse>

using namespace Eigen;

/* NMF class */
class Nmf {
 public:
  typedef std::map<std::string, uint32_t> Str2Column;
  typedef MatrixXf Mat;
  typedef SparseMatrix<double, RowMajor> SMat;

  Nmf() { }

  void read_file(const char *filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
      fprintf(stderr, "cannot open %s\n", filename);
      exit(1);
    }
    Str2Column s2c;
    uint32_t maxcol = 0;
    uint32_t maxrow = 0;
    std::string line;
    std::vector<std::string> splited;
    while (getline(ifs, line)) {
      splitstring(line, "\t", splited);
      if (splited.size() % 2 != 1) {
        fprintf(stderr, "format error: %s\n", line.c_str());
        continue;
      }
      document_ids_.push_back(splited[0]);
      for (size_t i = 1; i < splited.size(); i += 2) {
        Str2Column::iterator kit = s2c.find(splited[i]);
        if (kit == s2c.end()) {
          s2c[splited[i]] = maxcol;
          feature_ids_.push_back(splited[i]);
          maxcol++;
        }
      }
      splited.clear();
      maxrow++;
    }
    ifs.clear();
    ifs.seekg(0, std::ios_base::beg);
    V_.resize(maxrow, maxcol);

    size_t row = 0;
    std::vector<std::pair<uint32_t, double> > pairs;
    while (getline(ifs, line)) {
      splitstring(line, "\t", splited);
      if (splited.size() % 2 != 1) {
        fprintf(stderr, "format error: %s\n", line.c_str());
        continue;
      }
      for (size_t i = 1; i < splited.size(); i += 2) {
        uint32_t col = s2c[splited[i]];
        double point = atof(splited[i+1].c_str());
        if (point != 0) {
          pairs.push_back(std::pair<uint32_t, double>(col, point)); 
        }
      }
      std::sort(pairs.begin(), pairs.end());
      for (size_t i = 0; i < pairs.size(); i++) {
        V_.fill(row, pairs[i].first) = pairs[i].second;
      }
      splited.clear();
      pairs.clear();
      row++;
    }
    V_ = V_.transpose();
  }

  void factorize(size_t ncluster, size_t niter) {
    W_.resize(V_.rows(), ncluster);
    H_.resize(ncluster, V_.cols());
    set_random(W_);
    W_.normalize();
    set_random(H_);
    for (size_t i = 0; i < niter; i++) {
      Mat Wt = W_.transpose();
      Mat hnumer = Wt * V_;
      Mat hdenom = Wt * W_ * H_;
      Mat Ht = H_.transpose();
      Mat wnumer = V_ * Ht;
      Mat wdenom = W_ * H_ * Ht;
      for (int j = 0; j < H_.rows(); j++) {
        for (int k = 0; k < H_.cols(); k++) {
          if (hdenom(j, k) != 0)
            H_(j, k) = H_(j, k) * hnumer(j, k) / hdenom(j, k);
          if (wdenom(k, j) != 0)
            W_(k, j) = W_(k, j) * wnumer(k, j) / wdenom(k, j);
        }
      }
      H_.normalize();
//      double cost = difcost(V_, W_*H_);
//      fprintf(stderr, " loop: %ld\tcost: %4f\n", i, cost);
//      if (cost == 0) break;
      if ((i + 1) % 10 == 0) printf(" loop: %ld\n", i+1);
    }
    W_.normalize();
  }

  void show_result() const {
    printf("Input matrix was factorized. ( V = W * H )\n");
    printf("=== W matrix ===\n");
    for (int i = 0; i < W_.rows(); i++) {
      if (static_cast<int>(feature_ids_.size()) <= i) break;
      printf("%s", feature_ids_[i].c_str());
      for (int j = 0; j < W_.cols(); j++) {
        printf("\t%.4f", W_(i, j));
      }
      printf("\n");
    }
    printf("\n=== H matrix (transposed) ===\n");
    for (int j = 0; j < H_.cols(); j++) {
      if (static_cast<int>(document_ids_.size()) <= j) break;
      printf("%s", document_ids_[j].c_str());
      for (int i = 0; i < H_.rows(); i++) {
        printf("\t%.4f", H_(i, j));
      }
      printf("\n");
    }
  }

 private:
  SMat V_;
  Mat W_;
  Mat H_;
  std::vector<std::string> feature_ids_;
  std::vector<std::string> document_ids_;

  double difcost(const SMat m1, const Mat m2) const {
    assert(m1.cols() == m2.cols() && m1.rows() == m2.rows());
    double distance = 0.0;
    for (int i = 0; i < m1.outerSize(); i++) {
      for (SMat::InnerIterator it(m1, i); it; ++it) {
        double diff = it.value() - m2(it.row(), it.col());
        distance += diff * diff;
      }
    }
    return distance;
  }

  void set_random(Mat &mat) const {
    for (int i = 0; i < mat.rows(); i++) {
      for (int j = 0; j < mat.cols(); j++) {
        mat(i, j) = static_cast<double>(rand()) / RAND_MAX;
      }
    }
  }

  size_t splitstring(std::string s, const std::string &delimiter,
                     std::vector<std::string> &splited) const {
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
    fprintf(stderr, "Usage: %s data ncluster [niter]\n", argv[0]);
    exit(1);
  }
  srand(time(NULL));
  Nmf nmf;
  printf("Reading input data\n");
  nmf.read_file(argv[1]);

  size_t niter = 50;
  if (argc == 4) niter = atoi(argv[3]);
  printf("Factorizing input matrix\n");
  nmf.factorize(atoi(argv[2]), niter);
  nmf.show_result();
  return 0;
}
