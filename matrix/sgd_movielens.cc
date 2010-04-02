//
// Gradient discent method, Stochastic gradient discent
//
// Requirement:
//  - Eigen (http://eigen.tuxfamily.org/index.php?title=Main_Page)
//
// Input data:
//  - Movie Lens 100K ratings data set
//    http://www.grouplens.org/node/73
//
// Build:
//   % g++ -Wall -O3 gd.cc -o gd
//

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Eigen/Array>
#include <Eigen/Sparse>

using namespace Eigen;

/* typedef */
typedef MatrixXf Mat;
typedef SparseMatrix<int, RowMajor> SMat;

/* function prototypes */
size_t split(std::string s, const std::string &delimiter,
             std::vector<std::string> &splited);
void read_file(const char *filename, SMat &mat);
void set_random(Mat &mat);
void gradient_descent(const SMat &mat, Mat &U, Mat &V, size_t niter);
void stochastic_gradient_descent(const SMat &mat, Mat &U, Mat &V,
                                 size_t niter, double eta, double lambda);
void stochastic_gradient_descent(const SMat &mat, Mat &U, Mat &V,
                                 size_t niter, double eta0, double lambda);
void check_test_data(const SMat &mat_test, const Mat &mat);


size_t split(std::string s, const std::string &delimiter,
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

void read_file(const char *filename, SMat &mat) {
  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", filename);
    exit(1);
  }
  size_t max_userid = 0;
  size_t max_itemid = 0;
  std::string line;
  std::vector<std::string> splited;
  while (getline(ifs, line)) {
    split(line, "\t", splited);
    size_t userid = atoi(splited[0].c_str());
    size_t itemid = atoi(splited[1].c_str());
    if (max_userid < userid) max_userid = userid;
    if (max_itemid < itemid) max_itemid = itemid;
    splited.clear();
  }
  mat.resize(max_userid+1, max_itemid+1);

  ifs.clear();
  ifs.seekg(0, std::ios_base::beg);
  while (getline(ifs, line)) {
    split(line, "\t", splited);
    size_t userid = atoi(splited[0].c_str());
    size_t itemid = atoi(splited[1].c_str());
    int rate = atoi(splited[2].c_str());
    mat.fill(userid, itemid) = rate;
    splited.clear();
  }
}

void set_random(Mat &mat) {
  for (int i = 0; i < mat.rows(); i++) {
    for (int j = 0; j < mat.cols(); j++) {
      mat(i, j) = static_cast<double>(rand()) / RAND_MAX;
    }
  }
}

void gradient_descent(const SMat &mat, Mat &U, Mat &V,
                      size_t niter, double eta, double lambda) {
  set_random(U);
  set_random(V);
  for (size_t i = 0; i < niter; i++) {
    Mat Unew = Mat::Zero(U.rows(), U.cols());
    Mat Vnew = Mat::Zero(V.rows(), V.cols());
    for (size_t j = 0; j < mat.outerSize(); j++) {
      for (SMat::InnerIterator it(mat, j); it; ++it) {
        double val = it.value() - U.row(it.row()).dot(V.col(it.col()));
        Unew.row(it.row()) += val * V.col(it.col()).transpose();
        Vnew.col(it.col()) += val * U.row(it.row()).transpose();
      }
    }
    U += eta * (Unew - lambda * U);
    V += eta * (Vnew - lambda * V);
  }
}

void stochastic_gradient_descent(const SMat &mat, Mat &U, Mat &V,
                                 size_t niter, double eta0, double lambda) {
  size_t count = 0;
  size_t N = mat.nonZeros();
  set_random(U);
  set_random(V);
  for (size_t i = 0; i < niter; i++) {
    for (size_t j = 0; j < mat.outerSize(); j++) {
      for (SMat::InnerIterator it(mat, j); it; ++it) {
        count++;
        double eta = eta0 / (1 + static_cast<double>(count) / N);
        double val = it.value() - U.row(it.row()).dot(V.col(it.col()));
        U.row(it.row()) +=
          eta * (val * V.col(it.col()).transpose() - lambda * U.row(it.row()));
        V.col(it.col()) +=
          eta * (val * U.row(it.row()).transpose() - lambda * V.col(it.col()));
      }
    }
  }
}

void check_test_data(const SMat &mat_test, const Mat &mat) {
  size_t ncorrect = 0;
  size_t ndiffone = 0;
  for (size_t j = 0; j < mat_test.outerSize(); j++) {
    for (SMat::InnerIterator it(mat_test, j); it; ++it) {
      int rate = round(mat(it.row(), it.col()));
      if (rate == it.value()) ncorrect++;
      else if (abs(rate - it.value()) == 1) ndiffone++;
    }
  }
  printf("Correct:    %d / %d (%.3f\%)\n", ncorrect, mat_test.nonZeros(),
    static_cast<double>(ncorrect) / mat_test.nonZeros() * 100);
  printf("Diff(-1/1): %d / %d (%.3f\%)\n", ndiffone, mat_test.nonZeros(),
    static_cast<double>(ndiffone) / mat_test.nonZeros() * 100);
}

int main(int argc, char **argv) {
  if (argc != 7) {
    fprintf(stderr, "Usage: %s train test ncluster niter eta lambda\n", argv[0]);
    exit(1);
  }
  srand(time(NULL));
  size_t ncluster = atoi(argv[3]);
  size_t niter    = atoi(argv[4]);
  double eta      = atof(argv[5]);
  double lambda   = atof(argv[5]);
  SMat mat_train;
  SMat mat_test;

  printf("Reading input data\n");
  read_file(argv[1], mat_train);
  read_file(argv[2], mat_test);
  Mat U(mat_train.rows(), ncluster);
  Mat V(ncluster, mat_train.cols());

  printf("Factorizing input matrix\n");
  //gradient_descent(mat_train, U, V, niter, eta, lambda);
  stochastic_gradient_descent(mat_train, U, V, niter, eta, lambda);
    
  printf("Input matrix was factorized ( M = U * V )\n");
  //std::cout << U << std::endl << V << std::endl;

  printf("Checking test data\n");
  check_test_data(mat_test, U * V);

  return 0;
}
