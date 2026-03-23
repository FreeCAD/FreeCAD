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
# This is an example from the Inventor Mentor,
# chapter 3, example 2.
#
# This code shows how to create a robot out of various nodes.
# It introduces shared instancing of nodes to create two legs
# using two instances of the same subgraph.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def makeRobot():
##############################################################
# CODE FOR The Inventor Mentor STARTS HERE
    # Robot with legs

    # Construct parts for legs (thigh, calf and foot)
    thigh = SoCube()
    thigh.width = 1.2
    thigh.height = 2.2
    thigh.depth = 1.1
    
    calfTransform = SoTransform()
    calfTransform.translation.setValue(0, -2.25, 0.0)
    
    calf = SoCube()
    calf.width, calf.height, calf.depth = 1, 2.2, 1

    footTransform = SoTransform()
    footTransform.translation = (0, -1.5, .5)

    foot = SoCube()
    foot.width, foot.height, foot.depth = 0.8, 0.8, 2

    # Put leg parts together
    leg = SoGroup()
    leg.addChild(thigh)
    leg.addChild(calfTransform)
    leg.addChild(calf)
    leg.addChild(footTransform)
    leg.addChild(foot)
    
    leftTransform = SoTransform()
    leftTransform.translation = (1, -4.25, 0)
    
    # Left leg
    leftLeg = SoSeparator()
    leftLeg.addChild(leftTransform)
    leftLeg.addChild(leg)
    
    rightTransform = SoTransform()
    rightTransform.translation = (-1, -4.25, 0)
    
    # Right leg
    rightLeg = SoSeparator()
    rightLeg.addChild(rightTransform)
    rightLeg.addChild(leg)
    
    # Parts for body
    bodyTransform = SoTransform()
    bodyTransform.translation = (0.0, 3.0, 0.0)
    
    bronze = SoMaterial()
    bronze.ambientColor = (.33, .22, .27)
    bronze.diffuseColor = (.78, .57, .11)
    bronze.specularColor = (.99, .94, .81)
    bronze.shininess = .28
    
    bodyCylinder = SoCylinder()
    bodyCylinder.radius = 2.5
    bodyCylinder.height = 6
    
    # Construct body out of parts 
    body = SoSeparator()
    body.addChild(bodyTransform)      
    body.addChild(bronze)
    body.addChild(bodyCylinder)
    body.addChild(leftLeg)
    body.addChild(rightLeg)
    
    # Head parts
    headTransform = SoTransform()
    headTransform.translation = (0, 7.5, 0)
    headTransform.scaleFactor = (1.5, 1.5, 1.5)
    
    silver = SoMaterial()
    silver.ambientColor = (.2, .2, .2)
    silver.diffuseColor = (.6, .6, .6)
    silver.specularColor = (.5, .5, .5)
    silver.shininess = .5
    
    headSphere = SoSphere()
    
    # Construct head
    head = SoSeparator()
    head.addChild(headTransform)
    head.addChild(silver)
    head.addChild(headSphere)
    
    # Robot is just head and body
    robot = SoSeparator()
    robot.addChild(body)               
    robot.addChild(head)
    
# CODE FOR The Inventor Mentor ENDS HERE
##############################################################

    return robot


def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()
    
    # This function contains our code fragment.
    root.addChild(makeRobot())
    
    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Robot")
    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
