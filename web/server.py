#!/usr/bin/env python

import sys
import subprocess
import RPi.GPIO as GPIO

from twisted.web import static, server, resource
from twisted.internet import reactor 

class PowerOn(resource.Resource):
  isLeaf = True
  def getChild(self, name, request):
    if name == '':
      return self
    return Resource.getChild(self, name, request)

  def render_GET(self, request):
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(17, GPIO.OUT)
    GPIO.output(17, GPIO.HIGH)
    return "Power On!"

class PowerOff(resource.Resource):
  isLeaf = True
  def getChild(self, name, request):
    if name == '':
      return self
    return Resource.getChild(self, name, request)

  def render_GET(self, request):
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(17, GPIO.OUT)
    GPIO.output(17, GPIO.LOW)
    return "Power Off!"

class I2CDetect(resource.Resource):
  isLeaf = True
  def getChild(self, anme, request):
    if name == '':
      return self
    return Resource.getChild(self, name, request)

  def render_GET(self, request):
    beginning = "<html><head><title>i2cdetect output</title></head><body><h1>Channel 1</h1><pre>"
    ch1out = subprocess.check_output("i2cdetect -a -y 1", shell=True)
    end = "</pre></body></html>"
    return beginning + ch1out + end

class I2CDetectRaw(resource.Resource):
  isLeaf = True
  def getChild(self, anme, request):
    if name == '':
      return self
    return Resource.getChild(self, name, request)

  def render_GET(self, request):
    return subprocess.check_output("i2cdetect -a -y 1", shell=True)



root = static.File(sys.argv[1])

root.putChild("off", PowerOff())
root.putChild("on", PowerOn())
root.putChild("i2c", I2CDetect())
root.putChild("i2craw", I2CDetectRaw())

reactor.listenTCP(80, server.Site(root))
reactor.run()

