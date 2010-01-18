//
// COP-KMEANS (Constrained K-means Algorithm)
// http://www.wkiri.com/research/cop-kmeans/
//

#include <cassert>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <vector>
#include <google/dense_hash_map>

typedef uint64_t VecKey;
typedef size_t VecId;
typedef google::dense_hash_map<VecKey, double> Vector;
typedef google::dense_hash_map<std::string, VecKey> KeyMap;

class KMeans;

/* function prototypes */
int main(int argc, char **argv);
void usage(const char *progname);
void read_vectors(const char *filename, KMeans &kmeans);
void read_constraints(const char *filename, KMeans &kmeans);
size_t splitstring(std::string s, const std::string &delimiter,
                   std::vector<std::string> &splited);

/* constants */
const size_t MAX_ITER  = 10;
const VecKey EMPTY_KEY = 0;
const double LONG_DIST = 1000000000000000;
const std::string DELIMITER("\t");

class KMeans {
 public:
  typedef google::dense_hash_map<std::string, size_t> LabelMap;
  typedef std::multimap<size_t, size_t> ConstraintMap;
  enum Constraint {
    CONSTRAINT_MUST,
    CONSTRAINT_CANNOT
  };

 private:
  std::vector<Vector *> vectors_;
  std::vector<Vector *> centers_;
  LabelMap labels_;
  ConstraintMap must_;
  ConstraintMap cannot_;

  double euclid_distance_squared(const Vector &vec1, const Vector &vec2) const {
    google::dense_hash_map<VecKey, bool> check;
    check.set_empty_key(EMPTY_KEY);
    double dist = 0.0;
    Vector::const_iterator it1, it2;
    for (it1 = vec1.begin(); it1 != vec1.end(); ++it1) {
      double val1 = it1->second;
      double val2 = 0.0;
      it2 = vec2.find(it1->first);
      if (it2 != vec2.end()) val2 = it2->second;
      dist += (val1 - val2) * (val1 - val2);
      check[it1->first] = true;
    }
    for (it2 = vec2.begin(); it2 != vec2.end(); ++it2) {
      if (check.find(it2->first) != check.end()) continue;
      double val2 = it2->second;
      double val1 = 0.0;
      it1 = vec1.find(it2->first);
      if (it1 != vec1.end()) val1 = it1->second;
      dist += (val1 - val2) * (val1 - val2);
    }
    return dist;
  }

  void choose_random_centers(size_t ncenters) {
    centers_.clear();
    google::dense_hash_map<size_t, bool> check;
    check.set_empty_key(vectors_.size());
    size_t cnt = 0;
    while (cnt < ncenters) {
      size_t index = rand() % vectors_.size();
      if (check.find(index) == check.end()) {
        Vector *center = new Vector(*vectors_[index]);
        centers_.push_back(center);
        cnt++;
        check[index] = true;
      }
    }
  }

  void choose_smart_centers(size_t ncenters) {
    centers_.clear();
    double closest_dist[vectors_.size()];
    double potential = 0.0;
    size_t cnt = 0;

    // choose one random center
    size_t index = rand() % vectors_.size();
    Vector *center = new Vector(*vectors_[index]);
    centers_.push_back(center);
    cnt++;
    // update closest distance
    for (size_t i = 0; i < vectors_.size(); i++) {
      double dist = euclid_distance_squared(*vectors_[i], *centers_[0]);
      closest_dist[i] = dist;
      potential += dist;
    }
    // choose each centers
    while (cnt < ncenters) {
      double randval = static_cast<double>(rand()) / RAND_MAX * potential;
      size_t index = 0;
      for (size_t i = 0; i < vectors_.size(); i++) {
        if (randval <= closest_dist[i]) {
          index = i;
          break;
        } else {
          randval -= closest_dist[i];
        }
      }
      Vector *center = new Vector(*vectors_[index]);
      double potential_new = 0.0;
      for (size_t i = 0; i < vectors_.size(); i++) {
        double dist = euclid_distance_squared(*vectors_[i], *center);
        if (closest_dist[i] > dist) closest_dist[i] = dist;
        potential_new += closest_dist[i];
      }
      centers_.push_back(center);
      cnt++;
      potential = potential_new;
    }
  }

