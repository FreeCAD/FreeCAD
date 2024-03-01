# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD
import FreeCADGui
import Path

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")

from PySide import QtCore, QtGui
from pivy import coin

__title__ = "CAM GetPoint UI"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Helper class to use FreeCADGUi.Snapper to let the user enter arbitrary points while the task panel is active."

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.track(Path.Log.thisModule())


class TaskPanel:
    """Use an instance of this class in another TaskPanel to invoke the snapper.
    Create the instance in the TaskPanel's constructors and invoke getPoint(whenDone, start) whenever a new point is
    required or an existing point needs to be edited. The receiver is expected to have the same lifespan as the form
    provided in the constructor.
    The (only) public API function other than the constructor is getPoint(whenDone, start).
    """

    def __init__(self, form, onPath=False):
        """__init___(form) ... form will be replaced by PointEdit.ui while the Snapper is active."""
        self.formOrig = form
        self.formPoint = FreeCADGui.PySideUic.loadUi(":/panels/PointEdit.ui")

        self.formPoint.setParent(form.parent())
        form.parent().layout().addWidget(self.formPoint)
        self.formPoint.hide()

        self.setupUi()
        self.buttonBox = None
        self.onPath = onPath
        self.obj = None

        self.pt = None
        self.point = None
        self.pointCbClick = None
        self.pointCbMove = None
        self.pointWhenDone = None
        self.escape = None
        self.view = None

    def setupUi(self):
        """setupUi() ... internal function - do not call."""
        self.formPoint.buttonBox.accepted.connect(self.pointAccept)
        self.formPoint.buttonBox.rejected.connect(self.pointReject)

        self.formPoint.XGlobal.editingFinished.connect(self.updatePoint)
        self.formPoint.YGlobal.editingFinished.connect(self.updatePoint)
        self.formPoint.ZGlobal.editingFinished.connect(self.updatePoint)

        self.formPoint.XGlobal.setProperty(
            "unit", FreeCAD.Units.MilliMetre.getUserPreferred()[2]
        )
        self.formPoint.YGlobal.setProperty(
            "unit", FreeCAD.Units.MilliMetre.getUserPreferred()[2]
        )
        self.formPoint.ZGlobal.setProperty(
            "unit", FreeCAD.Units.MilliMetre.getUserPreferred()[2]
        )

    def addEscapeShortcut(self):
        """addEscapeShortcut() ... internal function - do not call."""
        # The only way I could get to intercept the escape key, or really any key was
        # by creating an action with a shortcut .....
        self.escape = QtGui.QAction(self.formPoint)
        self.escape.setText("Done")
        self.escape.setShortcut(QtGui.QKeySequence.fromString("Esc"))
        QtCore.QObject.connect(
            self.escape, QtCore.SIGNAL("triggered()"), self.pointDone
        )
        self.formPoint.addAction(self.escape)

    def removeEscapeShortcut(self):
        """removeEscapeShortcut() ... internal function - do not call."""
        if self.escape:
            self.formPoint.removeAction(self.escape)
            self.escape = None

    def getPoint(self, whenDone, start=None):
        """getPoint(whenDone, start=None) ... invoke Snapper and call whenDone when a point is entered or the user cancels the operation.
        whenDone(point, obj) is called either with a point and the object on which the point lies if the user set the point,
        or None and None if the user cancelled the operation.
        start is an optional Vector indicating from where to start Snapper. This is mostly used when editing existing points. Snapper also
        creates a dotted line indicating from where the original point started from.
        If start is specified the Snapper UI is closed on the first point the user enters. If start remains None, then Snapper is kept open
        until the user explicitly closes Snapper. This lets the user enter multiple points in quick succession."""

        # there's no get point without Snapper, if it's not loaded, need to do that explicitly
        if not hasattr(FreeCADGui, "Snapper"):
            import DraftTools

        def displayPoint(p):
            self.point = p
            self.formPoint.XGlobal.setProperty("rawValue", p.x)
            self.formPoint.YGlobal.setProperty("rawValue", p.y)
            self.formPoint.ZGlobal.setProperty("rawValue", p.z)
            self.formPoint.XGlobal.setFocus()
            self.formPoint.XGlobal.selectAll()

        def mouseMove(cb):
            p = None
            event = cb.getEvent()
            pos = event.getPosition()
            if self.onPath:
                # There should not be a dependency from Draft->Path, so handle Path "snapping"
                # directly, at least for now. Simple enough because there isn't really any
                # "snapping" going on other than what getObjectInfo() provides.
                screenpos = tuple(pos.getValue())
                snapInfo = Draft.get3DView().getObjectInfo(screenpos)
                if snapInfo:
                    obj = FreeCAD.ActiveDocument.getObject(snapInfo["Object"])
                    if hasattr(obj, "Path"):
                        self.obj = obj
                        p = FreeCAD.Vector(snapInfo["x"], snapInfo["y"], snapInfo["z"])
                        self.pt = p
                    else:
                        self.obj = None
            else:
                # Snapper handles regular objects just fine
                cntrl = event.wasCtrlDown()
                shift = event.wasShiftDown()
                self.pt = FreeCADGui.Snapper.snap(
                    pos, lastpoint=start, active=cntrl, constrain=shift
                )
                plane = FreeCAD.DraftWorkingPlane
                p = plane.getLocalCoords(self.pt)
                self.obj = FreeCADGui.Snapper.lastSnappedObject

            if p:
                displayPoint(p)

        def click(cb):
            event = cb.getEvent()
            if (
                event.getButton() == 1
                and event.getState() == coin.SoMouseButtonEvent.DOWN
            ):
                if self.obj:
                    accept()

        def accept():
            if start:
                self.pointAccept()
            else:
                self.pointAcceptAndContinue()

        def cancel():
            self.pointReject()

        self.pointWhenDone = whenDone
        self.formOrig.hide()
        self.formPoint.show()
        self.addEscapeShortcut()
        if start:
            displayPoint(start)
        else:
            displayPoint(FreeCAD.Vector(0, 0, 0))

        self.view = Draft.get3DView()
        self.pointCbClick = self.view.addEventCallbackPivy(
            coin.SoMouseButtonEvent.getClassTypeId(), click
        )
        self.pointCbMove = self.view.addEventCallbackPivy(
            coin.SoLocation2Event.getClassTypeId(), mouseMove
        )

        if self.buttonBox:
            self.buttonBox.setEnabled(False)

        FreeCADGui.Snapper.forceGridOff = True

    def pointFinish(self, ok, cleanup=True):
        """pointFinish(ok, cleanup=True) ... internal function - do not call."""

        if cleanup:
            self.removeGlobalCallbacks()
            FreeCADGui.Snapper.off()
            if self.buttonBox:
                self.buttonBox.setEnabled(True)
            self.removeEscapeShortcut()
            self.formPoint.hide()
            self.formOrig.show()
            self.formOrig.setFocus()

        if ok:
            self.updatePoint(False)
            self.pointWhenDone(self.pt, self.obj)
        else:
            self.pointWhenDone(None, None)

    def pointDone(self):
        """pointDone() ... internal function - do not call."""
        self.pointFinish(False)

    def pointReject(self):
        """pointReject() ... internal function - do not call."""
        self.pointFinish(False)

    def pointAccept(self):
        """pointAccept() ... internal function - do not call."""
        self.pointFinish(True)

    def pointAcceptAndContinue(self):
        """pointAcceptAndContinue() ... internal function - do not call."""
        self.pointFinish(True, False)

    def removeGlobalCallbacks(self):
        """removeGlobalCallbacks() ... internal function - do not call."""
        if hasattr(self, "view") and self.view:
            if self.pointCbClick:
                self.view.removeEventCallbackPivy(
                    coin.SoMouseButtonEvent.getClassTypeId(), self.pointCbClick
                )
                self.pointCbClick = None
            if self.pointCbMove:
                self.view.removeEventCallbackPivy(
                    coin.SoLocation2Event.getClassTypeId(), self.pointCbMove
                )
                self.pointCbMove = None
            self.view = None

    def updatePoint(self, usePoint=True):
        """updatePoint() ... internal function - do not call."""
        if usePoint and self.point:
            self.pt = self.point
        else:
            x = FreeCAD.Units.Quantity(self.formPoint.XGlobal.text()).Value
            y = FreeCAD.Units.Quantity(self.formPoint.YGlobal.text()).Value
            z = FreeCAD.Units.Quantity(self.formPoint.ZGlobal.text()).Value
            self.pt = FreeCAD.Vector(x, y, z)
