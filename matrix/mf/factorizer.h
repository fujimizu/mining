//
// Matrix Factorizer
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

#ifndef MF_FACTORIZER_H_
#define MF_FACTORIZER_H_

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <tr1/unordered_map>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Sparse>
#include "util.h"

namespace mf {

/* typedef */
typedef Eigen::MatrixXf Mat;
typedef Eigen::SparseMatrix<int, Eigen::RowMajor> SMat;

/**
 * Matrix factorizer interfaces
 * (virtual class)
 */
class MatrixFactorizer {
 private:
  template<typename KeyType, typename ValueType>
  struct GreaterPair {
    bool operator() (const std::pair<KeyType, ValueType> a,
                     const std::pair<KeyType, ValueType> b) {
      return a.second < b.second;
    }
  };

  /**
   * Save a matrix to a file.
   * @param filename output file name
   * @return return true if succeeded
   */
  void save_matrix(const char *filename, const Mat &mat) const {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
      fprintf(stderr, "[Error] cannot open %s\n", filename);
      exit(1);
    }
    for (int i = 0; i < mat.rows(); i++) {
      for (int j = 0; j < mat.cols(); j++) {
        if (j != 0) fprintf(fp, "\t");
        fprintf(fp, "%.2f", mat(i, j));
      }
      fprintf(fp, "\n");
    }
    fclose(fp);
  }

 protected:
  SMat mtrain_;  ///< training matrix
  Mat U_;        ///< user matrix
  Mat V_;        ///< item matrix

