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


class VProperty(SceneDetail):
    """VProperty(object, propname, val = None): plugin for TempoVis to alter ViewProvider properties"""

    class_id = "SDVProperty"
    affects_persistence = True
    propname = ""
    objname = ""
    mild_restore = True

    def __init__(self, object, propname, val=None):
        self.objname = object.Name
        self.propname = propname
        self.doc = object.Document
        self.key = self.objname + "." + self.propname
        self.data = val
        if propname == "LinkVisibility":  # seems to not be a property
            self.affects_persistence = False

    def scene_value(self):
        return getattr(self.doc.getObject(self.objname).ViewObject, self.propname)

    def apply_data(self, val):
        setattr(self.doc.getObject(self.objname).ViewObject, self.propname, val)
