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
# chapter 12, example 3.
#
# Alarm sensor that raises a flag after 10 minutes
#

import sys

from pivy.coin import *
from pivy.sogui import *

###########################################################
# CODE FOR The Inventor Mentor STARTS HERE

def raiseFlagCallback(flagAngleXform, sensor):
    # We know that flagAngleXform is an autocasted SoTransform node
    # Rotate flag by 90 degrees about the Z axis:
    flagAngleXform.rotation.setValue(SbVec3f(0,0,1), M_PI/2)

# CODE FOR The Inventor Mentor ENDS HERE
###########################################################


def main():
    myWindow = SoGui.init(sys.argv[0]) # pass the app name
    if myWindow == None: sys.exit(1)

###########################################################
# CODE FOR The Inventor Mentor STARTS HERE

    flagXform = SoTransform()

    # Create an alarm that will call the flag-raising callback:
    myAlarm = SoAlarmSensor(raiseFlagCallback, flagXform)
    myAlarm.setTimeFromNow(12.0)  # 12 seconds
    myAlarm.schedule()

# CODE FOR The Inventor Mentor ENDS HERE
###########################################################

    root = SoSeparator()
    root.addChild(flagXform)
    myCone = SoCone()
    myCone.bottomRadius = 0.1
    root.addChild(myCone)

    myViewer = SoGuiExaminerViewer(myWindow)

    # Put our scene in myViewer, change the title
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Raise The Cone")
    myViewer.show()

    SoGui.show(myWindow)  # Display main window
    SoGui.mainLoop()      # Main Inventor event loop

if __name__ == "__main__":
    main()