  /**
   * Read matrix data from a text file.
   * @param filename a text file
   * @param mat output matrix
   */
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
      split_string(line, "\t", splited);
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
      split_string(line, "\t", splited);
      size_t userid = atoi(splited[0].c_str());
      size_t itemid = atoi(splited[1].c_str());
      int rate = atoi(splited[2].c_str());
      mat.insert(userid, itemid) = rate;
      splited.clear();
    }
  }

  /**
   * Set random values to a matrix.
   * @param mat matrix to be set values
   */
  void set_matrix_random(Mat &mat) const {
    for (int i = 0; i < mat.rows(); i++) {
      for (int j = 0; j < mat.cols(); j++) {
        mat(i, j) = static_cast<double>(rand()) / RAND_MAX;
      }
    }
  }

  /**
   * Get the average value of a matrix.
   * @param mat matrix
   * @return average value
   */
  double matrix_average(const SMat &mat) const {
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

  /**
   * Predict a rate using user matrix and item matrix. (virtual function)
   * @param user user index
   * @param item item index
   * @return rate
   */
  virtual double predict_rate(int user, int item) const = 0;

 public:
  /**
   * Constructor.
   */
  MatrixFactorizer() { }

  /**
   * Destructor.
   */
  ~MatrixFactorizer() { }

  /**
   * Factorize a training matrix. (virtual function)
   * @param ncluster the number of clusters
   * @param niter the number of iterations
   * @param eta a tuning parameter
   * @param lambda a tuning parameter
   */
  virtual void factorize(size_t ncluster, size_t niter,
                         double eta, double lambda) = 0;

  /**
   * Read a training file.
   * @param filename training file
   */
  virtual void train(const char *filename) {
    read_file(filename, mtrain_);
  }

  /**
   * Do test.
   * @param filename test file
   * @return RMSE(root mean square error)
   */
  double test(const char *filename) const {
    SMat mtest;
    read_file(filename, mtest);
    if (mtest.nonZeros() == 0) return -1;
    double rmse = 0.0;
    for (int j = 0; j < mtest.outerSize(); j++) {
      for (SMat::InnerIterator it(mtest, j); it; ++it) {
        if (it.row() >= U_.rows() || it.col() >= V_.cols()) {
          fprintf(stderr, "no data: row:%d col:%d\n", it.row(), it.col());
          continue;
        }
        int rate = round(predict_rate(it.row(), it.col()));
        rmse += (rate - it.value()) * (rate - it.value());
      }
    }
    return sqrt(rmse / mtest.nonZeros());
  }

  /**
   * Print all predicted rates.
   */
  void print_all_rate() const {
    for (int i = 1; i < mtrain_.rows(); i++) {
      for (int j = 1; j < mtrain_.cols(); j++) {
        printf("%d\t%d\t%.2f\n", i, j, predict_rate(i, j));
      }
    }
  }

  /**
   * Print top n predicted rates.
   * @param num the number of output rates
   */
  void print_top_rate(size_t num) const {
    for (int i = 1; i < mtrain_.rows(); i++) {
      std::priority_queue<std::pair<int, double>,
                          std::vector<std::pair<int, double> >,
                          GreaterPair<int, double> > pq;
      for (int j = 1; j < mtrain_.cols(); j++) {
        pq.push(std::pair<int, double>(j,predict_rate(i, j)));
      }
      for (size_t k = 0; k < num && !pq.empty(); k++) {
        std::pair<int, double> p = pq.top();
        printf("%d\t%d\t%.2f\n", i, p.first, p.second);
        pq.pop();
      }
    }
  }

  /**
   * Save a user matrix.
   * @param filename output file name
   */
  void save_user_matrix(const char *filename) const {
    save_matrix(filename, U_);
  }

  /**
   * Save a item matrix.
   * @param filename output file name
   */
  void save_item_matrix(const char *filename) const {
    save_matrix(filename, V_.transpose());
  }

  void save_recommend(const char *filename, size_t max) const {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
      fprintf(stderr, "[Error] cannot open %s\n", filename);
      exit(1);
    }
    // user cluster
    std::vector<int> user_clusters(U_.rows());
    for (int i = 1; i < U_.rows(); i++) {
      int max_idx = 1;
      double max_val = -1.0;
      for (int j = 1; j < U_.cols(); j++) {
        if (max_val < U_(i, j)) {
          max_idx = j;
          max_val = U_(i, j);
        }
      }
      user_clusters[i] = max_idx;
    }
    // cluster items
    std::vector<std::tr1::unordered_map<int, size_t> *>
      cluster_item_maps(V_.rows());
    std::tr1::unordered_map<int, size_t>::iterator cit;
    for (int i = 0; i < mtrain_.outerSize(); i++) {
      for (SMat::InnerIterator it(mtrain_, i); it; ++it) {
//        if (it.value() < 3.0) continue;
        int cluster_id = user_clusters[it.row()];
        if (cluster_item_maps[cluster_id] == NULL) {
          cluster_item_maps[cluster_id] =
            new std::tr1::unordered_map<int, size_t>;
        }
        cit = cluster_item_maps[cluster_id]->find(it.col());
        if (cit == cluster_item_maps[cluster_id]->end()) {
          cluster_item_maps[cluster_id]->insert(
            std::pair<int, size_t>(it.col(), 1));
        } else {
          cit->second += 1;
        }
      }
    }
    // sort cluster items
    std::vector<std::pair<int, size_t> > pairs;
    std::vector<std::vector<int> > cluster_items(V_.rows());
    for (size_t i = 0; i < cluster_item_maps.size(); i++) {
      if (cluster_item_maps[i] == NULL) continue;
      for (cit = cluster_item_maps[i]->begin();
           cit != cluster_item_maps[i]->end(); ++cit) {
        pairs.push_back(std::pair<int, size_t>(cit->first, cit->second));
      }
      sort(pairs.begin(), pairs.end(), greater_pair<int, size_t>);
      for (size_t j = 0; j < pairs.size() && j < max; j++) {
        cluster_items[i].push_back(pairs[j].first);
      }
      pairs.clear();
    }
    cluster_item_maps.clear();

    for (int i = 1; i < U_.rows(); i++) {
      fprintf(fp, "%d", i);
      int cluster_id = user_clusters[i];
      for (size_t j = 0; j < cluster_items[cluster_id].size(); j++) {
        fprintf(fp, "\t%d", cluster_items[cluster_id][j]);
      }
      fprintf(fp, "\n");
    }
    fclose(fp);
    for (size_t i = 0; i < cluster_item_maps.size(); i++) {
      if (cluster_item_maps[i]) delete cluster_item_maps[i];
    }
  }
};


