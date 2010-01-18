#include <sys/types.h>
#include <dirent.h>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

const static size_t SIZE = 128;

std::vector<std::string> splitAll(std::string s, std::string t) {
  std::vector<std::string> v;
  for (int p = 0; (p = s.find(t)) != s.npos; ) {
    v.push_back(s.substr(0, p));
    s = s.substr(p + t.size());
  }
  v.push_back(s);
  return v;
}

double euclid_distance_squared(const std::vector<double> &vec1,
                               const std::vector<double> &vec2) {
  assert(vec1.size() == vec2.size());
  double dist = 0.0;
  for (size_t i = 0; i < vec1.size(); i++) {
    dist += (vec1[i] - vec2[i]) * (vec1[i] - vec2[i]);
  }
  return dist;
}

std::string get_extension(const std::string filename) {
  size_t index = filename.rfind('.', filename.size());
  if (index != std::string::npos) {
    return filename.substr(index+1, filename.size()-1);
  }
  return "";
}

void read_centroids(const char *filename,
                    std::vector<std::vector<double> *> &centroids) {
  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", filename);
    exit(1);
  }
  std::string line;
  while (std::getline(ifs, line)) {
    std::vector<double> *centroid = new std::vector<double>;
    centroid->resize(SIZE);
    std::vector<std::string> splited = splitAll(line, "\t");
    for (size_t i = 1; i < splited.size(); i += 2) {
      (*centroid)[atoi(splited[i].c_str())] = atof(splited[i+1].c_str());
    }
    centroids.push_back(centroid);
  }
}

void read_sift(const char *filename,
               std::vector<std::vector<double> *> &features) {
  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", filename);
    exit(1);
  }
  std::string line;
  std::vector<double> vec;
  while (std::getline(ifs, line)) {
    if (line[0] == ' ') {
      std::vector<std::string> splited = splitAll(line, " ");
      for (size_t i = 0; i < splited.size(); i++) {
        if (!splited[i].empty()) {
          vec.push_back(atoi(splited[i].c_str()));
        }
      }
    } else if (!vec.empty()) {
      std::vector<double> *vec_copy = new std::vector<double>;
      std::copy(vec.begin(), vec.end(), std::back_inserter(*vec_copy));
      features.push_back(vec_copy);
      vec.clear();
    }
  }
}

size_t search_nearest_centroid(
  const std::vector<double> &feature,
  const std::vector<std::vector<double> *> &centroids) {
  size_t min_id = 0;
  double min_dist = 1000000;
  for (size_t i = 0; i < centroids.size(); i++) {
    double dist = euclid_distance_squared(feature, *centroids[i]);
    if (dist < min_dist) {
      min_id = i;
      min_dist = dist;
    }
  }
  return min_id;
}

void delete_features(std::vector<std::vector<double> *> &features) {
  for (size_t i = 0; i < features.size(); i++) {
    if (features[i]) delete features[i];
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s centroid dirname\n", argv[0]);
    exit(1);
  }
  std::vector<std::vector<double> *> centroids;
  read_centroids(argv[1], centroids);

  DIR *dp = opendir(argv[2]);
  if (!dp) {
    fprintf(stderr, "cannot open directory: %s\n", argv[2]);
    exit(1);
  }
  struct dirent *dent;
  size_t count = 0;
  while ((dent = readdir(dp)) != NULL) {
    std::string ext = get_extension(dent->d_name);
    if (ext != "sift") continue;
    char path[256];
    sprintf(path, "%s/%s", argv[2], dent->d_name);
    std::vector<std::vector<double> *> features;
    read_sift(path, features);

    std::map<size_t, size_t> histogram;
    for (size_t i = 0; i < features.size(); i++) {
      size_t id = search_nearest_centroid(*features[i], centroids);
      histogram[id]++;
    }
    printf("%s", dent->d_name);
    for (std::map<size_t, size_t>::iterator it = histogram.begin();
         it != histogram.end(); ++it) {
      printf("\t%d\t%d", it->first, it->second);
    }
    printf("\n");
    fprintf(stderr, "%d\t%s\n", ++count, path);
    delete_features(features);
  }
  closedir(dp);


  delete_features(centroids);
  return 0;
}
