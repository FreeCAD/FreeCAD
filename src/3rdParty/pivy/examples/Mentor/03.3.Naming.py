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
# chapter 3.
#
# Create a little scene graph and then name some nodes and
# get back nodes by name.
#

import sys

from pivy.coin import *

def RemoveCube():
   # Remove the cube named 'MyCube' from the separator named
   # 'Root'.  In a real application, isOfType() would probably
   # be used to make sure the nodes are of the correct type
   # before doing the cast.
   # In Pivy no cast is necessary as it gets autocasted for you.

   myRoot = SoNode.getByName("Root")

   myCube = SoNode.getByName("MyCube")
   
   myRoot.removeChild(myCube)

def main():
   # SoDB.init() invoked automatically upon coin module import
    
   # Create some objects and give them names:
   root = SoSeparator()
   root.setName("Root")
    
   myCube = SoCube()
   root.addChild(myCube)
   myCube.setName("MyCube")
    
   mySphere = SoSphere()
   root.addChild(mySphere)
   mySphere.setName("MySphere")
    
   RemoveCube()

if __name__ == "__main__":
   main()
