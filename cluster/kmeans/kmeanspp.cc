/**
 * k-means++
 * (Ref: http://www.stanford.edu/~darthur/kMeansPlusPlus.pdf)
 *
 * Usage:
 *  kmeanpp -i inputdb -o txt -n ncenters
 *    -i, --input dbm  ... input TCHDB file
 *    -o, --output txt ... output text file
 *    -n, --number n   ... number of centers (n > 0)
 *
 * Requirement:
 *  - Tokyo Cabinet (http://tokyocabinet.sourceforge.net/)
 *
 * Build:
 *  % g++ `tcucodec conf -l` kmeanspp.cc -o kmeanspp
 *
 */

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <tchdb.h>
#include <ext/hash_map>

using namespace std;

struct stringhash { // hash function for string
  size_t operator()(const string& str) const {
    const char *ptr = str.data();
    int size = str.size();
    size_t idx = 19780211;
    while(size--){
      idx = idx * 37 + *(uint8_t *)ptr++;
    }
    return idx;
  }
};

//typedef map<string, double> Vector;
//typedef map<string, double>::iterator VecIt;
typedef __gnu_cxx::hash_map<string, double, stringhash> Vector;
typedef __gnu_cxx::hash_map<string, double, stringhash>::iterator VecIt;

/* function prototypes */
int main(int, char **);
void usage_exit();
void print_vector(Vector &);
void add_vector(Vector &, Vector &);
void mult_each(Vector &, double);
double squared_dist(Vector &, Vector &);
double product(Vector &, Vector &);
double length(Vector &);
double cosine(Vector &, Vector &);
double cosine_dist(Vector &, Vector &);
void parse_dbmdata(const char *, Vector &);
void choose_random_centers(TCHDB *, vector<Vector> &);
void choose_smart_centers(TCHDB *, vector<Vector> &);
void assign_clusters(TCHDB *, map<string, int> &, vector<Vector> &);
void move_centers(TCHDB *, map<string, int> &, vector<Vector> &);
void kmeans(TCHDB *, map<string, int> &, vector<Vector> &);
void save_clusters(map<string, int> &, const char *);

/* Constants */
const int MAX_ITERATION = 10;


int main(int argc, char **argv) {
  int opt;
  int ncenters = 0;
  char *input  = NULL;
  char *output = NULL;
  while ((opt = getopt(argc, argv, "n:i:o:")) != -1) {
    switch (opt) {
    case 'i':
      input = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    case 'n':
      ncenters = atoi(optarg);
      break;
    }
  }
  if (input == NULL || output == NULL || ncenters <= 0) usage_exit();

  cout << "Open input dbm and output text file" << endl;
  TCHDB *vecdb = tchdbnew();
  if (!tchdbopen(vecdb, input, HDBOREADER)) {
    int ecode = tchdbecode(vecdb);
    cerr << "cannot open dbm: " << tchdberrmsg(ecode) << endl;
    exit(1);
  }

  cout << "Choose initial centers" << endl;
  vector<Vector> centers(ncenters);
  choose_smart_centers(vecdb, centers);
  //choose_random_centers(vecdb, centers);
//  for (unsigned int i = 0; i < centers.size(); i++) {
//    cout << "center-" << i << "\t";
//    print_vector(centers[i]);
//  }

  cout << "Do k-means clustering" << endl;
  map<string, int> assign;
  kmeans(vecdb, assign, centers);

  cout << "Save clusters" << endl;
  save_clusters(assign, output);

  tchdbdel(vecdb);
  return 0;
}

void usage_exit() {
  cerr << "Usage: " << endl
       << " kmeanpp -i inputdb -o outputdb -n ncenters"       << endl
       << "   -i, --input dbm  ... input TCHDB file"          << endl
       << "   -o, --output dbm ... output TCHDB file"         << endl
       << "   -n, --number n   ... number of centers (n > 0)" << endl;
  exit(1);
}

void print_vector(Vector &vec) {
  VecIt it = vec.begin();
  while (it != vec.end()) {
    cout << it->first << ":" << it->second << " ";
    ++it;
  }
  cout << endl;
}

