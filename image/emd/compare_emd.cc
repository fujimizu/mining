//
// Calculate the distance between images using Earth Mover's Distance(EMD)
//
// Build:
// % g++ -I /usr/local/include/opencv -lcv -lhighgui -Wall -O2 -fopenmp compare_emd.cc -o compare_emd
//
// Requirement:
//  - OpenCV
//  - OpenMP
//

#include <fstream>
#include <iostream>
#include <string>
#include <cv.h>
#include <highgui.h>
#include <omp.h>

std::string get_extension(const std::string filename) {
  size_t index = filename.rfind('.', filename.size());
  if (index != std::string::npos) {
    return filename.substr(index+1, filename.size()-1);
  }
  return "";
}

class EmdCalc {
 private:
  virtual CvMat *get_signature(const cv::MatND &hist) = 0;
  virtual void get_histogram(const cv::Mat &img, cv::MatND &hist) = 0;

 public:
  void print_emd(const cv::Mat &target, const std::vector<std::string> &filenames) {
    cv::MatND hist_target;
    get_histogram(target, hist_target);
    CvMat *sig_target = get_signature(hist_target);

    int size = static_cast<int>(filenames.size());
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
      cv::Mat img = cv::imread(filenames[i]);
      if (!img.data) {
        fprintf(stderr, "file open error: %s\n", filenames[i].c_str());
        continue;
      }
      cv::MatND hist;
      get_histogram(img, hist);
      CvMat *sig = get_signature(hist);
      if (!sig) {
        fprintf(stderr, "cannot get signature: %s", filenames[i].c_str());
        continue;
      }
      try {
        float emd = cvCalcEMD2(sig_target, sig, CV_DIST_L2);
        printf("%s\t%f\n", filenames[i].c_str(), emd);
      } catch (cv::Exception e) {
        fprintf(stderr, "error: %s : %s:\n", e.err.c_str(), filenames[i].c_str());
        continue;
      }
      cvReleaseMat(&sig);
      fprintf(stderr, "%d\t%s\n", i+1, filenames[i].c_str());
    }
    cvReleaseMat(&sig_target);
  }
};

class EmdCalcHsv : public EmdCalc{
 private:
  int h_bins;
  int s_bins;

  CvMat *get_signature(const cv::MatND &hist) {
    CvMat *sig = cvCreateMat(h_bins * s_bins, 3, CV_32FC1);
    for (int h = 0; h < h_bins; h++) {
      for (int s = 0; s < s_bins; s++) {
        float bin_val = hist.at<float>(h, s);
        cvSet2D(sig, h * s_bins + s, 0, cvScalar(bin_val));
        cvSet2D(sig, h * s_bins + s, 1, cvScalar(h));
        cvSet2D(sig, h * s_bins + s, 2, cvScalar(s));
      }
    }
    return sig;
  }

  void get_histogram(const cv::Mat &img, cv::MatND &hist) {
    cv::Mat hsv;
    cvtColor(img, hsv, CV_BGR2HSV);
    int histSize[] = {h_bins, s_bins};
    float hranges[] = {0, 180};
    float sranges[] = {0, 255};
    const float *ranges[] = {hranges, sranges};
    int channels[] = {0, 1};

    cv::calcHist(&img, 1, channels, cv::Mat(),
                 hist, 2, histSize, ranges, true, false);
    cv::normalize(hist, hist, 1, 0, cv::NORM_L1);
  }

 public:
  EmdCalcHsv(int h_bins, int s_bins) : h_bins(h_bins), s_bins(s_bins) { }
  ~EmdCalcHsv() { }
};

class EmdCalcLuv : public EmdCalc {
 private:
  int l_bins;
  int u_bins;
  int v_bins;

  void get_histogram(const cv::Mat &img, cv::MatND &hist) {
    cv::Mat luv;
    cvtColor(img, luv, CV_BGR2Luv);
    int histSize[] = {l_bins, u_bins, v_bins};
    float l_ranges[] = {0, 255};
    float u_ranges[] = {0, 255};
    float v_ranges[] = {0, 255};
    const float *ranges[] = {l_ranges, u_ranges, v_ranges};
    int channels[] = {0, 1, 2};

    cv::calcHist(&img, 1, channels, cv::Mat(),
                 hist, 3, histSize, ranges, true, false);
    cv::normalize(hist, hist, 1, 0, cv::NORM_L1);
  }

  CvMat *get_signature(const cv::MatND &hist) {
    CvMat *sig = cvCreateMat(l_bins * u_bins * v_bins, 4, CV_32FC1);
    for (int l = 0; l < l_bins; l++) {
      for (int u = 0; u < u_bins; u++) {
        for (int v = 0; v < v_bins; v++) {
          float bin_val = hist.at<float>(l, u, v);
          cvSet2D(sig, l * u_bins * v_bins + u * v_bins + v, 0, cvScalar(bin_val));
          cvSet2D(sig, l * u_bins * v_bins + u * v_bins + v, 1, cvScalar(l));
          cvSet2D(sig, l * u_bins * v_bins + u * v_bins + v, 2, cvScalar(u));
          cvSet2D(sig, l * u_bins * v_bins + u * v_bins + v, 3, cvScalar(v));
        }
      }
    }
    return sig;
  }

 public:
  EmdCalcLuv(int l_bins, int u_bins, int v_bins)
    : l_bins(l_bins), u_bins(u_bins), v_bins(v_bins) {}
  ~EmdCalcLuv() { }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s imgfile txtfile\n", argv[0]);
    fprintf(stderr, "Usage: %s imgfile imgfile1 imgfile2 ..\n", argv[0]);
    exit(1);
  }
  cv::Mat target = cv::imread(argv[1]);
  if (!target.data) {
    fprintf(stderr, "file open error: %s\n", argv[1]);
    exit(1);
  }
  std::vector<std::string> filenames;
  if (get_extension(argv[2]) == "txt") {
    std::ifstream ifs(argv[2]);
    if (!ifs) {
      fprintf(stderr, "cannot open %s\n", argv[2]);
      exit(1);
    }
    std::string line;
    while (std::getline(ifs, line)) {
      filenames.push_back(line);
    }
  } else {
    for (int i = 2; i < argc; i++) {
      filenames.push_back(argv[i]);
    }
  }

  //EmdCalcHsv calc(6, 6);
  //EmdCalcHsv calc(30, 32);
  EmdCalcLuv calc(6, 6, 6);
  calc.print_emd(target, filenames);

  return 0;
}
