#!/usr/bin/env python3

# This actually runs decently now!

import sys
import os
import fcntl
import re


if len(sys.argv) < 2:
  print ( "Usage: ", sys.argv[0], "[i2c device]" )
  sys.exit(1)


devfd = os.open(sys.argv[1], os.O_WRONLY) 

nonhex = re.compile("[^0-9a-fA-F]")

for theline in sys.stdin:
  theline = nonhex.sub("", theline) 

  if ((len(theline) % 2) == 1):
    theline = theline[:-1]
 
  (address, packet) = (theline[0:2], theline[2:]) 

  address = int(address, 16)
  packet = bytes.fromhex(packet) 
  fcntl.ioctl(devfd, 0x0703, address)
  os.write(devfd, packet)
