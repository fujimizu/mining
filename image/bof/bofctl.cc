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
#include "visual_words.h"

/* function prototypes */
int main(int argc, char **argv);
static void usage(const char *progname);
static int run_vwd(int argc, char **argv);
static int run_lsh(int argc, char **argv);
static void vwd_base(std::istream &is);
static void lsh_base(std::istream &is);


int main(int argc, char **argv) {
  if (argc < 2) usage(argv[0]);
  std::string command(argv[1]);
  if (command == "vwd") {
    return run_vwd(argc, argv);
  } else if (command == "lsh") {
    return run_lsh(argc, argv);
  } else {
    usage(argv[0]);
  }
}

static void usage(const char *progname) {
  fprintf(stderr, "%s: Image Feature Extractor\n", progname);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, " %% %s vwd [file]\n", progname);
  fprintf(stderr, " %% %s lsh [file]\n", progname);
  exit(EXIT_FAILURE);
}

static int run_vwd(int argc, char **argv) {
  srand(time(NULL));
  vwd_base(std::cin);
  return 0;
}

static int run_lsh(int argc, char **argv) {
  if (argc == 2) {
    lsh_base(std::cin);
  } else if (argc == 3) {
    std::ifstream ifs(argv[2]);
    if (!ifs) {
      fprintf(stderr, "[Error] cannot open file: %s\n", argv[2]);
      return 1;
    }
    lsh_base(ifs);
  } else {
    usage(argv[0]);
  }
  return 0;
}

static void vwd_base(std::istream &is) {
  bof::VisualWords vwd;
  bof::SurfDetector detector;
  char desc_path[] = "vwd_desc.tmp";
  fprintf(stderr, "Saving descriptros..\n");
  vwd.save_descriptors(desc_path, is, detector);
  fprintf(stderr, "Clustering descriptros..\n");
  vwd.do_clustering(desc_path, 0.1);
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
