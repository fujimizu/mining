//
// Command-line tool
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include "descriptor.h"
#include "lsh.h"

/* function prototypes */
int main(int argc, char **argv);
static void usage(const char *progname);
static void extract_stdin();
static void extract_file(const char *path);
static void extract_base(std::istream &is);


int main(int argc, char **argv) {
  if (argc == 1) {
    extract_stdin();
  } else if (argc == 2) {
    extract_file(argv[1]);
  } else {
    usage(argv[0]);
  }
}

static void usage(const char *progname) {
  fprintf(stderr, "%s: Image Feature Extractor\n", progname);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, " %% %s [-d type] file\n", progname);
  exit(EXIT_FAILURE);
}

static void extract_stdin() {
  extract_base(std::cin);
}

static void extract_file(const char *path) {
  std::ifstream ifs(path);
  if (!ifs) {
    fprintf(stderr, "[Error] cannot open file: %s\n", path);
    exit(1);
  }
  extract_base(ifs);
}

static void extract_base(std::istream &is) {
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
