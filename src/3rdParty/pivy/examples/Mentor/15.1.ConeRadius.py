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
# This is an example from the Inventor Mentor.
# chapter 15, example 1.
#
# Uses an SoTranslate1Dragger to control the bottomRadius field 
# of an SoCone.  The 'translation' field of the dragger is the 
# input to an SoDecomposeVec3f engine. The engine extracts the
# x component from the translation. This extracted value is
# connected to the bottomRadius field of the cone.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Create myDragger with an initial translation of (1,0,0)
    myDragger = SoTranslate1Dragger()
    root.addChild(myDragger)
    myDragger.translation = (1,0,0)

    # Place an SoCone above myDragger
    myTransform = SoTransform()
    myCone = SoCone()
    root.addChild(myTransform)
    root.addChild(myCone)
    myTransform.translation = (0,3,0)

    # SoDecomposeVec3f engine extracts myDragger's x-component
    # The result is connected to myCone's bottomRadius.
    myEngine = SoDecomposeVec3f()
    myEngine.vector.connectFrom(myDragger.translation)
    myCone.bottomRadius.connectFrom(myEngine.x)

    # Display them in a viewer
    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Dragger Edits Cone Radius")
    myViewer.viewAll()
    myViewer.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
