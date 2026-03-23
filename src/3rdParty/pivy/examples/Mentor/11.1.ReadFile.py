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
# chapter 11, example 1.
#
# Example of reading from a file.
# Read a file given a filename and return a separator
# containing all of the file.  Return NULL if there is 
# an error reading the file.
#

from __future__ import print_function
import sys

from pivy.coin import *
from pivy.sogui import *

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE

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

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])

    # Read the file
    scene = readFile("bookshelf.iv")

    # Create a viewer
    myViewer = SoGuiExaminerViewer(myWindow)

    # attach and show viewer
    myViewer.setSceneGraph(scene)
    myViewer.setTitle("File Reader")
    myViewer.show()
    
    # Loop forever
    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
