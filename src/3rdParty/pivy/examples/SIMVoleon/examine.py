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

    root = SoSeparator()

    dim = SbVec3s(64, 64, 64)
    voxeldata = utility.generate8bitVoxelSet(dim)

    # Add SoVolumeData to scene graph
    volumedata = SoVolumeData()
    volumedata.setVolumeData(dim, voxeldata.tostring())
    root.addChild(volumedata)

    # Add TransferFunction (color map) to scene graph
    transfunc = SoTransferFunction()
    root.addChild(transfunc)

    # Add VolumeRender to scene graph
    volrend = SoVolumeRender()
    root.addChild(volrend)

    viewer = SoGuiExaminerViewer(window)
    viewer.setBackgroundColor(SbColor(0.1, 0.3, 0.5))
    viewer.setSceneGraph(root)

    viewer.show()
    SoGui.show(window)
    SoGui.mainLoop()

if __name__ == '__main__':
    main()