void add_vector(Vector &vec1, Vector &vec2) {
  VecIt it = vec2.begin();
  while (it != vec2.end()) {
    string key = it->first;
    double point = it->second;
    vec1[key] += point;
    ++it;
  }
}

void mult_each(Vector &vec, double x) {
  VecIt it = vec.begin();
  while (it != vec.end()) {
    vec[it->first] = it->second * x;
    ++it;
  }
}

void parse_dbmdata(char *data, Vector &vec) {
  istringstream iss(data);
  string token;
  string word;
  int cnt = 0;
  while (iss >> token) {
    if (cnt % 2 == 0) {
      word = token;
    } else {
      double point = atof(token.c_str());
      vec[word] = point;
    }
    ++cnt;
  }
}

double length(Vector &vec) {
  VecIt it = vec.begin();
  double sum = 0;
  while (it != vec.end()) {
    double point = it->second;
    sum += point * point;
    ++it;
  }
  return sqrt(sum);
}

double squared_dist(Vector &vec1, Vector &vec2) {
  map<string, bool> check;
  double dist = 0;
  VecIt it = vec1.begin();
  while (it != vec1.end()) {
    string key = it->first;
    double p1 = vec1[key];
    double p2 = vec2[key];
    dist += (p1 - p2) * (p1 - p2);
    check[key] = true;
    ++it;
  }
  it = vec2.begin();
  while (it != vec2.end()) {
    string key = it->first;
    if (!check[key]) {
      double p1 = vec1[key];
      double p2 = vec2[key];
      dist += (p1 - p2) * (p1 - p2);
      check[key] = true;
    }
    ++it;
  }
  return dist;
}

double product(Vector &vec1, Vector &vec2) {
  VecIt it, end;
  if (vec1.size() < vec2.size()) {
    it = vec1.begin();
    end = vec1.end();
  } else {
    it = vec2.begin();
    end = vec2.end();
  }
  double prod = 0;
  while (it != end) {
    string key = it->first;
    double point1 = vec1[key];
    double point2 = vec2[key];
    prod += point1 * point2;
    ++it;
  }
  return prod;
}

double cosine(Vector &vec1, Vector &vec2) {
  double len1 = length(vec1);
  double len2 = length(vec2);
  if (len1 == 0 || len2 == 0) return 0;

  double prod = product(vec1, vec2);
  double result = prod / (len1 * len2);
  if (isnan(result)) {
    return 0;
  } else {
    return result;
  }
}

double cosine_dist(Vector &vec1, Vector &vec2) {
  return 1 - cosine(vec1, vec2);
}

void choose_random_centers(TCHDB *vecdb, vector<Vector> &centers) {
  map<int, bool> idxmap;
  int rnum = tchdbrnum(vecdb);
  unsigned int ncenters = 0;
  srand((unsigned) time(NULL));
  while (ncenters < centers.size()) {
    int idx = rand() % rnum;
    if (idxmap.find(idx) == idxmap.end()) {
      idxmap[idx] = true;
      ++ncenters;
    }
  }

  ncenters = 0;
  int idx = 0;
  char *key;
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    if (idxmap[idx]) {
      char *value = tchdbget2(vecdb, key);
      free(key);
      if (value == NULL) continue;
      parse_dbmdata(value, centers[ncenters]);
      free(value);
      ++ncenters;
    } else {
      free(key);
    }
    ++idx;
  }
}

