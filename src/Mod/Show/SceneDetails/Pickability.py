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

from Show.SceneDetail import SceneDetail


class Pickability(SceneDetail):
    """Pickability(object, pickstyle = None):Plugin for TempoVis for altering pick style
    of objects (i.e., selectability).
    pickstyle may be:
        PS_REGULAR = 0 # selectable
        PS_BOUNDBOX = 1 # selectable, but faster hit testing using bounding box
        PS_UNPICKABLE = 2 # not selectable and not obstructing."""

    class_id = "SDPickability"
    propname = ""
    objname = ""

    def __init__(self, object, pickstyle=None):
        self.objname = object.Name
        self.doc = object.Document
        self.key = self.objname
        if pickstyle is not None:
            self.data = pickstyle

    def scene_value(self):
        return getPickStyle(self.doc.getObject(self.objname).ViewObject)

    def apply_data(self, val):
        setPickStyle(self.doc.getObject(self.objname).ViewObject, val)


PS_REGULAR = 0
PS_BOUNDBOX = 1
PS_UNPICKABLE = 2


def getPickStyleNode(viewprovider, make_if_missing=True):
    from pivy import coin

    sa = coin.SoSearchAction()
    sa.setType(coin.SoPickStyle.getClassTypeId())
    sa.traverse(viewprovider.RootNode)
    if sa.isFound() and sa.getPath().getLength() == 1:
        return sa.getPath().getTail()
    else:
        if not make_if_missing:
            return None
        pick_style = coin.SoPickStyle()
        pick_style.style.setValue(coin.SoPickStyle.SHAPE)
        viewprovider.RootNode.insertChild(pick_style, 0)
        return pick_style


def getPickStyle(viewprovider):
    ps = getPickStyleNode(viewprovider, make_if_missing=False)
    if ps is not None:
        return ps.style.getValue()
    else:
        return PS_REGULAR


def setPickStyle(viewprovider, pickstyle):
    ps = getPickStyleNode(viewprovider, make_if_missing=pickstyle != 0)  # coin.SoPickStyle.SHAPE
    if ps is not None:
        return ps.style.setValue(pickstyle)
