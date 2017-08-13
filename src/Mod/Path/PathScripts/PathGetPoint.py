# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import Draft
import FreeCAD
import FreeCADGui

from PySide import QtCore, QtGui
from pivy import coin

class TaskPanel:

    def __init__(self, formOrig, formPoint):
        self.formOrig = formOrig
        self.formPoint = formPoint

        self.setupUi()
        self.buttonBox = None

    def setupUi(self):
        self.formPoint.buttonBox.accepted.connect(self.pointAccept)
        self.formPoint.buttonBox.rejected.connect(self.pointReject)

        self.formPoint.ifValueX.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueY.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueZ.editingFinished.connect(self.updatePoint)

    def addEscapeShortcut(self):
        # The only way I could get to intercept the escape key, or really any key was
        # by creating an action with a shortcut .....
        self.escape = QtGui.QAction(self.formPoint)
        self.escape.setText('Done')
        self.escape.setShortcut(QtGui.QKeySequence.fromString('Esc'))
        QtCore.QObject.connect(self.escape, QtCore.SIGNAL('triggered()'), self.pointDone)
        self.formPoint.addAction(self.escape)

    def removeEscapeShortcut(self):
        if self.escape:
            self.formPoint.removeAction(self.escape)
            self.escape = None

    def getPoint(self, whenDone, start=None):

        def displayPoint(p):
            self.formPoint.ifValueX.setText(FreeCAD.Units.Quantity(p.x, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueY.setText(FreeCAD.Units.Quantity(p.y, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueZ.setText(FreeCAD.Units.Quantity(p.z, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueX.setFocus()
            self.formPoint.ifValueX.selectAll()

        def mouseMove(cb):
            event = cb.getEvent()
            pos = event.getPosition()
            cntrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(pos, lastpoint=start, active=cntrl, constrain=shift)
            plane = FreeCAD.DraftWorkingPlane
            p = plane.getLocalCoords(self.pt)
            displayPoint(p)

        def click(cb):
            event = cb.getEvent()
            if event.getButton() == 1 and event.getState() == coin.SoMouseButtonEvent.DOWN:
                accept()

        def accept():
            if start:
                self.pointAccept()
            else:
                self.pointAcceptAndContinue()

        def cancel():
            self.pointCancel()

        self.pointWhenDone = whenDone
        self.formOrig.hide()
        self.formPoint.show()
        self.addEscapeShortcut()
        if start:
            displayPoint(start)
        else:
            displayPoint(FreeCAD.Vector(0, 0, 0))

        self.view = Draft.get3DView()
        self.pointCbClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), click)
        self.pointCbMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), mouseMove)

        if self.buttonBox:
            self.buttonBox.setEnabled(False)
        FreeCADGui.Snapper.forceGridOff=True

    def pointFinish(self, ok, cleanup = True):
        obj = FreeCADGui.Snapper.lastSnappedObject

        if cleanup:
            self.removeGlobalCallbacks()
            FreeCADGui.Snapper.off(True)
            if self.buttonBox:
                self.buttonBox.setEnabled(True)
            self.removeEscapeShortcut()
            self.formPoint.hide()
            self.formOrig.show()
            self.formOrig.setFocus()

        if ok:
            self.pointWhenDone(self.pt, obj)
        else:
            self.pointWhenDone(None, None)

    def pointDone(self):
        self.pointFinish(False)

    def pointReject(self):
        self.pointFinish(False)

    def pointAccept(self):
        self.pointFinish(True)

    def pointAcceptAndContinue(self):
        self.pointFinish(True, False)

    def removeGlobalCallbacks(self):
        if hasattr(self, 'view') and self.view:
            if self.pointCbClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.pointCbClick)
                self.pointCbClick = None
            if self.pointCbMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.pointCbMove)
                self.pointCbMove = None
            self.view = None

    def updatePoint(self):
        x = FreeCAD.Units.Quantity(self.formPoint.ifValueX.text()).Value
        y = FreeCAD.Units.Quantity(self.formPoint.ifValueY.text()).Value
        z = FreeCAD.Units.Quantity(self.formPoint.ifValueZ.text()).Value
        self.pt = FreeCAD.Vector(x, y, z)

