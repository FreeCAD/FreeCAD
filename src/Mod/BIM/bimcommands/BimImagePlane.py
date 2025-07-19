# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM ImagePlane command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_ImagePlane:
    def GetResources(self):
        return {
            "Pixmap": "BIM_ImagePlane.svg",
            "MenuText": QT_TRANSLATE_NOOP("BIM_ImagePlane", "Image Plane"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_ImagePlane", "Creates a plane from an image"
            ),
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        from PySide import QtGui
        import draftguitools.gui_trackers as DraftTrackers

        self.doc = FreeCAD.ActiveDocument
        self.tracker = DraftTrackers.rectangleTracker()
        self.basepoint = None
        self.opposite = None
        (filename, _filter) = QtGui.QFileDialog.getOpenFileName(
            QtGui.QApplication.activeWindow(),
            translate("BIM", "Select Image"),
            None,
            translate("BIM", "Image file (*.png *.jpg *.bmp)"),
        )
        if filename:
            FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
            self.filename = filename
            im = QtGui.QImage(self.filename)
            self.proportion = float(im.height()) / float(im.width())
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.getPoint(
                    callback=self.PointCallback, movecallback=self.MoveCallback
                )

    def MoveCallback(self, point, snapinfo):
        import DraftVecUtils

        if point and self.basepoint and (point != self.basepoint):
            chord = point.sub(self.basepoint)
            length = DraftVecUtils.project(chord, self.tracker.u).Length
            height = length * self.proportion
            self.opposite = (
                FreeCAD.Vector(self.tracker.u)
                .multiply(length)
                .add(FreeCAD.Vector(self.tracker.v).multiply(height))
            )
            self.opposite = self.basepoint.add(self.opposite)
            self.tracker.update(self.opposite)

    def PointCallback(self, point, snapinfo):
        import os
        import DraftVecUtils
        import WorkingPlane

        if not point:
            # cancelled
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.tracker.off()
            return
        elif not self.basepoint:
            # this is our first clicked point, nothing to do just yet
            self.basepoint = point
            self.tracker.setorigin(point)
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(
                last=point, callback=self.PointCallback, movecallback=self.MoveCallback
            )
        else:
            # this is our second point
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.tracker.off()
            midpoint = self.basepoint.add(
                self.opposite.sub(self.basepoint).multiply(0.5)
            )
            wp = WorkingPlane.get_working_plane()
            rotation = wp.get_placement().Rotation
            diagonal = self.opposite.sub(self.basepoint)
            length = DraftVecUtils.project(diagonal, wp.u).Length
            height = DraftVecUtils.project(diagonal, wp.v).Length
            self.doc.openTransaction("Create image plane")
            image = self.doc.addObject("Image::ImagePlane", "ImagePlane")
            image.Label = os.path.splitext(os.path.basename(self.filename))[0]
            image.ImageFile = self.filename
            image.Placement = FreeCAD.Placement(midpoint, rotation)
            image.XSize = length
            image.YSize = height
            self.doc.commitTransaction()
            self.doc.recompute()


FreeCADGui.addCommand("BIM_ImagePlane", BIM_ImagePlane())
