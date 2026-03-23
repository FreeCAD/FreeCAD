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
# chapter 12, example 1.
#
# Sense changes to a viewer's camera's position.
#

import sys

from pivy.coin import *
from pivy.sogui import *

# Callback that reports whenever the viewer's position changes.
def cameraChangedCB(viewerCamera, sensor):
    cameraPosition = viewerCamera.position.getValue()
    print("Camera position: (%g,%g,%g)" % (cameraPosition[0],
                                           cameraPosition[1],
                                           cameraPosition[2]))
    
def main():
    if len(sys.argv) != 2:
        print("Usage: %s filename.iv" % (sys.argv[0]),  file=sys.stderr) 
        sys.exit(1)

    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    inputFile = SoInput()
    if inputFile.openFile(sys.argv[1]) == 0:
        print("Could not open file %s" % (sys.argv[1]), file=sys.stderr)
        sys.exit(1)
   
    root = SoDB.readAll(inputFile)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Camera Sensor")
    myViewer.show()

    # Get the camera from the viewer, and attach a 
    # field sensor to its position field:
    camera = myViewer.getCamera()
    mySensor = SoFieldSensor(cameraChangedCB, camera)
    mySensor.attach(camera.position)
    
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
