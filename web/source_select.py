#!/usr/bin/env python3

import subprocess
import sys

if len(sys.argv) < 2:
  print ("Usage: output_select [prefix]")
  sys.exit(1)

path_prefix = sys.argv[1]

my_child = None

for line in sys.stdin:
  if my_child is not None:
    my_child.kill()
  line = line.strip()

  if line: 
    the_path = path_prefix + line

    my_child = subprocess.Popen([the_path]) 


