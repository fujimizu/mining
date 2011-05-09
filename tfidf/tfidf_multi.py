#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Calculate TFIDF values from input texts
#
# Usage:
#  $ ./tfidf_multi.py outdir textfile1 textfile2 ...
#
# Requirement:
#  - MeCab
#  - MeCab python binding
#  - Kyoto Cabinet
#  - Kyoto Cabinet python binding
#

from kyotocabinet import DB
import MeCab
import math
import os
import re
import struct
import sys

def get_words(tagger, words, text):
    morph = tagger.parseToNode(text)
    while morph:
        features = morph.feature.split(',')
        if features[0] == '名詞':
            if morph.surface in words:
                words[morph.surface] += 1
            else:
                words[morph.surface] = 1
        morph = morph.next

def count_words(tfdb, dfdb, filename):
    fp = open(filename, 'r')
    tagger = MeCab.Tagger('')
    words = {}
    title = ''
    body = ''
    for line in fp:
        line.strip()
        if not line: continue
        matched = re.match('^\[\[(.+)\]\]$', line)
        if matched:
            if title and body:
                for sentence in body.split('。'):
                    get_words(tagger, words, sentence)
                if words:
                    save_tf(tfdb, title, words)
                    save_df(dfdb, words)
            title = matched.groups()[0]
            body = ''
            words = {}
        else:
            body += title
    fp.close()

def save_tf(tfdb, title, words):
    val = '\t'.join(map(
        lambda x: '%s\t%d' % x, sorted(words.items(), key=lambda x: -x[1])))
    tfdb.set(title, val)

def save_df(dfdb, words):
    for word in words:
        dfdb.increment(word, 1)

def save_tfidf(tfdb, dfdb, tfidfdb, maxcol=50):
    total = tfdb.count()
    cur = tfdb.cursor()
    cur.jump()
    while True:
        rec = cur.get(True)
        if not rec: break
        title = rec[0]
        splited = rec[1].split('\t')
        tfidf = []
        for i in range(0, len(splited), 2):
            word = splited[i]
            tf = int(splited[i+1])
            df = struct.unpack('>q', dfdb.get(word))[0]
            idf = math.log(float(total) / (df + 1))
            tfidf.append((word, tf * idf))
        tfidf_sorted = sorted(tfidf, key=lambda x: -x[1])
        if len(tfidf_sorted) > maxcol:
            tfidf_sorted = tfidf_sorted[0:maxcol]
        val = '\t'.join(map(lambda x: '%s\t%d' % (x[0], x[1] * 100),
                            tfidf_sorted))
        tfidfdb.set(title, val)
    cur.disable()

def main():
    if len(sys.argv) < 3:
        sys.stderr.write('Usage: %s outdir textfile1 textfile2 ...\n'
                         % sys.argv[0])
        sys.exit(1)
    outdir = sys.argv[1]
    tfdb = DB()
    if not tfdb.open(os.path.join(outdir, 'tf.kch'),
                     DB.OWRITER | DB.OCREATE | DB.OTRUNCATE):
        sys.stderr.write('cannot open tfdb: %s\n' % str(tfdb.error))
        sys.exit(1)
    dfdb = DB()
    if not dfdb.open(os.path.join(outdir, 'df.kch'),
                     DB.OWRITER | DB.OCREATE | DB.OTRUNCATE):
        sys.stderr.write('cannot open dfdb: %s\n' % str(dfdb.error))
        sys.exit(1)
    tfidfdb = DB()
    if not tfidfdb.open(os.path.join(outdir, 'tfidf.kch'),
                     DB.OWRITER | DB.OCREATE | DB.OTRUNCATE):
        sys.stderr.write('cannot open tfidfdb: %s\n' % str(tfidfdb.error))
        sys.exit(1)

    print 'Count words ...'
    for i in range(len(sys.argv)-2):
        filename = sys.argv[i+2]
        print '(%d/%d) %s' % (i+1, len(sys.argv)-2, filename)
        count_words(tfdb, dfdb, filename)
    print 'Calculate TFIDF ...'
    save_tfidf(tfdb, dfdb, tfidfdb)

    tfdb.close()
    dfdb.close()
    tfidfdb.close()

if __name__ == '__main__':
    main()
