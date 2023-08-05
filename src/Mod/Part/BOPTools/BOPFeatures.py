# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "BOPTools.BOPFeatures module"
__author__ = "Werner Mayer"
__url__ = "http://www.freecad.org"
__doc__ = "Helper class to create the features for Boolean operations."

import FreeCAD
import Part

class BOPFeatures:
    def __init__(self, doc):
        self.doc = doc

    def make_section(self, inputNames):
        obj = self.doc.addObject("Part::Section", "Section")
        obj.Base = self.doc.getObject(inputNames[0])
        obj.Tool = self.doc.getObject(inputNames[1])
        self.copy_visual_attributes(obj, obj.Base)
        target = self.move_input_objects([obj.Base, obj.Tool])
        if target:
            target.addObject(obj)
        return obj

    def make_cut(self, inputNames):
        obj = self.doc.addObject("Part::Cut", "Cut")
        obj.Base = self.doc.getObject(inputNames[0])
        obj.Tool = self.doc.getObject(inputNames[1])
        self.copy_visual_attributes(obj, obj.Base)
        target = self.move_input_objects([obj.Base, obj.Tool])
        if target:
            target.addObject(obj)
        return obj

    def make_common(self, inputNames):
        obj = self.doc.addObject("Part::Common", "Common")
        obj.Base = self.doc.getObject(inputNames[0])
        obj.Tool = self.doc.getObject(inputNames[1])
        self.copy_visual_attributes(obj, obj.Base)
        target = self.move_input_objects([obj.Base, obj.Tool])
        if target:
            target.addObject(obj)
        return obj

    def make_multi_common(self, inputNames):
        obj = self.doc.addObject("Part::MultiCommon", "Common")
        obj.Shapes = [self.doc.getObject(name) for name in inputNames]
        self.copy_visual_attributes(obj, obj.Shapes[0])
        target = self.move_input_objects(obj.Shapes)
        if target:
            target.addObject(obj)
        return obj

    def make_fuse(self, inputNames):
        obj = self.doc.addObject("Part::Fuse", "Fusion")
        obj.Base = self.doc.getObject(inputNames[0])
        obj.Tool = self.doc.getObject(inputNames[1])
        self.copy_visual_attributes(obj, obj.Base)
        target = self.move_input_objects([obj.Base, obj.Tool])
        if target:
            target.addObject(obj)
        return obj

    def make_multi_fuse(self, inputNames):
        obj = self.doc.addObject("Part::MultiFuse", "Fusion")
        obj.Shapes = [self.doc.getObject(name) for name in inputNames]
        self.copy_visual_attributes(obj, obj.Shapes[0])
        target = self.move_input_objects(obj.Shapes)
        if target:
            target.addObject(obj)
        return obj

    def move_input_objects(self, objects):
        targetGroup = None
        for obj in objects:
            obj.Visibility = False
            parent = obj.getParent()
            if parent:
                parent.removeObject(obj)
                targetGroup = parent
        return targetGroup

    def copy_visual_attributes(self, target, source):
        if target.ViewObject:
            target.ViewObject.ShapeColor = source.ViewObject.ShapeColor
            target.ViewObject.DisplayMode = source.ViewObject.DisplayMode