/**
 * Matrix factorization using stochastic gradient descent.
 */
class MatrixFactorizerSgd : public MatrixFactorizer {
 protected:
  /**
   * Predict a rate using user matrix and item matrix.
   * @param user user index
   * @param item item index
   * @return a rate
   */
  double predict_rate(int user, int item) const {
    assert(user < U_.rows() && item < V_.cols());
    return U_.row(user).dot(V_.col(item));
  }

 public:
  /**
   * Constructor.
   */
  MatrixFactorizerSgd() { }

  /**
   * Destructor.
   */
  ~MatrixFactorizerSgd() { }

  /**
   * Factorize a training matrix.
   * @param ncluster the number of clusters
   * @param niter the number of iterations
   * @param eta a tuning parameter
   * @param lambda a tuning parameter
   */
  void factorize(size_t ncluster, size_t niter, double eta, double lambda) {
    size_t count = 0;
    size_t N = mtrain_.nonZeros();
    U_.resize(mtrain_.rows(), ncluster);
    V_.resize(ncluster, mtrain_.cols());
    set_matrix_random(U_);
    set_matrix_random(V_);
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


/**
 * Matrix factorization using stochastic gradient descent with biases.
 * Biases will be updated in each iteration.
 */
class MatrixFactorizerSgdBias : public MatrixFactorizerSgd {
 protected:
  std::vector<double> user_biases_;  ///< user bias
  std::vector<double> item_biases_;  ///< item bias
  double average_rate_;              ///< average value of rates

   /**
   * Get a bias value.
   * @param user user index
   * @param item item index
   * @return a bias value
   */
  double bias(int user, int item) const {
    assert(user < static_cast<int>(user_biases_.size()) &&
           item < static_cast<int>(item_biases_.size()));
    return average_rate_ + user_biases_[user] + item_biases_[item];
  }

  /**
   * Predict a rate using user matrix and item matrix.
   * @param user user index
   * @param item item index
   * @return a rate
   */
  double predict_rate(int user, int item) const {
    assert(user < U_.rows() && item < V_.cols());
    return bias(user, item) + U_.row(user).dot(V_.col(item));
  }

  /**
   * Set random values to the biases of users and items
   */
  void set_biases_random() {
    user_biases_.resize(mtrain_.rows());
    item_biases_.resize(mtrain_.cols());
    for (int i = 0; i < mtrain_.rows(); i++) {
      user_biases_[i] = static_cast<double>(rand()) / RAND_MAX;
    }
    for (int i = 0; i < mtrain_.cols(); i++) {
      item_biases_[i] = static_cast<double>(rand()) / RAND_MAX;
    }
  }

 public:
  /**
   * Constructor.
   */
  MatrixFactorizerSgdBias() : average_rate_(0.0) { }

  /**
   * Destructor.
   */
  ~MatrixFactorizerSgdBias() { }

  /**
   * Read a training file.
   * @param filename training file
   */
  void train(const char *filename) {
    read_file(filename, mtrain_);
    average_rate_ = matrix_average(mtrain_);
  }

  /**
   * Factorize a training matrix.
   * @param ncluster the number of clusters
   * @param niter the number of iterations
   * @param eta a tuning parameter
   * @param lambda a tuning parameter
   */
  void factorize(size_t ncluster, size_t niter, double eta, double lambda) {
    size_t count = 0;
    size_t N = mtrain_.nonZeros();
    U_.resize(mtrain_.rows(), ncluster);
    V_.resize(ncluster, mtrain_.cols());
    set_matrix_random(U_);
    set_matrix_random(V_);
    set_biases_random();
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
          average_rate_ += eta_2 * val;
          user_biases_[it.row()] +=
            eta_2 * (val - lambda * user_biases_[it.row()]);
          item_biases_[it.col()] +=
            eta_2 * (val - lambda * item_biases_[it.col()]);
        }
      }
    }
  }
};

