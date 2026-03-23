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
# chapter 14, example 1.
#
# Use SoShapeKits to create two 3-D words, "NICE" and "HAPPY"
# Use nodekit methods to access the fields of the "material"
# and "transform" parts.
# Use a calculator engine and an elapsed time engine to make
# the words change color and fly about the screen.
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Create shape kits with the words "HAPPY" and "NICE"
    happyKit = SoShapeKit()
    root.addChild(happyKit)
    happyKit.setPart("shape", SoText3())
    happyKit.set("shape { parts ALL string \"HAPPY\"}")
    happyKit.set("font { size 2}")

    niceKit = SoShapeKit()
    root.addChild(niceKit)
    niceKit.setPart("shape", SoText3())
    niceKit.set("shape { parts ALL string \"NICE\"}")
    niceKit.set("font { size 2}")

    # Create the Elapsed Time engine
    myTimer = SoElapsedTime()

    # Create two calculator - one for HAPPY, one for NICE.
    happyCalc = SoCalculator()
    happyCalc.a.connectFrom(myTimer.timeOut)
    happyCalc.expression = """ta=cos(2*a); tb=sin(2*a);
                              oA = vec3f(3*pow(ta,3),3*pow(tb,3),1);
                              oB = vec3f(fabs(ta)+.1,fabs(.5*fabs(tb))+.1,1);
                              oC = vec3f(fabs(ta),fabs(tb),.5)"""

    # The second calculator uses different arguments to
    # sin() and cos(), so it moves out of phase.
    niceCalc = SoCalculator()
    niceCalc.a.connectFrom(myTimer.timeOut)
    niceCalc.expression = """ta=cos(2*a+2); tb=sin(2*a+2);
                             oA = vec3f(3*pow(ta,3),3*pow(tb,3),1);
                             oB = vec3f(fabs(ta)+.1,fabs(.5*fabs(tb))+.1,1);
                             oC = vec3f(fabs(ta),fabs(tb),.5)"""

    # Connect the transforms from the calculators...
    happyXf = happyKit.getPart("transform",TRUE)
    happyXf.translation.connectFrom(happyCalc.oA)
    happyXf.scaleFactor.connectFrom(happyCalc.oB)
    niceXf = niceKit.getPart("transform",TRUE)
    niceXf.translation.connectFrom(niceCalc.oA)
    niceXf.scaleFactor.connectFrom(niceCalc.oB)

    # Connect the materials from the calculators...
    happyMtl = happyKit.getPart("material",TRUE)
    happyMtl.diffuseColor.connectFrom(happyCalc.oC)
    niceMtl = niceKit.getPart("material",TRUE)
    niceMtl.diffuseColor.connectFrom(niceCalc.oC)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Frolicking Words")
    myViewer.viewAll()
    myViewer.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
