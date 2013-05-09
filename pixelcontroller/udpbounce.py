#!/usr/bin/python3

import sys
import socket

if len(sys.argv) < 4:
  print ("usage: udpbounce [bindhost] [localport] [remotehost] [remoteport]")
  quit()

recvhost = sys.argv[1]
recvport = int(sys.argv[2])
sendhost = sys.argv[3]
sendport = int(sys.argv[4])

insock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
insock.bind((recvhost, recvport))
outsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
  data, addr = insock.recvfrom(4096)

  outsock.sendto(data, (sendhost, sendport))  
  