void choose_smart_centers(TCHDB *vecdb, vector<Vector> &centers) {
  map<int, bool> idxmap;
  map<string, double> closest_dist;
  int rnum = tchdbrnum(vecdb);
  double potential = 0;
  unsigned int ncenters = 0;
  char *key, *value;

  /* choose one random center */
  srand((unsigned) time(NULL));
  int initidx = rand() % rnum;
  int idx = 0;
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    if (idx == initidx) {
      value = tchdbget2(vecdb, key);
      free(key);
      if (value == NULL) continue;
      parse_dbmdata(value, centers[ncenters]);
      free(value);
      ++ncenters;
      break;
    } else {
      free(key);
    }
    ++idx;
  }

  /* update closest distance */
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    value = tchdbget2(vecdb, key);
    if (value == NULL) {
      free(key);
      continue;
    }
    Vector vec;
    parse_dbmdata(value, vec);
    free(value);
    //double dist = squared_dist(vec, centers[0]);
    double dist = cosine_dist(vec, centers[0]);
    closest_dist[key] = dist;
    potential += dist;
    free(key);
  }

  /* choose each center */
  while (ncenters < centers.size()) {
    double randval = static_cast<double>(rand()) / RAND_MAX * potential;
    string centkey = "";
    tchdbiterinit(vecdb);
    while ((key = tchdbiternext2(vecdb)) != NULL) {
      centkey = key;
      double dist = closest_dist[key];
      free(key);
      if (randval <= dist) break;
      randval -= dist;
    }
    Vector centvec;
    value = tchdbget2(vecdb, centkey.c_str());
    if (value == NULL) continue;
    parse_dbmdata(value, centvec);
    free(value);

    double newpotential = 0;
    tchdbiterinit(vecdb);
    while ((key = tchdbiternext2(vecdb)) != NULL) {
      value = tchdbget2(vecdb, key);
      if (value == NULL) {
        free(key);
        continue;
      }
      Vector vec;
      parse_dbmdata(value, vec);
      free(value);
      //double dist = squared_dist(vec, centvec);
      double dist = cosine_dist(vec, centvec);
      closest_dist[key] = dist;
      newpotential += dist;
      free(key);
    }
    centers[ncenters++] = centvec;
    potential = newpotential;
    cout << " center No." << ncenters << endl;
  }
}

void assign_clusters(TCHDB *vecdb, map<string, int> &assign, vector<Vector> &centers) {
  char *key, *value;
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    value = tchdbget2(vecdb, key);
    if (value == NULL) {
      free(key);
      continue;
    }
    Vector vec;
    parse_dbmdata(value, vec);
    free(value);
    double mindist = -1;
    int minidx = 0;
    for (unsigned int i = 0; i < centers.size(); ++i) {
      //double dist = squared_dist(vec, centers[i]);
      double dist = cosine_dist(vec, centers[i]);
      if (mindist < 0 || mindist > dist) {
        mindist = dist;
        minidx = i;
      }
    }
    assign[key] = minidx;
    free(key);
  }
}

void move_centers(TCHDB *vecdb, map<string, int> &assign, vector<Vector> &centers) {
  vector<vector<string> > clusters(centers.size());
  map<string, int>::iterator it = assign.begin();
  while (it != assign.end()) {
    string key = it->first;
    int idx = it->second;
    clusters[idx].push_back(key);
    ++it;
  }
  for (unsigned int i = 0; i < centers.size(); ++i) {
    if (clusters[i].size() == 0) continue;
    centers[i].clear();
    for (unsigned int j = 0; j < clusters[i].size(); ++j) {
      string key = clusters[i][j];
      char *value = tchdbget2(vecdb, key.c_str());
      if (value == NULL) continue;
      Vector vec;
      parse_dbmdata(value, vec);
      free(value);
      add_vector(centers[i], vec);
    }
    mult_each(centers[i], static_cast<double>(1) / clusters[i].size());
  }
}

void kmeans(TCHDB *vecdb, map<string, int> &assign, vector<Vector> &centers) {
  assign_clusters(vecdb, assign, centers);

  for (int i = 0; i < MAX_ITERATION; ++i) {
    cout << " k-kmeans loop No." << i+1 << endl;
    move_centers(vecdb, assign, centers);
//    for (unsigned int j = 0; j < centers.size(); j++) {
//      cout << "center-" << j << "\t";
//      print_vector(centers[j]);
//    }

    map<string, int> newassign;
    assign_clusters(vecdb, newassign, centers);
    map<string, int>::iterator it = assign.begin();
    bool changed = false;
    while (it != assign.end()) {
      string key = it->first;
      if (assign[key] != newassign[key]) {
        changed = true;
        break;
      }
      ++it;
    }
    if (!changed) break;
    assign = newassign;
  }
}

void save_clusters(map<string, int> &assign, const char *path) {
  ofstream ofs(path);
  map<string, int>::iterator it = assign.begin();
  while (it != assign.end()) {
    ofs << it->second << "\t" << it->first << endl;
    ++it;
  }
}
