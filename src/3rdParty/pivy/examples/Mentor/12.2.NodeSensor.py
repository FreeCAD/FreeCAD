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
# chapter 12, example 2.
#
# Using getTriggerNode/getTriggerField methods of the data
# sensor.
#

from __future__ import print_function
import sys

from pivy.coin import *

# Sensor callback function:
def rootChangedCB(void, mySensor):
    # Sensors get autocasted; there is no need to cast them manually
    # through the cast() function, such as:
    #   mySensor = cast(s, "SoDataSensor")
    # mySensor is therefore a SoNodeSensor.

    changedNode = mySensor.getTriggerNode()
    changedField = mySensor.getTriggerField()
    
    print("The node named '%s' changed" % (changedNode.getName().getString()))

    if changedField:
        # the pythonic getFieldName() method returns a string or None in Pivy.
        fieldName = changedNode.getFieldName(changedField)
        print(" (field %s)" % (fieldName))
    else:
        print(" (no fields changed)")

def main():
    # SoDB.init() invoked automatically upon coin module import

    root = SoSeparator()
    root.setName("Root")

    myCube = SoCube()
    root.addChild(myCube)
    myCube.setName("MyCube")

    mySphere = SoSphere()
    root.addChild(mySphere)
    mySphere.setName("MySphere")

    mySensor = SoNodeSensor(rootChangedCB, None)
    mySensor.setPriority(0)
    # mySensor.setFunction(rootChangedCB)
    mySensor.attach(root)

    # Now, make a few changes to the scene graph the sensor's
    # callback function will be called immediately after each
    # change.
    myCube.width = 1.0
    myCube.height = 2.0
    mySphere.radius = 3.0
    root.removeChild(mySphere)

if __name__ == "__main__":
    main()
