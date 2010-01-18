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
  if (argc < 4) {
    fprintf(stderr, "Usage: %s txtfile surf surfid", argv[0]);
    exit(1);
  }
  std::ifstream ifs(argv[1]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[1]);
    exit(1);
  }
  FILE *surffp = fopen(argv[2], "w");
  if (!surffp) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  FILE *idfp = fopen(argv[3], "w");
  if (!idfp) {
    fprintf(stderr, "cannot open %s\n", argv[3]);
    exit(1);
  }

  CvMemStorage *storage = cvCreateMemStorage(0);
  size_t count = 0;
  uint64_t descid = 0;
  std::string line;
  while (std::getline(ifs, line)) {
    if (!line.empty()) {
      cvClearMemStorage(storage);
      CvSeq *keypoints;
      CvSeq *descriptors;
      get_surf(line.c_str(), storage, keypoints, descriptors);
      if (keypoints->total == 0) continue;

      fputs(line.c_str(), idfp);
      for (int i = 0; i < keypoints->total; i++) {
        fprintf(idfp, "\t%d", descid);
        fprintf(surffp, "%d", descid);
        descid++;
        float *descriptor = (float *)cvGetSeqElem(descriptors, i);
        for (size_t j = 0; j < NUM_VECTOR; j++) {
          if (fabs(descriptor[j]) > 0.0001) { // threshold
            fprintf(surffp, "\t%d\t%.4f", j, descriptor[j]);
          }
        }
        fputs("\n", surffp);
      }
      fputs("\n", idfp);
    }
    printf("(%d)\t%s\n", ++count, line.c_str());
  }
  cvReleaseMemStorage(&storage);
  return 0;
}
