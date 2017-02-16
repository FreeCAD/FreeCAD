# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Victor Titov (DeepSOIC) <vv.titov@gmail.com>     *
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

__title__ = "CompoundTools package"
__author__ = "DeepSOIC, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"
__doc__ = """CompoundTools Package (part of FreeCAD)."""

## @package CompoundTools
#  \ingroup PART
#  \brief CompoundTools Package for Part workbench

__all__ = [
"CompoundFilter"
]


def importAll():
    "importAll(): imports all modules of CompoundTools package"
    from . import CompoundFilter


def reloadAll():
    "reloadAll(): reloads all modules of CompoundTools package. Useful for debugging."
    for modstr in __all__:
        reload(globals()[modstr])
    import FreeCAD
    if FreeCAD.GuiUp:
        addCommands()


def addCommands():
    "addCommands(): add all GUI commands of CompoundTools package to FreeCAD command manager."
    CompoundFilter.addCommands()
