#!/usr/bin/env python

###
# This file is part of a set of example programs for the Coin library.
# Copyright (C) 2000-2003 by Systems in Motion. All rights reserved.
#
#                  <URL:http://www.coin3d.org>
#
# This sourcecode can be redistributed and/or modified under the
# terms of the GNU General Public License version 2 as published by
# the Free Software Foundation. See the file COPYING at the root
# directory of the distribution for more details.
#
# As a special exception, all sourcecode of the demo examples can be
# used for any purpose for licensees of the Coin Professional
# Edition License, without the restrictions of the GNU GPL. See our
# web pages for information about how to acquire a Professional Edition
# License.
#
# Systems in Motion, <URL:http://www.sim.no>, <mailto:support@sim.no>
#

SOGUI_BINDING = "SoQt"

import sys

from pivy.coin import *
from pivy.sogui import *
from pivy.simvoleon import *

import utility

def main():
    window = SoGui.init(sys.argv[0])
    SoVolumeRendering.init()

    if len(sys.argv) != 2:
        sys.stdout.write("Usage: %s filename.vol\n" % sys.argv[0])
        sys.exit(1)

    root = SoSeparator()

    volumedata = SoVolumeData()

    # Add SoVolumeData to the scene graph
    filereader = SoVRVolFileReader()
    filereader.setUserData(sys.argv[1])
    volumedata.setReader(filereader)

    root.addChild(volumedata)

    # Add TransferFunction (color map) to scene graph
    transfunc = SoTransferFunction()
    root.addChild(transfunc)

    # Add VolumeRender to scene graph
    volrend = SoVolumeRender()
    root.addChild(volrend)

    ortho = SoOrthoSlice()
    ortho.alphaUse = SoOrthoSlice.ALPHA_AS_IS
    ortho.sliceNumber = 33
    root.addChild(ortho)

    viewer = SoGuiExaminerViewer(window)
    viewer.setBackgroundColor(SbColor(0.1, 0.3, 0.5))
    viewer.setSceneGraph(root)

    viewer.show()
    SoGui.show(window)
    SoGui.mainLoop()

if __name__ == '__main__':
    main()
