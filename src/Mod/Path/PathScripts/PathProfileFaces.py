# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Schildkroet                                        *
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
# *   Major modifications: 2020 Russell Johnson <russ4262@gmail.com>        *

import FreeCAD
import PathScripts.PathProfile as PathProfile


__title__ = "Path Profile Faces Operation (depreciated)"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecadweb.org"
__doc__ = "Path Profile operation based on faces (depreciated)."
__contributors__ = "Schildkroet"


class ObjectProfile(PathProfile.ObjectProfile):
    """Pseudo class for Profile operation,
    allowing for backward compatibility with pre-existing "Profile Faces" operations."""

    pass


# Eclass


def SetupProperties():
    return PathProfile.SetupProperties()


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Profile operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name, parentJob)
    return obj
