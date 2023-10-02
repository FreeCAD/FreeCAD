# /***************************************************************************
# *   Copyright (c) 2019 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/


def is3DObject(obj):
    """is3DObject(obj): tests if the object has some 3d geometry.
    TempoVis is made only for objects in 3d view, so all objects that don't pass this check are ignored by TempoVis."""

    # See "Gui Problem Sketcher and TechDraw" https://forum.freecad.org/viewtopic.php?f=3&t=22797

    # observation: all viewproviders have transform node, then a switch node. If that switch node contains something, the object has something in 3d view.
    try:
        from pivy import coin

        return obj.ViewObject.SwitchNode.getNumChildren() > 0
    except Exception as err:
        import FreeCAD as App

        App.Console.PrintWarning("Show.ShowUtils.is3DObject error: {err}\n".format(err=str(err)))
        return True  # assume.
