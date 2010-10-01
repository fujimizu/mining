#!/usr/bin/env python
# -*- coding: utf8 -*-

import os
import sys
from PIL import Image

def show_images(filenames):
    length = 100
    nrow = 3
    ncol = 5
    cnt = 0
    canvas = Image.new('RGB', (length * ncol, length * nrow), (255, 255, 255))
    for filename in filenames:
        if not os.path.exists(filename): continue
        if cnt >= nrow * ncol: break
        img = Image.open(filename)
        img_resized = img.resize((length, length))
        pos = (length * (cnt % ncol), length * (cnt / ncol))
        canvas.paste(img_resized, pos)
        cnt += 1
    canvas.show('clustering result', 'display')

def main():
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: %s filename id\n" % (sys.argv[0]))
        sys.exit(1)
    filename = sys.argv[1]
    target_id = sys.argv[2]
    fp = open(filename, 'r')
    for line in fp:
        line = line.strip()
        arr = line.split('\t')
        cluster_id = arr.pop(0)
        if target_id == cluster_id:
            show_images(arr)

if __name__ == '__main__':
    main();
