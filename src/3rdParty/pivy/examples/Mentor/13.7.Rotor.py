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
# chapter 13, example 8.
#
# Rotor node example.  
# Read in the tower and vanes of a windmill from a file.
# Use a rotor node to rotate the vanes.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def readFile(filename):
    # Open the input file
    mySceneInput = SoInput()
    if not mySceneInput.openFile(filename):
        print("Cannot open file %s" % (filename), file=sys.stderr)
        return None

    # Read the whole file into the database
    myGraph = SoDB.readAll(mySceneInput)
    if myGraph == None:
        print("Problem reading file", file=sys.stderr)
        return None

    mySceneInput.closeFile()
    return myGraph

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Read in the data for the windmill tower
    windmillTower = readFile("windmillTower.iv")
    root.addChild(windmillTower)

    # Add a rotor node to spin the vanes
    myRotor = SoRotor()
    myRotor.rotation.setValue(SbVec3f(0, 0, 1), 0) # z axis
    myRotor.speed = 0.2
    root.addChild(myRotor)

    # Read in the data for the windmill vanes
    windmillVanes = readFile("windmillVanes.iv")
    root.addChild(windmillVanes)

    # Create a viewer
    myViewer = SoGuiExaminerViewer(myWindow)

    # attach and show viewer
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Windmill")
    myViewer.show()

    SoDB.setRealTimeInterval(1/120.0)
    
    # Loop forever
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
