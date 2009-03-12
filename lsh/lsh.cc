/**
 * Locality Sensitive Hash
 *
 * Usage:
 *   % lsh input_dbm output_txt
 *
 * Build:
 *   % g++ `tcucodec conf -l` -Wall -O3 lsh.c -o lsh
 *
 * Requirement:
 *   - Tokyo Cabinet
 *
 */
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <tchdb.h>
#include <ext/hash_map>

using namespace std;

/* Constants */
const int    NUM_RANDOM_KEY = 100;
const int    NUM_SHUFFLE    = 15;
const int    NUM_NEIGHBOR   = 10;
const int    MAX_VECTOR_KEY = 50;
const double MIN_COSINE     = 0.7;

/* type */
typedef map<string, double> Vector;
typedef map<string, double>::iterator VecIt;
typedef map<string, vector<int> > Bits;
typedef map<string, vector<int> >::iterator BitsIt;

/*
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

typedef __gnu_cxx::hash_map<string, double, stringhash> Vector;
typedef __gnu_cxx::hash_map<string, double, stringhash>::iterator VecIt;
typedef __gnu_cxx::hash_map<string, vector<int>, stringhash> Bits;
typedef __gnu_cxx::hash_map<string, vector<int>, stringhash>::iterator BitsIt;
*/

/* function prototypes */
int main(int, char **);
void read_vector(TCHDB *, const char *, int, Vector &);
void select_keys_randomly(TCHDB *, int, vector<string> &);
void lsh(TCHDB *, const char *);
double cosine(Vector &, Vector &);
double inner_product(Vector &, Vector &);
double norm(Vector &);
void print_vector(Vector &);


int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage %s input_dbm output_txt\n", argv[0]);
    exit(1);
  }
  
  TCHDB *vecdb = tchdbnew();
  if (!tchdbopen(vecdb, argv[1], HDBOREADER)) {
    int ecode = tchdbecode(vecdb);
    fprintf(stderr, "[error] open error:%s\n", tchdberrmsg(ecode));
    exit(1);
  }

  lsh(vecdb, argv[2]);

  tchdbdel(vecdb);
  return 0;
}

void read_vector(TCHDB *vecdb, const char *key, int knum, Vector &v) {
  if (key == NULL) return;

  char *value = tchdbget2(vecdb, key);
  if (value != NULL) {
    string vecstr(value);
    string key(""), delimiter("\t");
    int cnt = 0;
    int kwcnt = 0;

    int p = vecstr.find(delimiter);
    while (p != (int)vecstr.npos && kwcnt < knum) {
      string s = vecstr.substr(0, p);
      if (cnt % 2 == 0) {
        key = s;
      } else {
        double point = 0;
        point = atof(s.c_str());
        if (!key.empty() && point != 0) {
          v[key] = point;
          kwcnt++;
        }
      }
      cnt++;
      vecstr = vecstr.substr(p + delimiter.size());
      p = vecstr.find(delimiter);
    }
    free(value);
  }
}


void select_keys_randomly(TCHDB *vecdb, int limit, vector<string> &keys) {
  int rnum = tchdbrnum(vecdb);
  if (limit > rnum) limit = rnum;

  map<int, bool> idxmap;
  int cnt = 0;
  srand((unsigned) time(NULL));
  while (cnt < limit) {
    int idx = rand() % rnum;
    if (idxmap.find(idx) == idxmap.end()) {
      idxmap[idx] = true;
      ++cnt;
    }
  }

  int idx = 0;
  char *key;
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    if (idxmap[idx]) {
      keys.push_back(key);
    }
    free(key);
    idx++;
  }
}

