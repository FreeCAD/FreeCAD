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
# chapter 15, example 4.
#
# Same as chapter 15, example 2, with one difference:
# The draggers are customized to use different geometry.
# We create our own scene graphs for the parts "translator"
# and "translatorActive."
# Then we call setPart() to replace these two parts with our 
# new scene graphs. (Remember, draggers are derived from 
# nodekits, so it's easy to change the parts).
#

import sys

from pivy.coin import *
from pivy.sogui import *

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])  
    if myWindow == None: sys.exit(1)     

    root = SoSeparator()

    # Create 3 translate1Draggers and place them in space.
    xDragSep = SoSeparator()
    yDragSep = SoSeparator()
    zDragSep = SoSeparator()
    root.addChild(xDragSep)
    root.addChild(yDragSep)
    root.addChild(zDragSep)
    # Separators will each hold a different transform
    xDragXf = SoTransform()
    yDragXf = SoTransform()
    zDragXf = SoTransform()
    xDragXf.set("translation  0 -4 8")
    yDragXf.set("translation -8  0 8 rotation 0 0 1  1.57")
    zDragXf.set("translation -8 -4 0 rotation 0 1 0 -1.57")
    xDragSep.addChild(xDragXf)
    yDragSep.addChild(yDragXf)
    zDragSep.addChild(zDragXf)

    # Add the draggers under the separators, after transforms
    xDragger = SoTranslate1Dragger()
    yDragger = SoTranslate1Dragger()
    zDragger = SoTranslate1Dragger()
    xDragSep.addChild(xDragger)
    yDragSep.addChild(yDragger)
    zDragSep.addChild(zDragger)

#############################################################
# CODE FOR The Inventor Mentor STARTS HERE

    # Create myTranslator and myTranslatorActive.
    # These are custom geometry for the draggers.
    myTranslator = SoSeparator()
    myTranslatorActive = SoSeparator()
    # Materials for the dragger in regular and active states
    myMtl = SoMaterial()
    myActiveMtl = SoMaterial()
    myMtl.diffuseColor = (1,1,1)
    myActiveMtl.diffuseColor = (1,1,0)
    myTranslator.addChild(myMtl)
    myTranslatorActive.addChild(myActiveMtl)
    # Same shape for both versions.
    myCube = SoCube()
    myCube.set("width 3 height .4 depth .4")
    myTranslator.addChild(myCube)
    myTranslatorActive.addChild(myCube)

    # Now, customize the draggers with the pieces we created.
    xDragger.setPart("translator",myTranslator)
    xDragger.setPart("translatorActive",myTranslatorActive)
    yDragger.setPart("translator",myTranslator)
    yDragger.setPart("translatorActive",myTranslatorActive)
    zDragger.setPart("translator",myTranslator)
    zDragger.setPart("translatorActive",myTranslatorActive)

# CODE FOR The Inventor Mentor ENDS HERE
#############################################################

    # Create shape kit for the 3D text
    # The text says 'Slide Cubes To Move Me'
    textKit = SoShapeKit()
    root.addChild(textKit)
    myText3 = SoText3()
    textKit.setPart("shape", myText3)
    myText3.justification = SoText3.CENTER
    myText3.string.set1Value(0,"Slide Cubes")
    myText3.string.set1Value(1,"To")
    myText3.string.set1Value(2,"Move Me")
    textKit.set("font { size 2}")
    textKit.set("material { diffuseColor 1 1 0}")

    # Create shape kit for surrounding box.
    # It's an unpickable cube, sized as (16,8,16)
    boxKit = SoShapeKit()
    root.addChild(boxKit)
    boxKit.setPart("shape", SoCube())
    boxKit.set("drawStyle { style LINES }")
    boxKit.set("pickStyle { style UNPICKABLE }")
    boxKit.set("material { emissiveColor 1 0 1 }")
    boxKit.set("shape { width 16 height 8 depth 16 }")

    # Create the calculator to make a translation
    # for the text.  The x component of a translate1Dragger's 
    # translation field shows how far it moved in that 
    # direction. So our text's translation is:
    # (xDragTranslate[0],yDragTranslate[0],zDragTranslate[0])
    myCalc = SoCalculator()
    myCalc.A.connectFrom(xDragger.translation)
    myCalc.B.connectFrom(yDragger.translation)
    myCalc.C.connectFrom(zDragger.translation)
    myCalc.expression = "oA = vec3f(A[0],B[0],C[0])"

    # Connect the the translation in textKit from myCalc
    textXf = textKit.getPart("transform",TRUE)
    textXf.translation.connectFrom(myCalc.oA)

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Customized Sliders")
    myViewer.viewAll()
    myViewer.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