/**
 * SVD++
 * Matrix factorization using stochastic gradient descent with biases
 * and implicit information (rental hisotry, ..)
 */
class MatrixFactorizerSvdpp : public MatrixFactorizerSgdBias {
 private:
  typedef std::vector<std::vector<int> > Implicit;
  Implicit implicit_;  ///< impclit information
  Mat Y_;              ///< bias of impclit information

  /**
   * Set implicit information
   */
  void set_implicit_information() {
    implicit_.resize(mtrain_.rows());
    for (int j = 0; j < mtrain_.outerSize(); j++) {
      for (SMat::InnerIterator it(mtrain_, j); it; ++it) {
        implicit_[it.row()].push_back(it.col());
      }
    }
  }

  /**
   * Get the bias values of implicit information
   * @param user user index
   * @return the vector of implicit bias
   */
  Eigen::VectorXf implicit_value(int user) const {
    Eigen::VectorXf vec = Eigen::VectorXf::Zero(Y_.rows());
    if (implicit_[user].size() == 0) return vec;
    for (size_t i = 0; i < implicit_[user].size(); i++) {
      vec += Y_.col(implicit_[user][i]);
    }
    vec *= pow(static_cast<double>(implicit_[user].size()), -0.5);
    return vec;
  }

 protected:
  /**
   * Predict a rate using user matrix and item matrix.
   * @param user user index
   * @param item item index
   * @return a rate
   */ 
  double predict_rate(int user, int item) const {
    return bias(user, item) + V_.col(item).transpose().dot(
      U_.row(user).transpose() + implicit_value(user));
//    double rate = bias(user, item) + V_.col(item).transpose().dot(
//      U_.row(user).transpose() + implicit_value(user));
//    return (rate > 5.0) ? 5.0 : (rate < 3.0) ? 3.0 : rate;
  }

 public:
  /**
   * Factorize a training matrix.
   * @param ncluster the number of clusters
   * @param niter the number of iterations
   * @param eta a tuning parameter
   * @param lambda a tuning parameter
   */
  void factorize(size_t ncluster, size_t niter, double eta, double lambda) {
    size_t count = 0;
    size_t N = mtrain_.nonZeros();
    U_.resize(mtrain_.rows(), ncluster);
    V_.resize(ncluster, mtrain_.cols());
    Y_.resize(ncluster, mtrain_.cols());
    set_matrix_random(U_);
    set_matrix_random(V_);
    set_matrix_random(Y_);
    set_biases_random();
    set_implicit_information();
    for (size_t i = 0; i < niter; i++) {
      for (int j = 0; j < mtrain_.outerSize(); j++) {
        for (SMat::InnerIterator it(mtrain_, j); it; ++it) {
          Eigen::VectorXf vec = implicit_value(it.row());
          count++;
          double eta_2 = eta / (1 + static_cast<double>(count) / N);
          double val = it.value() - predict_rate(it.row(), it.col());

          U_.row(it.row()) += eta_2 * (val * V_.col(it.col()).transpose()
                                       - lambda * U_.row(it.row()));
          V_.col(it.col()) +=
            eta_2 * (val * (U_.row(it.row()).transpose() + vec)
                     - lambda * V_.col(it.col()));
          average_rate_ += eta_2 * val;
          user_biases_[it.row()] +=
            eta_2 * (val - lambda * user_biases_[it.row()]);
          item_biases_[it.col()] +=
            eta_2 * (val - lambda * item_biases_[it.col()]);
          double coeff = pow(implicit_[it.row()].size(), -0.5);
          for (size_t k = 0; k < implicit_[it.row()].size(); k++) {
            Y_.col(implicit_[it.row()][k]) +=
              eta_2 * (val * coeff * V_.col(it.col())
                - lambda / 2.0 * Y_.col(implicit_[it.row()][k]));
          }
        }
      }
    }
  }
};

} /* namespace mf */

#endif  // MF_FACTORIZER_H_ 
