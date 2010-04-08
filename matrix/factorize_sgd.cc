//
// Matrix Factorization
//  1) gradient discent
//  2) stochastic grandient discent
//  3) stochastic gradient descent with biases
//
// Requirement:
//  - Eigen (http://eigen.tuxfamily.org/index.php?title=Main_Page)
//
// Input data:
//  - Movie Lens 100K ratings data set
//    http://www.grouplens.org/node/73
//
// Build:
//   % g++ -Wall -O3 sgd_movielens.cc -o sgd_movielens
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

/* virtual class of Matrix Factorization */
class MF {
 protected:
  SMat mtrain_;
  Mat U_;
  Mat V_;

  size_t split(std::string s, const std::string &delimiter,
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

  void read_file(const char *filename, SMat &mat) const {
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

  void set_random(Mat &mat) const {
    for (int i = 0; i < mat.rows(); i++) {
      for (int j = 0; j < mat.cols(); j++) {
        mat(i, j) = static_cast<double>(rand()) / RAND_MAX;
      }
    }
  }

  virtual double predict_rate(int user, int item) const = 0;

 public:
  virtual void factorize(size_t ncluster, size_t niter,
                         double eta, double lambda) = 0;

  void do_test(const char *filename) const {
    SMat mtest;
    read_file(filename, mtest);
    size_t ncorrect = 0;
    size_t ndiffone = 0;
    double rmse = 0.0;
    for (int j = 0; j < mtest.outerSize(); j++) {
      for (SMat::InnerIterator it(mtest, j); it; ++it) {
        if (it.row() >= U_.rows() || it.col() >= V_.cols()) {
          fprintf(stderr, "no data: row:%d col:%d\n", it.row(), it.col());
          continue;
        }
        int rate = round(predict_rate(it.row(), it.col()));
        if (rate == it.value()) ncorrect++;
        else if (abs(rate - it.value()) == 1) ndiffone++;
        rmse += (rate - it.value()) * (rate - it.value());
      }
    }
    int N = mtest.nonZeros();
    printf("Correct:    %ld / %d (%.3f%%)\n", ncorrect, N,
      static_cast<double>(ncorrect) / N * 100);
    printf("Diff(-1/1): %ld / %d (%.3f%%)\n", ndiffone, N,
      static_cast<double>(ndiffone) / N * 100);
    printf("RMSE:       %.3f\n", sqrt(rmse / N));
  }
};

/* matrix factorization using gradient descent */
class MFGD : public MF {
 protected:
  double predict_rate(int user, int item) const {
    assert(user < U_.rows() && item < V_.cols());
    return U_.row(user).dot(V_.col(item));
  }

 public:
  MFGD(const char *filename) {
    read_file(filename, mtrain_);
  }

  void factorize(size_t ncluster, size_t niter, double eta, double lambda) {
    U_.resize(mtrain_.rows(), ncluster);
    V_.resize(ncluster, mtrain_.cols());
    set_random(U_);
    set_random(V_);
    for (size_t i = 0; i < niter; i++) {
      Mat Utmp = Mat::Zero(U_.rows(), U_.cols());
      Mat Vtmp = Mat::Zero(V_.rows(), V_.cols());
      for (int j = 0; j < mtrain_.outerSize(); j++) {
        for (SMat::InnerIterator it(mtrain_, j); it; ++it) {
          double val = it.value() - predict_rate(it.row(), it.col());
          Utmp.row(it.row()) += val * V_.col(it.col()).transpose();
          Vtmp.col(it.col()) += val * U_.row(it.row()).transpose();
        }
      }
      U_ += eta * (Utmp - lambda * U_);
      V_ += eta * (Vtmp - lambda * V_);
    }
  }
};

/* matrix factorization using stochastic gradient descent */
class MFSGD : public MF {
 protected:
  double predict_rate(int user, int item) const {
    assert(user < U_.rows() && item < V_.cols());
    return U_.row(user).dot(V_.col(item));
  }

 public:
  MFSGD() { }
  MFSGD(const char *filename) {
    read_file(filename, mtrain_);
  }

  void factorize(size_t ncluster, size_t niter, double eta, double lambda) {
    size_t count = 0;
    size_t N = mtrain_.nonZeros();
    U_.resize(mtrain_.rows(), ncluster);
    V_.resize(ncluster, mtrain_.cols());
    set_random(U_);
    set_random(V_);
    for (size_t i = 0; i < niter; i++) {
      for (int j = 0; j < mtrain_.outerSize(); j++) {
        for (SMat::InnerIterator it(mtrain_, j); it; ++it) {
          count++;
          double eta_2 = eta / (1 + static_cast<double>(count) / N);
          double val = it.value() - predict_rate(it.row(), it.col());
          U_.row(it.row()) += eta_2 * (val * V_.col(it.col()).transpose()
                                       - lambda * U_.row(it.row()));
          V_.col(it.col()) += eta_2 * (val * U_.row(it.row()).transpose()
                                       - lambda * V_.col(it.col()));
        }
      }
    }
  }
};

/* matrix factorization using stochastic gradient descent with biases */
class MFSGDbias : public MFSGD {
 private:
  std::vector<double> user_biases_;
  std::vector<double> item_biases_;
  double average_rate_;

  double average(const SMat &mat) const {
    size_t N = mat.nonZeros();
    if (N == 0) return 0.0;
    double sum = 0.0;
    for (int j = 0; j < mat.outerSize(); j++) {
      for (SMat::InnerIterator it(mat, j); it; ++it) {
        sum += it.value();
      }
    }
    return sum / N;
  }

  void calc_biases() {
    std::vector<size_t> user_count(mtrain_.rows(), 0);
    std::vector<size_t> item_count(mtrain_.cols(), 0);
    user_biases_.resize(mtrain_.rows());
    item_biases_.resize(mtrain_.cols());
    for (int i = 0; i < mtrain_.outerSize(); i++) {
      for (SMat::InnerIterator it(mtrain_, i); it; ++it) {
        user_count[it.row()]++;
        item_count[it.col()]++;
        user_biases_[it.row()] += it.value();
        item_biases_[it.col()] += it.value();
      }
    }
    for (size_t i = 0; i < user_biases_.size(); i++) {
      if (user_count[i])
        user_biases_[i] = user_biases_[i] / user_count[i] - average_rate_;
    }
    for (size_t i = 0; i < item_biases_.size(); i++) {
      if (item_count[i])
        item_biases_[i] = item_biases_[i] / item_count[i] - average_rate_;
    }
  }

  double bias(int user, int item) const {
    assert(user < static_cast<int>(user_biases_.size()) &&
           item < static_cast<int>(item_biases_.size()));
    return average_rate_ + user_biases_[user] + item_biases_[item];
  }

 protected:
  double predict_rate(int user, int item) const {
    assert(user < U_.rows() && item < V_.cols());
    return bias(user, item) + U_.row(user).dot(V_.col(item));
  }

 public:
  MFSGDbias(const char *filename) {
    read_file(filename, mtrain_);
    average_rate_ = average(mtrain_);
    calc_biases();
  }
};

int main(int argc, char **argv) {
  if (argc != 7) {
    fprintf(stderr, "Usage: %s train test ncluster niter eta lambda\n",
            argv[0]);
    exit(1);
  }
  srand(time(NULL));
  size_t ncluster = atoi(argv[3]);
  size_t niter    = atoi(argv[4]);
  double eta      = atof(argv[5]);
  double lambda   = atof(argv[5]);

  printf("Reading input data\n");
  //MFGD mf(argv[1]);
  //MFSGD mf(argv[1]);
  MFSGDbias mf(argv[1]);

  printf("Factorizing input matrix\n");
  mf.factorize(ncluster, niter, eta, lambda);
  printf("Input matrix was factorized ( M = U * V )\n");

  printf("Checking test data\n");
  mf.do_test(argv[2]);

  return 0;
}
