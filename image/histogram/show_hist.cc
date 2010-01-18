#include <iostream>
#include <cv.h>
#include <highgui.h>

void get_histogram_hsv(const cv::Mat &img, cv::MatND &hist) {
  cv::Mat hsv;
  cv::cvtColor(img, hsv, CV_BGR2HSV);
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

void compute_color_hist(const cv::Mat &img, cv::MatND &hist, int N) {
  const int histSize[] = {N, N, N};
  hist.create(3, histSize, CV_32F);
  hist = cv::Scalar(0);
  CV_Assert(img.type() == CV_8UC3);
  cv::MatConstIterator_<cv::Vec3b> it = img.begin<cv::Vec3b>(),
                           it_end = img.end<cv::Vec3b>();
  for (; it != it_end; ++it) {
    const cv::Vec3b &pix = *it;
    hist.at<float>(pix[0] * N / 256, pix[1] * N / 256, pix[2] * N / 256) += 1.f;
  }
  cv::normalize(hist, hist, 1, 0, cv::NORM_L2);
}

void print_histogram(const cv::MatND &hist) {
  cv::NAryMatNDIterator it(hist);
  for (int i = 0; i < it.nplanes; i++, ++it) {
    cv::Mat mat = it.planes[0];
    for (int j = 0; j < mat.rows; j++) {
      for (int k = 0; k < mat.cols; k++) {
        if (k != 0) printf("\t");
        printf("%.3f", mat.ptr<double>(j)[k]);
      }
      printf("\n");
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s imgfile ...\n", argv[0]);
    exit(1);
  }
  for (int i = 1; i < argc; i++) {
    cv::Mat img = cv::imread(argv[i]);
    if (!img.data) {
      fprintf(stderr, "file open error: %s\n", argv[i]);
      exit(1);
    }
    cv::MatND hist;
    compute_color_hist(img, hist, 16);
    //get_histogram_hsv(img, hist);
    print_histogram(hist);
  }
  return 0;
}
