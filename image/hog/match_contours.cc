#include <fstream>
#include <iostream>
#include <cv.h>
#include <highgui.h>

void compare_contours_c(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s targetimg txtfile\n", argv[0]);
    exit(1);
  }
  IplImage *target = cvLoadImage(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
  std::ifstream ifs(argv[2]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    IplImage *img = cvLoadImage(line.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
    double similarity = cvMatchShapes(target, img, CV_CONTOURS_MATCH_I3, 0);
    //double similarity = cvMatchShapes(target, img, CV_CONTOURS_MATCH_I3, 0);
    printf("%s\t%f\n", line.c_str(), similarity);
  }
}

void compare_contours(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s targetimg txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1], 0);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }
  cv::vector<cv::vector<cv::Point> > target_contours;
  cv::findContours(target, target_contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

  std::ifstream ifs(argv[2]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    cv::Mat img = cv::imread(line, 0);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", line.c_str());
      continue;
    }
    cv::vector<cv::vector<cv::Point> > contours;
    cv::findContours(target, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    double similarity = cv::matchShapes(target_contours, contours, CV_CONTOURS_MATCH_I3, 0);
    printf("%s\t%f\n", line.c_str(), similarity);
  }
}

void compare_grayimages(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s targetimg txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1], 0);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }

  std::ifstream ifs(argv[2]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    cv::Mat img = cv::imread(line, 0);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", line.c_str());
      continue;
    }
    double similarity = cv::matchShapes(target, img, CV_CONTOURS_MATCH_I3, 0);
    printf("%s\t%f\n", line.c_str(), similarity);
  }
}

int main(int argc, char **argv) {
  compare_contours_c(argc, argv);
  //compare_contours(argc, argv);
  //compare_grayimages(argc, argv);
  return 0;
}
