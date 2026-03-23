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
# chapter 9, example 1.
#
# Printing example.
# Read in an Inventor file and display it in ExaminerViewer.  Press
# the "p" key and the scene renders into a PostScript
# file for printing.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

class callbackData:
    vwr = None
    filename = None
    scene = None

##############################################################
## CODE FOR The Inventor Mentor STARTS HERE

def printToPostScript(root, file, viewer, printerDPI):
    # Calculate size of the images in inches which is equal to
    # the size of the viewport in pixels divided by the number
    # of pixels per inch of the screen device.  This size in
    # inches will be the size of the Postscript image that will
    # be generated.
    vp  = viewer.getViewportRegion()
    imagePixSize = vp.getViewportSizePixels()
    imageInches = SbVec2f()

    pixPerInch = SoOffscreenRenderer.getScreenPixelsPerInch()
    imageInches.setValue(imagePixSize[0] / pixPerInch,
                         imagePixSize[1] / pixPerInch)

    # The resolution to render the scene for the printer
    # is equal to the size of the image in inches times
    # the printer DPI
    postScriptRes = SbVec2s()
    postScriptRes.setValue(int(imageInches[0]*printerDPI),
                           int(imageInches[1]*printerDPI))

    # Create a viewport to render the scene into.
    myViewport = SbViewportRegion()
    myViewport.setWindowSize(postScriptRes)
    myViewport.setPixelsPerInch(printerDPI)
    
    # Render the scene
    myRenderer = SoOffscreenRenderer(myViewport)

    if not myRenderer.render(root):
        return FALSE

    # Generate PostScript and write it to the given file
    myRenderer.writeToPostScript(file)

    return TRUE

# CODE FOR The Inventor Mentor ENDS HERE
##############################################################

def processKeyEvents(data, cb):
    if SoKeyboardEvent_isKeyPressEvent(cb.getEvent(), SoKeyboardEvent.P):
        myFile = open(data.filename, "w")

        if myFile == None:
            sys.stderr.write("Cannot open output file\n")
            sys.exit(1)

        sys.stdout.write("Printing scene... ")
        sys.stdout.flush()
        if not printToPostScript(data.scene, myFile, data.vwr, 75):
            sys.stderr.write("Cannot print image\n")
            myFile.close()
            sys.exit(1)

        myFile.close()
        sys.stdout.write("  ...done printing.\n")
        sys.stdout.flush()
        cb.setHandled()

def main():
    # Initialize Inventor and Qt
    appWindow = SoGui.init(sys.argv[0])
    if appWindow == None:
        sys.exit(1)
        
    # Verify the command line arguments
    if len(sys.argv) != 3:
        sys.stdout.write("Usage: %s infile.iv outfile.ps\n" % sys.argv[0])
        sys.exit(1) 
  
    print("To print the scene: press the 'p' key while in picking mode")

    # Make a scene containing an event callback node
    root = SoSeparator()
    eventCB = SoEventCallback()
    root.addChild(eventCB)

    # Read the geometry from a file and add to the scene
    myInput = SoInput()
    if not myInput.openFile(sys.argv[1]):
        sys.exit(1)
    geomObject = SoDB.readAll(myInput)
    if geomObject == None:
        sys.exit(1)
    root.addChild(geomObject)

    viewer = SoGuiExaminerViewer(appWindow, "None", TRUE, SoGuiExaminerViewer.BUILD_ALL, SoGuiExaminerViewer.EDITOR)
    viewer.setSceneGraph(root)
    viewer.setTitle("Print to PostScript")
    
    # Setup the event callback data and routine for performing the print
    data = callbackData()
    data.vwr = viewer
    data.filename = sys.argv[2]
    data.scene = viewer.getSceneGraph()
    eventCB.addEventCallback(SoKeyboardEvent.getClassTypeId(), processKeyEvents, data)
    viewer.show()

    SoGui.show(appWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
