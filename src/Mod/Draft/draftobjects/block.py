# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
"""Provides the object code for the Block object."""
## @package block
# \ingroup draftobjects
# \brief Provides the object code for the Block object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

from draftobjects.base import DraftObject


class Block(DraftObject):
    """The Block object"""

    def __init__(self, obj):
        super(Block, self).__init__(obj, "Block")

        _tip = QT_TRANSLATE_NOOP("App::Property",
                "The components of this block")
        obj.addProperty("App::PropertyLinkList","Components", "Draft", _tip)

    def execute(self, obj):
        if self.props_changed_placement_only(obj):
            obj.positionBySupport()
            self.props_changed_clear()
            return

        import Part
        plm = obj.Placement
        shps = []
        for c in obj.Components:
            shps.append(c.Shape)
        if shps:
            shape = Part.makeCompound(shps)
            obj.Shape = shape
        obj.Placement = plm
        obj.positionBySupport()
        self.props_changed_clear()

    def onChanged(self, obj, prop):
        self.props_changed_store(prop)


# Alias for compatibility with v0.18 and earlier
_Block = Block

## @}
