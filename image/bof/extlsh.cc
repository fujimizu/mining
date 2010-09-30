//
// Command-line tool
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "descriptor.h"
#include "lsh.h"

/* function prototypes */
int main(int argc, char **argv);
static void usage(const char *progname);
static void lsh_base(std::istream &is);


int main(int argc, char **argv) {
  if (argc == 1) {
    lsh_base(std::cin);
  } else if (argc == 2) {
    std::ifstream ifs(argv[1]);
    if (!ifs) {
      fprintf(stderr, "[Error] cannot open file: %s\n", argv[1]);
      return 1;
    }
    lsh_base(ifs);
  } else {
    usage(argv[0]);
  }
  return 0;
}

static void usage(const char *progname) {
  fprintf(stderr, "%s: Image Feature Extractor using LSH\n", progname);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, " %% %s [file]\n", progname);
  exit(EXIT_FAILURE);
}

static void lsh_base(std::istream &is) {
  bof::SurfDetector detector;
  std::string line;
  std::vector<std::vector<float> > features;
  std::vector<size_t> values;
  bof::Lsh lsh(10, 128, -1.0, 1.0);
  while (std::getline(is, line)) {
    printf("%s", line.c_str());
    features.clear();
    detector.extract(line.c_str(), features);
    if (features.empty()) {
      fprintf(stderr, "[Warning] cannot detect descriptors: %s\n",
              line.c_str());
      continue;
    }
    values.clear();
    lsh.hash(features, values);
    if (values.empty()) {
      fprintf(stderr, "[Warning] lsh error: %s\n", line.c_str());
      continue;
    } else {
      printf("%s", line.c_str());
      for (size_t i = 0; i < values.size(); i++)
        if (values[i]) printf("\t%zd\t%zd", i, values[i]);
      printf("\n");
    }
  }
}
