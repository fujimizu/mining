//
// Command-line tool of matrix factorization
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

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <string>
#include "factorizer.h"

/* typedef */
//typedef mf::MatrixFactorizerSvdpp MF;
typedef mf::MatrixFactorizerSgdBias MF;

/* constants */
size_t MAX_RECOMMEND = 30;

/* function prototypes */
int main(int argc, char **argv);
static void usage(const char *progname);
static int run_factorize(int argc, char **argv);
static int run_test(int argc, char **argv);
static int run_mktest(int argc, char **argv);
void cross_validation(const char *dir, size_t ncluster,
                      size_t niter, double eta, double lambda);

int main(int argc, char **argv) {
  if (argc < 2) usage(argv[0]);
  std::string command(argv[1]);
  if (command == "factorize") {
    return run_factorize(argc, argv);
  } else if (command == "test") {
    return run_test(argc, argv);
  } else if (command == "mktest") {
    return run_mktest(argc, argv);
  } else {
    usage(argv[0]);
  }
}

/**
 * Show usage.
 * @param progname the name of this program
 */
static void usage(const char *progname) {
  fprintf(stderr, "%s: matrix factorization utility tool\n", progname);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, " %% %s factorize file dir ncluster niter eta lambda\n", progname);
  fprintf(stderr, " %% %s mktest file dir ntest\n", progname);
  fprintf(stderr, " %% %s test dir ncluster niter eta lambda\n", progname);
  std::exit(EXIT_FAILURE);
}


static int run_factorize(int argc, char **argv) {
  const char *progname = argv[0];
  if (argc != 8) usage(progname);
  char *filename  = argv[2];
  char *dirname   = argv[3];
  size_t ncluster = atoi(argv[4]);
  size_t niter    = atoi(argv[5]);
  double eta      = atof(argv[6]);
  double lambda   = atof(argv[7]);

  MF mf;
  mf.train(filename);
  fprintf(stderr, "Factorizing input matrix ...\n");
  mf.factorize(ncluster, niter, eta, lambda);
  fprintf(stderr, "Saving a user matirx and a item matrix ...\n");
  char upath[256], ipath[256], rpath[256];
  sprintf(upath, "%s/usermat.tsv", dirname);
  mf.save_user_matrix(upath);
  sprintf(ipath, "%s/itemmat.tsv", dirname);
  mf.save_item_matrix(ipath);
  fprintf(stderr, "Saving recommend items for each user ...\n");
  sprintf(rpath, "%s/recom.tsv", dirname);
  mf.save_recommend(rpath, MAX_RECOMMEND);
  return 0;
}

/**
 * Run cross validation test.
 */
static int run_test(int argc, char **argv) {
  const char *progname = argv[0];
  if (argc != 7) usage(progname);
  char *dirname   = argv[2];
  size_t ncluster = atoi(argv[3]);
  size_t niter    = atoi(argv[4]);
  double eta      = atof(argv[5]);
  double lambda   = atof(argv[6]);

  size_t ntest = 0;
  double sum = 0.0;
  struct stat st_train;
  struct stat st_test;
  for (size_t i = 1; ; i++) {
    char train_path[128], test_path[128];
    sprintf(train_path, "%s/u%ld.base", dirname, i);
    sprintf(test_path, "%s/u%ld.test", dirname, i);
    if (stat(train_path, &st_train) || stat(test_path, &st_test)) break;
    printf("Training data: %s\n", train_path);
    printf("Test data:     %s\n", test_path);
    MF mf;
    mf.train(train_path);
    printf("Factorizing input matrix ...\n");
    mf.factorize(ncluster, niter, eta, lambda);
    double rmse = mf.test(test_path);
    printf("RMSE=%0.3f\n\n", rmse);
    sum += rmse;
    ntest++;
  }
  printf("Result of cross validation: RMSE=%.3f\n", sum / ntest);
  return 0;
}

/**
 * Make test sets to split a file into a training file and a test file.
 */
static int run_mktest(int argc, char **argv) {
  const char *progname = argv[0];
  if (argc != 5) usage(progname);
  char *filename = argv[2];
  char *dirname  = argv[3];
  size_t ntest   = atoi(argv[4]);

  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "[Error] cannot open file: %s\n", filename);
    exit(1);
  }
  FILE *fp_trains[ntest];
  FILE *fp_tests[ntest];
  for (size_t i = 0; i < ntest; i++) {
    char train_path[256], test_path[256];
    sprintf(train_path, "%s/u%ld.base", dirname, i+1);
    if ((fp_trains[i] = fopen(train_path, "w")) == NULL) {
      fprintf(stderr, "[Error] cannot open file: %s\n", train_path);
      exit(1);
    }
    sprintf(test_path, "%s/u%ld.test", dirname, i+1);
    if ((fp_tests[i] = fopen(test_path, "w")) == NULL) {
      fprintf(stderr, "[Error] cannot open file: %s\n", test_path);
      exit(1);
    }
  }

  std::string line;
  while (getline(ifs, line)) {
    size_t index = rand() % ntest;
    for (size_t i = 0; i < ntest; i++) {
      if (i == index) fprintf(fp_tests[i], "%s\n", line.c_str());
      else fprintf(fp_trains[i], "%s\n", line.c_str());
    }
  }
  for (size_t i = 0; i < ntest; i++) {
    fclose(fp_trains[i]);
    fclose(fp_tests[i]);
  }
  ifs.close();
  return 0;
}
