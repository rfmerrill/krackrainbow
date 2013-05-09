#!/usr/bin/env python3

# DISCLAIMER: this exists for educational purposes only, to illustrate what krackoutput.c does to people
# who might have trouble understanding it.
# This version runs WAY slower. Don't use it. Use the C version.

import sys
import os
import functools
import string
import binascii
import fcntl


if len(sys.argv) < 2:
  print ( "Usage: ", sys.argv[0], "[i2c device]" )
  sys.exit(1)


devfd = os.open(sys.argv[1], os.O_WRONLY) 

while True:
  theline = sys.stdin.readline()

  if not theline:
    break
   
  theline = ''.join(c for c in theline if c in string.hexdigits) 

  if (len(theline) % 2) == 1:
    theline = theline + '0'

  (address, packet) = (theline[0:2], theline[2:]) 

  address = int(address, 16)
  packet = binascii.a2b_hex(packet.encode('ascii')) 

  fcntl.ioctl(devfd, 0x0703, address)
  os.write(devfd, packet)

