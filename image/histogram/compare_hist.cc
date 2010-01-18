#include <cmath>
#include <fstream>
#include <iostream>
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>

double euclid_distance(const std::vector<float> &vec1,
                       const std::vector<float> &vec2) {
  assert(vec1.size() == vec2.size());
  double dist = 0.0;
  for (size_t i = 0; i < vec1.size(); i++) {
    dist += (vec1[i] - vec2[i]) * (vec1[i] - vec2[i]);
  }
  return sqrt(dist);
}

double norm(const std::vector<float> &vec) {
  double nrm = 0.0;
  for (size_t i = 0; i < vec.size(); i++) {
    nrm += vec[i] * vec[i];
  }
  return sqrt(nrm);
}

double inner_product(const std::vector<float> &vec1,
                     const std::vector<float> &vec2) {
  assert(vec1.size() == vec2.size());
  double prod = 0.0;
  for (size_t i = 0; i < vec1.size(); i++) {
    prod += vec1[i] * vec2[i];
  }
  return prod;
}

double cosine(const std::vector<float> &vec1,
              const std::vector<float> &vec2) {
  assert(vec1.size() == vec2.size());
  double nrm1 = norm(vec1);
  double nrm2 = norm(vec2);
  if (!nrm1 || !nrm2) return 0;
  double prod = inner_product(vec1, vec2);
  return prod / (nrm1 * nrm2);
}

double cosine_distance(const std::vector<float> &vec1,
                       const std::vector<float> &vec2) {
  return 1.0 - cosine(vec1, vec2);
}

void calc_hog(const cv::HOGDescriptor &hog, const cv::Mat &img,
                std::vector<float> &descriptors) {
  cv::Mat resized;
//  printf("%d\t%d\n", hog.winSize.width, hog.winSize.height);
  cv::resize(img, resized, hog.winSize);
  hog.compute(resized, descriptors);
}

void get_histogram_hsv(const cv::Mat &img, cv::MatND &hist) {
  cv::Mat hsv;
  cvtColor(img, hsv, CV_BGR2HSV);
  int hbins = 30;
  int sbins = 32;
  int histSize[] = {hbins, sbins};
  float hranges[] = {0, 180};
  float sranges[] = {0, 256};
  const float *ranges[] = {hranges, sranges};
  int channels[] = {0, 1};

  cv::calcHist(&img, 1, channels, cv::Mat(),
               hist, 2, histSize, ranges, true, false);
  cv::normalize(hist, hist, 1, 0, cv::NORM_L1);
}

void get_histogram_rgb(const cv::Mat &img, cv::MatND &hist) {
  int n = 16;
  int histSize[] = {n, n, n, n};
  float range[] = {0, 256};
  const float *ranges[] = {range, range, range, range};
  int channels[] = {0, 1};
  cv::calcHist(&img, 1, channels, cv::Mat(),
               hist, 4, histSize, ranges, true, false);
  cv::normalize(hist, hist, 1, 0, cv::NORM_L1);
}

void compare_hist_hsv(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s imgfile txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }
  cv::MatND hist_target;
  get_histogram_hsv(target, hist_target);

  std::ifstream ifs(argv[2]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    cv::Mat img = cv::imread(line);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", line.c_str());
      continue;
    }
    cv::MatND hist;
    //get_histogram_rgb(img, hist);
    get_histogram_hsv(img, hist);
    double similarity = cv::compareHist(hist_target, hist, CV_COMP_BHATTACHARYYA);
    //double similarity = cv::compareHist(hist_target, hist, CV_COMP_INTERSECT);
    printf("%s\t%f\n", line.c_str(), similarity);
  }
}

void compare_hist_hog(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s imgfile txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }

  cv::MatND hist_target;
  get_histogram_hsv(target, hist_target);

  cv::HOGDescriptor hog;
  std::vector<float> desc_target;
  calc_hog(hog, target, desc_target);

  std::ifstream ifs(argv[2]);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", argv[2]);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    cv::Mat img = cv::imread(line);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", line.c_str());
      continue;
    }
    cv::MatND hist;
    get_histogram_hsv(img, hist);
    double sim_hist = 1.0 - cv::compareHist(hist_target, hist, CV_COMP_BHATTACHARYYA);
    std::vector<float> desc;
    calc_hog(hog, img, desc);
    double sim_hog = cosine(desc_target, desc);
    double similarity = 0.3 * sim_hist + 1.5 * sim_hog;

    printf("%s\t%f\t%f\t%f\n", line.c_str(), similarity, sim_hist, sim_hog);
  }
}

int main(int argc, char **argv) {
  //compare_hist_hsv(argc, argv);
  compare_hist_hog(argc, argv);
  return 0;
}