void lsh(TCHDB *vecdb, const char *filename) {
  ofstream ofs(filename);

  cout << "Select vectors randomly" << endl;
  vector<string> keys;
  select_keys_randomly(vecdb, NUM_RANDOM_KEY, keys);
  vector<Vector> randvecs;
  for (unsigned int i = 0; i < keys.size(); i++) {
    Vector v;
    read_vector(vecdb, keys[i].c_str(), MAX_VECTOR_KEY, v);
    randvecs.push_back(v);
  }

  cout << "Calculate bits" << endl;
  Bits bits;
  char *key;
  tchdbiterinit(vecdb);
  while ((key = tchdbiternext2(vecdb)) != NULL) {
    Vector v;
    read_vector(vecdb, key, MAX_VECTOR_KEY, v);
    if (!v.empty()) {
      vector<int> bit;
      for (unsigned int i = 0; i < randvecs.size(); i++) {
        double cos = cosine(v, randvecs[i]);
        if (cos > 0) {
          bit.push_back(1); 
        } else {
          bit.push_back(0); 
        }
      }
      bits[key] = bit;
    }
    free(key);
  }

  map<string, bool> check;
  for (int i = 0; i < NUM_SHUFFLE; i++) {
    cout << "Loop No." << i << endl;

    cout << " Shuffle bits" << endl;
    vector<int> indexes;
    for (int j = 0; j < NUM_RANDOM_KEY; j++) {
      indexes.push_back(j);
    }
    random_shuffle(indexes.begin(), indexes.end());

    vector<pair<string, string> > bits_shuffled;
    BitsIt it = bits.begin();
    while (it != bits.end()) {
      string key = (*it).first;
      stringstream ss;
      for (unsigned int j = 0; j < indexes.size(); j++) {
        ss << bits[key][j];
      }
      pair<string, string> p;
      p.first = ss.str();
      p.second = key;
      bits_shuffled.push_back(p);
      ++it;
    }
    sort(bits_shuffled.begin(), bits_shuffled.end());

    cout << " Get similar keys" << endl;
    int size = bits_shuffled.size();
    for (int j = 0; j < size; j++) {
      string key = bits_shuffled[j].second;
      Vector v;
      read_vector(vecdb, key.c_str(), MAX_VECTOR_KEY, v);
      int start = ((j - NUM_NEIGHBOR) > 0) ? (j - NUM_NEIGHBOR) : 0;
      int end = (j + NUM_NEIGHBOR < size) ? (j + NUM_NEIGHBOR) : (size-1);
      for (int k = start; k <= end; k++) {
        if (j == k) continue;
        string key_neigh = bits_shuffled[k].second;
        stringstream ss;
        if (j < k) {
          ss << key << "\t" << key_neigh;
        } else {
          ss << key_neigh << "\t" << key;
        }
        string chkkey = ss.str();
        if (check.find(chkkey) == check.end()) {
          check[chkkey] = true;
          Vector v_neigh;
          read_vector(vecdb, key_neigh.c_str(), MAX_VECTOR_KEY, v_neigh);
          double cos = cosine(v, v_neigh);
          if (cos > MIN_COSINE) ofs << chkkey << "\t" << cos << endl;
        }
      }
    }
  }
}

double cosine(Vector &v1, Vector &v2) {
  double len1 = norm(v1);
  double len2 = norm(v2);
  if (len1 == 0 || len2 == 0) return 0;
  
  double prod = inner_product(v1, v2);
  return prod / (len1 * len2);
}

double inner_product(Vector &v1, Vector &v2) {
  double result = 0;
  VecIt it = v1.begin();
  while (it != v1.end()) {
    string key = (*it).first;
    if (v2.find(key) != v2.end()) {
      double p1 = v1[key];
      double p2 = v2[key];
      result += p1 * p2;
    }
    ++it;
  }
  return result;
}

double norm(Vector &v) {
  if (v.size() == 0) return 0;

  double result = 0;
  VecIt it = v.begin();
  while (it != v.end()) {
    double point = (*it).second;
    result += point * point;
    ++it;
  }
  return sqrt(result);
}

void print_vector(Vector &v) {
  VecIt it = v.begin();
  cout << "( ";
  while (it != v.end()) {
    cout << (*it).first << ":" << (*it).second << " ";
    ++it;
  }
  cout << " )" << endl;
}
