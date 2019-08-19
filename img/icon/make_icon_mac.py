#!/usr/bin/env python
#
# Inkscape should be in the PATH, should be already there on Ubuntu
#
# for Windows MSYS2 do:
#
# export PATH=/c/'Program Files'/Inkscape:$PATH
#
# for Windows cmd do:
#
# set PATH=c:\"Program Files"\Inkscape;%PATH%
#

from __future__ import print_function

import os
import subprocess

def export_image(size, factor):
  out_filename = 'icon_{size}x{size}{suffix}.png'.format(size = size, suffix = '@2' if factor == 2 else '')
  cmd = ['inkscape',
         '--export-png=procyon.iconset/{}'.format(out_filename),
         '--export-width={}'.format(size * factor),
         '--export-height={}'.format(size * factor),
         'main_src.svg']
           
  p = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, universal_newlines = True)
  for line in iter(p.stdout.readline, ''): print(line, end = '')
  p.stdout.close()

os.chdir(os.path.dirname(os.path.realpath(__file__)))

for size in [16, 23, 128, 256, 512]:
  export_image(size, 1)
  export_image(size, 2)
