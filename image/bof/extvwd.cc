//
// Command-line tool for visual words
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include "descriptor.h"
#include "visual_words.h"

/* function prototypes */
int main(int argc, char **argv);
static void usage(const char *progname);
static void vwd_base(std::istream &is);


int main(int argc, char **argv) {
  srand(time(NULL));
  if (argc == 1) {
    vwd_base(std::cin);
  } else if (argc == 2) {
    std::ifstream ifs(argv[1]);
    if (!ifs) {
      fprintf(stderr, "cannot open file: %s\n", argv[1]);
      exit(1);
    }
    vwd_base(ifs);
  } else {
    usage(argv[0]);
  }
  return 0;
}

static void usage(const char *progname) {
  fprintf(stderr, "%s: Image Feature Extractor using Visual Words\n", progname);
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, " %% %s [file]\n", progname);
  exit(EXIT_FAILURE);
}

static void vwd_base(std::istream &is) {
  bof::VisualWords vwd;
  bof::SurfDetector detector;
  char desc_path[] = "vwd_desc.tmp";
  fprintf(stderr, "Saving descriptros..\n");
  vwd.save_descriptors(desc_path, is, detector);
  fprintf(stderr, "Clustering descriptros..\n");
  vwd.do_clustering(desc_path, 1.0);
  //vwd.do_clustering(desc_path, 0.1);
  fprintf(stderr, "Print bag-of-features..\n");
  vwd.get_bof(desc_path);
}
