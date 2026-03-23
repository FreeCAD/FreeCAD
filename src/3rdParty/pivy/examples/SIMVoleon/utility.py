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

import array
from math import sin, cos

from pivy.coin import *

def generate8bitVoxelSet(dim):
    blocksize = dim[0] * dim[1] * dim[2];
    voxels = array.array('B', [0] * dim[0] * dim[1] * dim[2])
    t = 0

    for i in range(50016):
        v = (sin((t + 1.4234) * 1.9) * sin(t) * 0.45 + 0.5,
             cos((t * 2.5) - 10) * 0.45 + 0.5,
             cos((t - 0.23123) * 3) * sin(t + 0.5) * cos(t) * 0.45 + 0.5)

        # assert(v[0] < 1.0 and v[1] < 1.0 and v[2] < 1.0)
        nx = int(dim[0] * v[0])
        ny = int(dim[1] * v[1])
        nz = int(dim[2] * v[2])

        memposition = nz*dim[0]*dim[1] + ny*dim[0] + nx
        voxels[memposition] = int((255.0 * cos(t)) % 256)

        t += 0.001

    return voxels
