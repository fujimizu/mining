#include <cassert>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
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

template<typename KeyType, typename ValueType>
bool greater_pair(const std::pair<KeyType, ValueType> &left,
                  const std::pair<KeyType, ValueType> &right) {
  if (left.second > right.second) {
    return true;
  } else if (left.second == right.second) {
    return left.first > right.first;
  } else {
    return false;
  }
}

template<typename KeyType, typename ValueType>
bool less_pair(const std::pair<KeyType, ValueType> &left,
                  const std::pair<KeyType, ValueType> &right) {
  if (left.second < right.second) {
    return true;
  } else if (left.second == right.second) {
    return left.first < right.first;
  } else {
    return false;
  }
}

void calc_hog(const cv::HOGDescriptor &hog, const cv::Mat &img,
                std::vector<float> &descriptors) {
  cv::Mat resized;
//  printf("%d\t%d\n", hog.winSize.width, hog.winSize.height);
  cv::resize(img, resized, hog.winSize);
  hog.compute(resized, descriptors);
}

void print_distances(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s targetimg txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }

  cv::Size winSize(40, 40);
  cv::Size blockSize(8, 8);
  cv::Size blockStride(4, 4);
  cv::Size cellSize(4, 4);
  int nbins = 9;

  cv::HOGDescriptor hog;
  //cv::HOGDescriptor hog(winSize, blockSize, blockStride, cellSize, nbins);
  std::vector<float> target_desc;
  calc_hog(hog, target, target_desc);

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
    std::vector<float> desc;
    calc_hog(hog, img, desc);
    //double dist = inner_product(target_desc, desc);
    double dist = cosine(target_desc, desc);
    //double dist = euclid_distance(target_desc, desc);
    printf("%s\t%f\n", line.c_str(), dist);
  }
}

void calc_distances(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s targetimg imgfile1 imgfile2 ...\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }

  cv::HOGDescriptor hog;
  std::vector<float> target_desc;
  calc_hog(hog, target, target_desc);

  std::vector<std::pair<std::string, double> > distances;
  for (int i = 2; i < argc; i++) {
    cv::Mat img = cv::imread(argv[i]);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", argv[i]);
      continue;
    }
    std::vector<float> desc;
    calc_hog(hog, img, desc);
    double dist = euclid_distance(target_desc, desc);
    distances.push_back(std::pair<std::string, double>(argv[i], dist));
  }

  size_t max = 10;
  std::sort(distances.begin(), distances.end(),
            less_pair<std::string, double>);
  for (size_t i = 0; i < max && i < distances.size(); i++) {
    printf("%s\t%f\n", distances[i].first.c_str(), distances[i].second);
  }
}

void calc_distances_text(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s targetimg txtfile\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }

  cv::HOGDescriptor hog;
  std::vector<float> target_desc;
  calc_hog(hog, target, target_desc);

  std::vector<std::pair<std::string, double> > distances;
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
    std::vector<float> desc;
    calc_hog(hog, img, desc);
    double dist = euclid_distance(target_desc, desc);
    distances.push_back(std::pair<std::string, double>(line, dist));
  }

  size_t max = 10;
  std::sort(distances.begin(), distances.end(),
            less_pair<std::string, double>);
  for (size_t i = 0; i < max && i < distances.size(); i++) {
    printf("%s\t%f\n", distances[i].first.c_str(), distances[i].second);
  }
}

int main(int argc, char **argv) {
  print_distances(argc, argv);
//  calc_distances(argc, argv);
//  calc_distances_text(argc, argv);
  return 0;
}
