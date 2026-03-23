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
# chapter 3, example 1.
#
# This code shows how to create a molecule out of 3 spherical
# atoms.  The molecule illustrates how the ordering of nodes
# within a scene graph affects the rendered image.
#
#

import sys

from pivy.coin import *
from pivy.sogui import *

def makeWaterMolecule():
##############################################################
#  CODE FOR The Inventor Mentor STARTS HERE

    # Construct all parts
    waterMolecule = SoGroup()  # water molecule

    oxygen = SoGroup()         # oxygen atom
    redPlastic = SoMaterial()
    sphere1 = SoSphere()
    
    hydrogen1 = SoGroup()      # hydrogen atoms
    hydrogen2 = SoGroup()
    hydrogenXform1 = SoTransform()
    hydrogenXform2 = SoTransform()
    whitePlastic = SoMaterial()
    sphere2 = SoSphere()
    sphere3 = SoSphere()
    
    # Set all field values for the oxygen atom
    redPlastic.ambientColor = (1.0, 0.0, 0.0)
    redPlastic.diffuseColor = (1.0, 0.0, 0.0) 
    redPlastic.specularColor = (0.5, 0.5, 0.5)
    redPlastic.shininess = 0.5
    
    # Set all field values for the hydrogen atoms
    hydrogenXform1.scaleFactor = (0.75, 0.75, 0.75)  
    hydrogenXform1.translation = (0.0, -1.2, 0.0)  
    hydrogenXform2.translation = (1.1852, 1.3877, 0.0)
    whitePlastic.ambientColor = (1.0, 1.0, 1.0)  
    whitePlastic.diffuseColor = (1.0, 1.0, 1.0) 
    whitePlastic.specularColor = (0.5, 0.5, 0.5)
    whitePlastic.shininess = 0.5
    
    # Create a hierarchy
    waterMolecule.addChild(oxygen)   
    waterMolecule.addChild(hydrogen1)   
    waterMolecule.addChild(hydrogen2)
    
    oxygen.addChild(redPlastic)
    oxygen.addChild(sphere1)
    hydrogen1.addChild(hydrogenXform1)
    hydrogen1.addChild(whitePlastic)
    hydrogen1.addChild(sphere2)
    hydrogen2.addChild(hydrogenXform2)
    hydrogen2.addChild(sphere3)

## CODE FOR The Inventor Mentor ENDS HERE
##############################################################

    return waterMolecule

def main():
    # Initialize Inventor and Qt
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    root = SoSeparator()

    # This function contains our code fragment.
    root.addChild(makeWaterMolecule())

    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("H two O")
    myViewer.show()
    myViewer.viewAll()

    SoGui.show(myWindow)
    SoGui.mainLoop()

if __name__ == "__main__":
    main()
