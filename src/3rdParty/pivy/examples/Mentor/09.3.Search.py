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
# chapter 9, example 3.
#
# Search Action example.
# Read in a scene from a file.
# Search through the scene looking for a light.
# If none exists, add a directional light to the scene
# and print out the modified scene.
#

import sys

from pivy.coin import *

def main():
    # Initialize Inventor
    # SoDB.init() invoked automatically upon coin module import
    
    # Open and read input scene graph
    sceneInput = SoInput()
    if not sceneInput.openFile("bird.iv"):
        return 1

    root = SoDB.readAll(sceneInput)
    if root == None:
        return 1

##############################################################
# CODE FOR The Inventor Mentor STARTS HERE

    mySearchAction = SoSearchAction()

    # Look for first existing light derived from class SoLight
    mySearchAction.setType(SoLight.getClassTypeId())
    mySearchAction.setInterest(SoSearchAction.FIRST)
    
    mySearchAction.apply(root)
    if mySearchAction.getPath() == None: # No lights found
        # Add a default directional light to the scene
        myLight = SoDirectionalLight()
        root.insertChild(myLight, 0)

# CODE FOR The Inventor Mentor ENDS HERE
##############################################################

    myWriteAction = SoWriteAction()
    myWriteAction.apply(root)

    return 0

if __name__ == "__main__":
    sys.exit(main())
