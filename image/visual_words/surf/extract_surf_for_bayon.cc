//
// Extract SURF features from images using OpenCV
//
// Build:
//  % g++ extract_surf_for_bayon.cc -o extract_surf -Wall -O3 -I/usr/include/opencv -lcv -lhighgui
//
//

#include <cmath>
#include <fstream>
#include <iostream>
#include <cv.h>
#include <highgui.h>

const size_t NUM_VECTOR = 128;

void get_surf(const char *filename, CvMemStorage *storage,
              CvSeq *&keypoints, CvSeq *&descriptors) {
  CvSURFParams params = cvSURFParams(500, 1);
  IplImage *img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
  cvExtractSURF(img, 0, &keypoints, &descriptors, storage, params);
  cvReleaseImage(&img);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s surf_count\n", argv[0]);
    exit(1);
  }
  FILE *countfp = fopen(argv[1], "w");
  if (!countfp) {
    fprintf(stderr, "cannot open %s\n", argv[1]);
    exit(1);
  }

  CvMemStorage *storage = cvCreateMemStorage(0);
  size_t count = 0;
  uint64_t descid = 0;
  std::string line;
  while (std::getline(std::cin, line)) {
    if (!line.empty()) {
      cvClearMemStorage(storage);
      CvSeq *keypoints;
      CvSeq *descriptors;
      get_surf(line.c_str(), storage, keypoints, descriptors);
      if (keypoints->total == 0) continue;

      fprintf(countfp, "%s\t%d\n", line.c_str(), keypoints->total);
      for (int i = 0; i < keypoints->total; i++) {
        printf("%ld", descid++);
        float *descriptor = (float *)cvGetSeqElem(descriptors, i);
        for (size_t j = 0; j < NUM_VECTOR; j++) {
          if (fabs(descriptor[j]) > 0.0001) { // threshold
            printf("\t%ld\t%.4f", j, descriptor[j]);
          }
        }
        printf("\n");
      }
    }
    fprintf(stderr, "(%ld)\t%s\n", ++count, line.c_str());
  }
  cvReleaseMemStorage(&storage);
  return 0;
}
