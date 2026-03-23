#!/usr/bin/env python

###
# Copyright (c) 2002-2007 Systems in Motion
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

###
# This is an example from the Inventor Mentor
# chapter 13, example 7.
#
# A calculator engine computes a closed, planar curve.
# The output from the engine is connected to the translation
# applied to a flower object, which consequently moves
# along the path of the curve.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Add a camera and light
    myCamera = SoPerspectiveCamera()
    myCamera.position = (-0.5, -3.0, 19.0)
    myCamera.nearDistance = 10.0
    myCamera.farDistance = 26.0
    root.addChild(myCamera)
    root.addChild(SoDirectionalLight())

    # Rotate scene slightly to get better view
    globalRotXYZ = SoRotationXYZ()
    globalRotXYZ.axis = SoRotationXYZ.X
    globalRotXYZ.angle = M_PI/7
    root.addChild(globalRotXYZ)

    # Read the background path from a file and add to the group
    myInput = SoInput()
    if not myInput.openFile("flowerPath.iv"):
        sys.exit(1)
    flowerPath = SoDB.readAll(myInput)
    if flowerPath == None: sys.exit(1)
    root.addChild(flowerPath)

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE  

    # Flower group
    flowerGroup = SoSeparator()
    root.addChild(flowerGroup)

    # Read the flower object from a file and add to the group
    if not myInput.openFile("flower.iv"):
        sys.exit(1)
    flower = SoDB.readAll(myInput)
    if flower == None: sys.exit(1)

    # Set up the flower transformations
    danceTranslation = SoTranslation()
    initialTransform = SoTransform()
    flowerGroup.addChild(danceTranslation)
    initialTransform.scaleFactor = (10., 10., 10.)
    initialTransform.translation = (0., 0., 5.)
    flowerGroup.addChild(initialTransform)
    flowerGroup.addChild(flower)

    # Set up an engine to calculate the motion path:
    # r = 5*cos(5*theta) x = r*cos(theta) z = r*sin(theta)
    # Theta is incremented using a time counter engine,
    # and converted to radians using an expression in
    # the calculator engine.
    calcXZ = SoCalculator()
    thetaCounter = SoTimeCounter()

    thetaCounter.max = 360
    thetaCounter.step = 4
    thetaCounter.frequency = 0.075

    calcXZ.a.connectFrom(thetaCounter.output)    
    calcXZ.expression.set1Value(0, "ta=a*M_PI/180") # theta
    calcXZ.expression.set1Value(1, "tb=5*cos(5*ta)") # r
    calcXZ.expression.set1Value(2, "td=tb*cos(ta)") # x 
    calcXZ.expression.set1Value(3, "te=tb*sin(ta)") # z 
    calcXZ.expression.set1Value(4, "oA=vec3f(td,0,te)") 
    danceTranslation.translation.connectFrom(calcXZ.oA)

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

    myRenderArea = SoGuiRenderArea(myWindow)
    myRenderArea.setSceneGraph(root)
    myRenderArea.setTitle("Flower Dance")
    myRenderArea.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
