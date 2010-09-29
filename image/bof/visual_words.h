//
// Visual words
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#include <cstdio>
#include <fstream>
#include <bayon.h>  // bayon library
#include "descriptor.h"
#include "util.h"

#ifndef BOF_VISUAL_WORDS_H_
#define BOF_VISUAL_WORDS_H_

namespace bof {

/**
 * Visual words
 */
class VisualWords {
 private:
  static const double CLUSTER_LIMIT = 1.5;
  static const size_t CLVECTOR_SIZE = 128;

  bayon::Analyzer analyzer_;
  bayon::Classifier classifier_;

 public:
  VisualWords() { }
  ~VisualWords() { }

  void save_descriptors(const char *path, std::istream &is,
                        FeatureDetector &detector) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
      fprintf(stderr, "cannot open file: %s\n", path);
      exit(1);
    }
    std::string line;
    std::vector<std::vector<float> > features;
    bof::VisualWords vwd;
    while (std::getline(is, line)) {
      features.clear();
      detector.extract(line.c_str(), features);
      if (features.empty()) {
        fprintf(stderr, "[Warning] cannot detect descriptors: %s\n",
                line.c_str());
        continue;
      }
      for (size_t i = 0; i < features.size(); i++) {
        fprintf(fp, "%s %zd", line.c_str(), i);
        for (size_t j = 0; j < features[i].size(); j++) {
          fprintf(fp, "\t%f", features[i][j]);
        }
        fprintf(fp, "\n");
      }
    }
    fclose(fp);
  }

  void do_clustering(const char *path, double rate) {
    std::ifstream ifs(path);
    if (!ifs) {
      fprintf(stderr, "cannot open file: %s\n", path);
      exit(1);
    }
    std::string line;
    std::vector<std::string> splited;
    bayon::DocumentId id = 0;
    while (std::getline(ifs, line)) {
      if (line.empty() || rand() / RAND_MAX > rate) continue;  // random skip
      splited.clear();
      bof::split_string(line, "\t", splited);
      bayon::Document doc(id++);
      for (size_t i = 1; i < splited.size(); i++) {
        doc.add_feature(i-1, atof(splited[i].c_str()));
      }
      analyzer_.add_document(doc);
    }
    /*
    for (size_t i = 0; i < analyzer_.documents().size(); i++) {
      bayon::Document *doc1 = analyzer_.documents()[i];
      bayon::VecHashMap *hmap = doc1->feature()->hash_map();
      for (bayon::VecHashMap::iterator it = hmap->begin();
           it != hmap->end(); ++it) {
        printf("%ld\t%f\n", it->first, it->second);
      }
    }
    */
    analyzer_.idf();
    analyzer_.set_eval_limit(CLUSTER_LIMIT);
    analyzer_.do_clustering(bayon::Analyzer::RB);
  }

  void get_bof() {
  }
};

} /* namespace bof */

#endif  // BOF_VISUAL_WORDS_H_
