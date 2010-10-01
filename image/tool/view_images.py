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
    canvas.show('images', 'display')

def main():
    filenames = []
    for line in sys.stdin:
        filename = line.strip()
        if os.path.exists(filename):
            filenames.append(filename)
    show_images(filenames)

if __name__ == '__main__':
    main();
