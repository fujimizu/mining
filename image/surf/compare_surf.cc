#include <cmath>
#include <iostream>
#include <vector>
#include <cv.h>
#include <highgui.h>

class Surf {
 private:
  double euclid_distance(const float *vec1, const float *vec2, int length) {
    double sum = 0.0;
    for (int i = 0; i < length; i++) {
      sum += (vec1[i] - vec2[i]) * (vec1[i] - vec2[i]);
    }
    return sqrt(sum);
  }
 
 public:
  Surf() { }
  ~Surf() { }

  void get_surf(const char *filename, CvSeq *&keypoints, CvSeq *&descriptors) {
    CvMemStorage *storage = cvCreateMemStorage(0);
    CvSURFParams params = cvSURFParams(500, 1);
    IplImage *img = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
    cvExtractSURF(img, 0, &keypoints, &descriptors, storage, params);
  }

  int nearest_neighbor(const float *vec, int laplacian,
                       const CvSeq *keypoints, const CvSeq* descriptors) {
    int length = (int) (descriptors->elem_size / sizeof(float));
    int neighbor = -1;
    double min_dist = 1e6;
    double secmin_dist = 1e6;
    for (int i = 0; i < descriptors->total; i++) {
      const CvSURFPoint *point = (const CvSURFPoint *)cvGetSeqElem(keypoints, i);
      if (laplacian != point->laplacian) continue;
      const float *v = (const float *)cvGetSeqElem(descriptors, i);
      double dist = euclid_distance(vec, v, length);
      if (dist < min_dist) {
        secmin_dist = min_dist;
        min_dist = dist;
        neighbor = i;
      } else if (dist < secmin_dist) {
        secmin_dist = dist;
      }
    }
    if (min_dist < 0.6 * secmin_dist) {
      return neighbor;
    }
    return -1;
  }

  void find_pairs(const CvSeq *keypoints1, const CvSeq *descriptors1,
                  const CvSeq *keypoints2, const CvSeq *descriptors2,
                  std::vector<int> &pairs) {
    for (int i = 0; i < descriptors1->total; i++) {
      const CvSURFPoint *point = (const CvSURFPoint *)cvGetSeqElem(keypoints1, i);
      const float *descriptor = (const float *)cvGetSeqElem(descriptors1, i);
      int neighbor = nearest_neighbor(descriptor, point->laplacian,
                                      keypoints2, descriptors2);
      if (neighbor >= 0) {
        pairs.push_back(i);
        pairs.push_back(neighbor);
      }
    }
  }

};

static int run_pair(int argc, char **argv);
static int run_multi(int argc, char **argv);

int main(int argc, char **argv) {
  return run_multi(argc, argv);
//  return run_pair(argc, argv);
}

static int run_pair(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s imgname1 imgname2\n", argv[0]);
    exit(1);
  }
  CvSeq *keypoints1, *keypoints2;
  CvSeq *descriptors1, *descriptors2;
  Surf surf;

  surf.get_surf(argv[1], keypoints1, descriptors1);
  surf.get_surf(argv[2], keypoints2, descriptors2);

  std::vector<int> pairs;
  surf.find_pairs(keypoints1, descriptors1, keypoints2, descriptors2, pairs);
  printf("descriptors:\n");
  printf(" %s : %d\n", argv[1], descriptors1->total);
  printf(" %s : %d\n", argv[2], descriptors2->total);
  printf("match: %d / %d\n",
         static_cast<int>(pairs.size() / 2), descriptors1->total);
  return 0;
}

static int run_multi(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s target imgname1 ...\n", argv[0]);
    exit(1);
  }
  CvSeq *keypoints1, *keypoints2;
  CvSeq *descriptors1, *descriptors2;
  Surf surf;
  surf.get_surf(argv[1], keypoints1, descriptors1);

  for (int i = 2; i < argc; i++) {
    surf.get_surf(argv[i], keypoints2, descriptors2);

    std::vector<int> pairs;
    surf.find_pairs(keypoints1, descriptors1, keypoints2, descriptors2, pairs);
    printf("descriptors:\n");
    printf(" %s : %d\n", argv[1], descriptors1->total);
    printf(" %s : %d\n", argv[i], descriptors2->total);
    printf("match: %d / %d\n\n",
           static_cast<int>(pairs.size() / 2), descriptors1->total);
  }
  return 0;
}
