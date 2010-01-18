#!/usr/bin/env python
# -*- coding: utf8 -*-
#
# Show color histogram
#

import sys
from PIL import Image

def get_histogram(filename):
    img = Image.open(filename)
    return img.histogram()

def main():
    if len(sys.argv) != 2:
        sys.stderr.write("Usage: %s txtfile\n" % (sys.argv[0]))
        sys.exit(1)
    filename = sys.argv[1]
    fp = open(filename, 'r')
    for line in fp:
        line = line.strip()
        hist = get_histogram(line)
        if len(hist) == 768:
            sys.stdout.write('%s' % line)
            for i in range(0, len(hist)):
                sys.stdout.write('\t%d\t%d' % (i, hist[i]))
            print

if __name__ == '__main__':
    main();