  void assign_clusters(size_t *assign) const {
    // clear assignments
    size_t init_index = centers_.size();
    for (size_t i = 0; i < vectors_.size(); i++) {
      assign[i] = init_index;
    }
    google::dense_hash_map<size_t, bool> cannot_cluster;
    cannot_cluster.set_empty_key(init_index);
    for (size_t i = 0; i < vectors_.size(); i++) {
      size_t min_index = init_index;
      double min_dist = LONG_DIST;
      // check must constraint
      for (ConstraintMap::const_iterator it = must_.lower_bound(i);
           it != must_.upper_bound(i); ++it) {
        if (assign[it->second] != init_index) {
          if (min_index == init_index) {
            min_index = assign[it->second];
          } else if (min_index != assign[it->second]) {
            fprintf(stderr, "constraint inconsistency: multiple must target\n");
            exit(1);
          }
        }
      }
      // check cannot constraint
      cannot_cluster.clear();
      for (ConstraintMap::const_iterator it = cannot_.lower_bound(i);
           it != cannot_.upper_bound(i); ++it) {
        if (assign[it->second] != init_index) {
          cannot_cluster[assign[it->second]] = true;
        }
      }
      if (min_index != init_index) {
        if (cannot_cluster.find(min_index) != cannot_cluster.end()) {
          fprintf(stderr, "constraint inconsistency: must target contained in 'cannot' cluster\n");
          exit(1);
        } else {
          assign[i] = min_index;
          continue;
        }
      }

      for (size_t j = 0; j < centers_.size(); j++) {
        if (cannot_cluster.find(j) != cannot_cluster.end()) continue;
        double dist = euclid_distance_squared(*vectors_[i], *centers_[j]);
        if (dist < min_dist) {
          min_index = j;
          min_dist = dist;
        }
      }
      if (min_index == vectors_.size()) {
        fprintf(stderr, "cannot find closest cluster. exit now\n");
        exit(1);
      } else {
        assign[i] = min_index;
      }
    }
  }

  void move_centers(const size_t *assign) {
    for (size_t i = 0; i < centers_.size(); i++) {
      centers_[i]->clear();
    }
    std::vector<size_t> count(centers_.size());
    Vector::iterator cit;
    for (size_t i = 0; i < vectors_.size(); i++) {
      for (Vector::iterator it = vectors_[i]->begin();
           it != vectors_[i]->end(); ++it) {
        cit = centers_[assign[i]]->find(it->first);
        if (cit != centers_[assign[i]]->end()) {
          cit->second += it->second;
        } else {
          centers_[assign[i]]->insert(
            std::pair<VecKey, double>(it->first, it->second));
        }
      }
      count[assign[i]]++;
    }
    for (size_t i = 0; i < count.size(); i++) {
      if (count[i] == 0) continue;
      for (Vector::iterator it = centers_[i]->begin();
           it != centers_[i]->end(); ++it) {
        it->second /= count[i];
      }
    }
  }

  bool is_same_array(size_t *array1, size_t *array2, size_t size) {
    for (size_t i = 0; i < size; i++) {
      if (array1[i] != array2[i]) return false;
    }
    return true;
  }

 public:
  KMeans() { labels_.set_empty_key(""); }

  ~KMeans() {
    for (size_t i = 0; i < vectors_.size(); i++) {
      if (vectors_[i]) delete vectors_[i];
    }
    for (size_t i = 0; i < centers_.size(); i++) {
      if (centers_[i]) delete centers_[i];
    }
  }

  void add_vector(const std::string &label, Vector *vec) {
    assert(!label.empty() && !vec->empty());
    labels_[label] = vectors_.size();
    vectors_.push_back(vec);
  }

  void add_constraint(const std::string &label1, const std::string &label2,
                      Constraint type) {
    size_t index1, index2;
    LabelMap::iterator it;
    it = labels_.find(label1);
    if (it != labels_.end()) {
      index1 = it->second;
    } else {
      fprintf(stderr, "label not found in add_constraint: %s\n", label1.c_str());
      return;
    }
    it = labels_.find(label2);
    if (it != labels_.end()) {
      index2 = it->second;
    } else {
      fprintf(stderr, "label not found in add_constraint: %s\n", label2.c_str());
      return;
    }

    switch(type) {
    case CONSTRAINT_MUST:
      must_.insert(std::pair<size_t, size_t>(index1, index2));
      must_.insert(std::pair<size_t, size_t>(index2, index1));
      break;
    case CONSTRAINT_CANNOT:
      cannot_.insert(std::pair<size_t, size_t>(index1, index2));
      cannot_.insert(std::pair<size_t, size_t>(index2, index1));
      break;
    default:
      break;
    }
  }

