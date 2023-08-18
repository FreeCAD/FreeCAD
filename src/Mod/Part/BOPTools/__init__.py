#/***************************************************************************
# *   Copyright (c) 2016 Victor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

__title__ = "BOPTools package"
__url__ = "http://www.freecad.org"
__doc__ = """BOPTools Package (part of FreeCAD). Routines that power Connect, Embed, Cutout,
BooleanFragments, Slice and XOR features of Part Workbench. Useful for other custom
BOP-like operations"""

## @package BOPTools
#  \ingroup PART

__all__ = [
"GeneralFuseResult",
"JoinAPI",
"JoinFeatures",
"ShapeMerge",
"Utils",
"SplitAPI",
"SplitFeatures",
]

def importAll():
    "importAll(): imports all modules of BOPTools package"
    from . import GeneralFuseResult
    from . import JoinAPI
    from . import JoinFeatures
    from . import ShapeMerge
    from . import Utils
    from . import SplitAPI
    from . import SplitFeatures

def reloadAll():
    "reloadAll(): reloads all modules of BOPTools package. Useful for debugging."
    for modstr in __all__:
        reload(globals()[modstr])
    import FreeCAD
    if FreeCAD.GuiUp:
        addCommands()

def addCommands():
    "addCommands(): add all GUI commands of BOPTools package to FreeCAD command manager."
    JoinFeatures.addCommands()
    SplitFeatures.addCommands()

