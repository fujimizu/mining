//
// LSH(Locality Sensitive Hashing)
// using LSHKIT (http://lshkit.sourceforge.net/)
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#ifndef BOF_LSH_H_
#define BOF_LSH_H_

#include <vector>
#include <lshkit.h>

namespace bof {

/**
 * Locality sensitive hashing using lshkit
 */
class Lsh {
 private:
  typedef lshkit::Repeat<lshkit::ThresholdingLsh> MyLsh;
  typedef lshkit::Histogram<MyLsh> MyHistogram;

  static const size_t M = 10;  ///< number repeated to take average
  static const size_t N = 10;  ///< number of concatenated histograms

  MyLsh::Parameter param_;  ///< parameter for LSH

 public:
  Lsh(size_t repeat, size_t dim, double min, double max) {
    param_.repeat = repeat;
    param_.dim = dim;
    param_.min = min;
    param_.max = max;
  }
  ~Lsh() { }

  void hash(const std::vector<std::vector<float> > &features,
            std::vector<size_t> &values) {
    MyHistogram hist;
    lshkit::DefaultRng rng;  // random number generator
    hist.reset(M, N, param_, rng);
    float *output = new float[hist.dim()];
    hist.zero(output);
    for (size_t i = 0; i < features.size(); i++) {
      hist.add(output, &features[i][0]);
    }
    values.resize(hist.dim());
    for (size_t i = 0; i < hist.dim(); i++) {
      if (output[i]) values[i] = static_cast<size_t>(output[i]);
    }
    delete [] output;
  }
};

} /* namespace bof */

#endif  // BOF_LSH_H_