  void execute(size_t nclusters) {
    assert(nclusters <= vectors_.size());
//    choose_random_centers(nclusters);
    choose_smart_centers(nclusters);
    size_t assign[vectors_.size()];
    size_t prev_assign[vectors_.size()];
    memset(assign, nclusters, sizeof(nclusters) * vectors_.size());
    memset(prev_assign, nclusters, sizeof(nclusters) * vectors_.size());
    for (size_t i = 0; i < MAX_ITER; i++) {
      fprintf(stderr, "kmeans loop No.%d ...\n", i);
      assign_clusters(assign);
      move_centers(assign);
      if (is_same_array(assign, prev_assign, vectors_.size())) {
        break;
      } else {
        std::copy(assign, assign + vectors_.size(), prev_assign);
      }
    }
    // show clustering result
    for (LabelMap::iterator it = labels_.begin(); it != labels_.end(); ++it) {
      printf("%s\t%d\n", it->first.c_str(), assign[it->second]);
    }
  }

  void show_vectors() const {
    for (LabelMap::const_iterator lit = labels_.begin();
         lit != labels_.end(); ++lit) {
      printf("%s", lit->first.c_str());
      for (Vector::const_iterator vit = vectors_[lit->second]->begin();
           vit != vectors_[lit->second]->end(); ++vit) {
        printf("\t%d\t%.3f", vit->first, vit->second);
      }
      printf("\n");
    }
  }
};

int main(int argc, char **argv) {
  if (argc < 3) usage(argv[0]);
  srand((unsigned int) time(NULL));
  KMeans kmeans;
  read_vectors(argv[2], kmeans);
//  kmeans.show_vectors();
  if (argc == 4) read_constraints(argv[3], kmeans);
  kmeans.execute(atoi(argv[1]));
  return 0;
}

void usage(const char *progname) {
  fprintf(stderr, "%s: ncluster data [constraint]\n", progname);
  exit(1);
}

void read_vectors(const char *filename, KMeans &kmeans) {
  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", filename);
    exit(1);
  }
  KeyMap keymap;
  keymap.set_empty_key("");
  VecKey curkey = EMPTY_KEY + 1;
  std::string line;
  std::vector<std::string> splited;
  while (getline(ifs, line)) {
    splitstring(line, DELIMITER, splited);
    if (splited.size() % 2 != 1) {
      fprintf(stderr, "format error: %s\n", line.c_str());
      continue;
    }
    Vector *vec = new Vector;
    vec->set_empty_key(EMPTY_KEY);
    for (size_t i = 1; i < splited.size(); i += 2) {
      KeyMap::iterator kit = keymap.find(splited[i]);
      VecKey key;
      if (kit != keymap.end()) {
        key = kit->second;
      } else {
        key = curkey;
        keymap[splited[i]] = curkey++;
      }
      double point = 0.0;
      point = atof(splited[i+1].c_str());
      if (point != 0) {
        vec->insert(std::pair<VecKey, double>(key, point));
      }
    }
    if (!splited[0].empty() && !vec->empty()) {
      kmeans.add_vector(splited[0], vec);
    }
    splited.clear();
  }
}

void read_constraints(const char *filename, KMeans &kmeans) {
  std::ifstream ifs(filename);
  if (!ifs) {
    fprintf(stderr, "cannot open %s\n", filename);
    exit(1);
  }
  std::vector<std::string> splited;
  std::string line;
  while (getline(ifs, line)) {
    splitstring(line, DELIMITER, splited);
    if (splited.size() == 3) {
      if (splited[2] == "c") {
        kmeans.add_constraint(splited[0], splited[1], KMeans::CONSTRAINT_CANNOT);
      } else if (splited[2] == "m") {
        kmeans.add_constraint(splited[0], splited[1], KMeans::CONSTRAINT_MUST);
      }
    }
    splited.clear();
  }
}

size_t splitstring(std::string s, const std::string &delimiter,
                   std::vector<std::string> &splited) {
  size_t cnt = 0;
  for (size_t p = 0; (p = s.find(delimiter)) != s.npos; ) {
    splited.push_back(s.substr(0, p));
    ++cnt;
    s = s.substr(p + delimiter.size());
  }
  splited.push_back(s);
  ++cnt;
  return cnt;
}
