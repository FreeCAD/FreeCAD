# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 wayofwood <code@wayofwood.com>                     *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import PathScripts.PathLog as PathLog
import Path

__title__ = "Keyhole Path Generator"
__author__ = "wayofwood"
__url__ = "https://www.freecad.org"
__doc__ = "Generates the toolpath for keyhole "

if True:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


def generate(face, startPoint,  endPoint):
    # Build up command stack
    cmdStack = []

    cmdStack.append( Path.Command("G1", {"X": startPoint.x,
                                         "Y": startPoint.y,
                                         "Z": startPoint.z }) )

    cmdStack.append( Path.Command("G1", {"X": endPoint.x,
                                         "Y": endPoint.y,
                                         "Z": endPoint.z }) )

    cmdStack.append( Path.Command("G1", {"X": startPoint.x,
                                         "Y": startPoint.y,
                                         "Z": startPoint.z }) )

    return cmdStack
