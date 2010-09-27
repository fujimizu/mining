//
// Local feature extractor
//
// Copyright(C) 2010  Mizuki Fujisawa <fujisawa@bayon.cc>
//

#ifndef BOF_DESCRIPTOR_H_
#define BOF_DESCRIPTOR_H_

#include <cassert>
#include <cstdio>
#include <cv.h>
#include <highgui.h>

namespace bof {

/**
 * This class extracts local features from input images
 * (virtual class)
 */
class FeatureDetector {
 public:
  /**
   * Extract local features.
   * @param path path of an image file
   * @param feature local features
   */
  virtual void extract(const char *path,
                       std::vector<std::vector<float> >&features) = 0;
};

/**
 * This class extract SURF features from input images
 */
class SurfDetector : public FeatureDetector {
 private:
  static const size_t DIM_DESCRIPTOR = 128;
  CvMemStorage *storage;

 public:
  /**
   * Constructor.
   */
  SurfDetector() : storage(NULL) {
    storage = cvCreateMemStorage(0);
  }

  /**
   * Destructor.
   */
  ~SurfDetector() {
    if (storage) cvReleaseMemStorage(&storage);
  }

  /**
   * Extract local features.
   * @param path path of an image file
   * @param feature local features
   */
  void extract(const char *path, std::vector<std::vector<float> > &features) {
    assert(path);
    CvSeq* keypoints;
    CvSeq *descriptors;
    CvSURFParams params = cvSURFParams(500, 1);
    IplImage *img = cvLoadImage(path, CV_LOAD_IMAGE_GRAYSCALE);
    if (img == NULL) {
      fprintf(stderr, "[Error] cannot open %s\n", path);
      return;
    }
    cvExtractSURF(img, 0, &keypoints, &descriptors, storage, params);
    cvReleaseImage(&img);
    if (keypoints->total == 0) return;

    for (int i = 0; i < keypoints->total; i++) {
      std::vector<float> feature;
      float *descriptor = (float *)cvGetSeqElem(descriptors, i);
      std::copy(descriptor, descriptor + DIM_DESCRIPTOR,
                std::back_inserter(feature));
      features.push_back(feature);
    }
  }
};

/**
 * This class extract SIFT features from input images
 */
class SiftDetector : public FeatureDetector {
  /**
   * Extract local features.
   * @param path path of an image file
   * @param feature local features
   */
  void extract(const char *path, std::vector<std::vector<float> > &features) { }
};

} /* namespace bof */

#endif  // BOF_DESCRIPTOR_H_
