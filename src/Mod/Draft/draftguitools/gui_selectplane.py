# ***************************************************************************
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
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
"""Provides GUI tools to set up the working plane and its grid."""
## @package gui_selectplane
# \ingroup draftguitools
# \brief Provides GUI tools to set up the working plane and its grid.

## \addtogroup draftguitools
# @{
import math
import pivy.coin as coin
import PySide.QtGui as QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD
import FreeCADGui
import Draft
import Draft_rc
import DraftVecUtils
import drafttaskpanels.task_selectplane as task_selectplane

from draftutils.todo import todo
from draftutils.messages import _msg
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False

__title__ = "FreeCAD Draft Workbench GUI Tools - Working plane-related tools"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin")
__url__ = "https://www.freecad.org"


class Draft_SelectPlane:
    """The Draft_SelectPlane FreeCAD command definition."""

    def __init__(self):
        self.ac = "FreeCAD.DraftWorkingPlane.alignToPointAndAxis"
        self.param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
        self.states = []

    def GetResources(self):
        """Set icon, menu and tooltip."""
        d = {'Pixmap': 'Draft_SelectPlane',
             'Accel': "W, P",
             'MenuText': QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select Plane"),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_SelectPlane", "Select the face of solid body to create a working plane on which to sketch Draft objects.\nYou may also select a three vertices or a Working Plane Proxy.")}
        return d

    def IsActive(self):
        """Return True when this command should be available."""
        if FreeCADGui.ActiveDocument:
            return True
        else:
            return False

    def Activated(self):
        """Execute when the command is called."""
        # finish active Draft command if any
        if FreeCAD.activeDraftCommand is not None:
            FreeCAD.activeDraftCommand.finish()

        # Reset variables
        self.view = Draft.get3DView()
        self.wpButton = FreeCADGui.draftToolBar.wplabel
        FreeCAD.DraftWorkingPlane.setup()

        # Write current WP if states are empty
        if not self.states:
            p = FreeCAD.DraftWorkingPlane
            self.states.append([p.u, p.v, p.axis, p.position])

        # Create task panel
        FreeCADGui.Control.closeDialog()
        self.taskd = task_selectplane.SelectPlaneTaskPanel()
        self.taskd.reject = self.reject

        # Fill values
        self.taskd.form.checkCenter.setChecked(self.param.GetBool("CenterPlaneOnView", False))
        try:
            q = FreeCAD.Units.Quantity(self.param.GetString("gridSpacing", "1 mm"))
        except ValueError:
            q = FreeCAD.Units.Quantity("1 mm")
        self.taskd.form.fieldGridSpacing.setText(q.UserString)
        self.taskd.form.fieldGridMainLine.setValue(self.param.GetInt("gridEvery", 10))
        self.taskd.form.fieldGridExtension.setValue(self.param.GetInt("gridSize", 100))
        self.taskd.form.fieldSnapRadius.setValue(self.param.GetInt("snapRange", 8))

        # Set icons
        self.taskd.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"))
        self.taskd.form.buttonTop.setIcon(QtGui.QIcon(":/icons/view-top.svg"))
        self.taskd.form.buttonFront.setIcon(QtGui.QIcon(":/icons/view-front.svg"))
        self.taskd.form.buttonSide.setIcon(QtGui.QIcon(":/icons/view-right.svg"))
        self.taskd.form.buttonAlign.setIcon(QtGui.QIcon(":/icons/view-isometric.svg"))
        self.taskd.form.buttonAuto.setIcon(QtGui.QIcon(":/icons/view-axonometric.svg"))
        self.taskd.form.buttonMove.setIcon(QtGui.QIcon(":/icons/Draft_Move.svg"))
        self.taskd.form.buttonCenter.setIcon(QtGui.QIcon(":/icons/view-fullscreen.svg"))
        self.taskd.form.buttonPrevious.setIcon(QtGui.QIcon(":/icons/edit-undo.svg"))

        # Connect slots
        self.taskd.form.buttonTop.clicked.connect(self.onClickTop)
        self.taskd.form.buttonFront.clicked.connect(self.onClickFront)
        self.taskd.form.buttonSide.clicked.connect(self.onClickSide)
        self.taskd.form.buttonAlign.clicked.connect(self.onClickAlign)
        self.taskd.form.buttonAuto.clicked.connect(self.onClickAuto)
        self.taskd.form.buttonMove.clicked.connect(self.onClickMove)
        self.taskd.form.buttonCenter.clicked.connect(self.onClickCenter)
        self.taskd.form.buttonPrevious.clicked.connect(self.onClickPrevious)
        self.taskd.form.fieldGridSpacing.textEdited.connect(self.onSetGridSize)
        self.taskd.form.fieldGridMainLine.valueChanged.connect(self.onSetMainline)
        self.taskd.form.fieldGridExtension.valueChanged.connect(self.onSetExtension)
        self.taskd.form.fieldSnapRadius.valueChanged.connect(self.onSetSnapRadius)

        # save previous WP to ensure back to the last used when restored
        FreeCAD.DraftWorkingPlane.save()

        # Try to find a WP from the current selection
        if FreeCADGui.Selection.getSelectionEx(FreeCAD.ActiveDocument.Name):
            if self.handle():
                pass
            # Try another method
            elif FreeCAD.DraftWorkingPlane.alignToSelection():
                FreeCADGui.Selection.clearSelection()
                self.display(FreeCAD.DraftWorkingPlane.axis)
            return None

        # Execute the actual task panel delayed to catch possible active Draft command
        todo.delay(FreeCADGui.Control.showDialog, self.taskd)
        _msg(translate(
                "draft",
                "Pick a face, 3 vertices or a WP Proxy to define the drawing plane"))
        self.call = self.view.addEventCallback("SoEvent", self.action)

    def finish(self):
        """Execute when the command is terminated."""
        # Store values
        self.param.SetBool("CenterPlaneOnView",
                           self.taskd.form.checkCenter.isChecked())

        # Terminate coin callbacks
        if self.call:
            try:
                self.view.removeEventCallback("SoEvent", self.call)
            except RuntimeError:
                # The view has been deleted already
                pass
            self.call = None

        # Reset everything else
        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):
        """Execute when clicking the Cancel button."""
        self.finish()
        return True

    def action(self, arg):
        """Set the callbacks for the view."""
        if arg["Type"] == "SoKeyboardEvent" and arg["Key"] == "ESCAPE":
            self.finish()
        if arg["Type"] == "SoMouseButtonEvent":
            if (arg["State"] == "DOWN") and (arg["Button"] == "BUTTON1"):
                # Coin detection happens before the selection
                # got a chance of being updated, so we must delay
                todo.delay(self.checkSelection, None)

    def checkSelection(self):
        """Check the selection, if there is a handle, finish the command."""
        if self.handle():
            self.finish()

    def handle(self):
        """Build a working plane. Return True if successful."""
        sel = FreeCADGui.Selection.getSelectionEx()
        if len(sel) == 1:
            sel = sel[0]
            if hasattr(sel.Object, 'TypeId') and (sel.Object.TypeId == 'App::Part' or sel.Object.TypeId == 'PartDesign::Plane'):
                self.setPlaneFromObjPlacement(sel.Object)
                return True
            elif Draft.getType(sel.Object) == "Axis":
                FreeCAD.DraftWorkingPlane.alignToEdges(sel.Object.Shape.Edges)
                self.display(FreeCAD.DraftWorkingPlane.axis)
                return True
            elif Draft.getType(sel.Object) in ("WorkingPlaneProxy", "BuildingPart"):
                self.setPlaneOnWPProxy(sel.Object)
                return True
            elif Draft.getType(sel.Object) == "SectionPlane":
                self.setPlaneFromObjPlacement(sel.Object)
                return True
            elif sel.HasSubObjects:
                if len(sel.SubElementNames) == 1:
                    # look for a face or a plane
                    if "Face" in sel.SubElementNames[0]:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.SubObjects[0],
                                                              self.getOffset(),
                                                              sel.Object.getParentGeoFeatureGroup())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
                    elif sel.SubElementNames[0] == "Plane":
                        FreeCAD.DraftWorkingPlane.setFromPlacement(sel.Object.Placement, rebase=True)
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
                elif len(sel.SubElementNames) == 3:
                    # look for 3 points
                    if ("Vertex" in sel.SubElementNames[0]) \
                    and ("Vertex" in sel.SubElementNames[1]) \
                    and ("Vertex" in sel.SubElementNames[2]):
                        FreeCAD.DraftWorkingPlane.alignTo3Points(sel.SubObjects[0].Point,
                                                                 sel.SubObjects[1].Point,
                                                                 sel.SubObjects[2].Point,
                                                                 self.getOffset())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
            elif sel.Object.isDerivedFrom("Part::Feature"):
                if sel.Object.Shape:
                    if len(sel.Object.Shape.Faces) == 1:
                        FreeCAD.DraftWorkingPlane.alignToFace(sel.Object.Shape.Faces[0], self.getOffset())
                        self.display(FreeCAD.DraftWorkingPlane.axis)
                        return True
            elif hasattr(sel.Object, 'Placement'):
                self.setPlaneFromObjPlacement(sel.Object)
                return True

        elif sel:
            # look for 3 points
            subs = []
            import Part
            for s in sel:
                for so in s.SubObjects:
                    if isinstance(so, Part.Vertex):
                        subs.append(so)
            if len(subs) == 3:
                FreeCAD.DraftWorkingPlane.alignTo3Points(subs[0].Point,
                                                         subs[1].Point,
                                                         subs[2].Point,
                                                         self.getOffset())
                self.display(FreeCAD.DraftWorkingPlane.axis)
                return True
        return False

    def setPlaneFromObjPlacement(self, obj):
        """Called by handle(): set the working plane according to an object placement."""
        if hasattr(obj, 'getGlobalPlacement'):
            pl = obj.getGlobalPlacement()
        else:
            pl = obj.Placement
        FreeCAD.DraftWorkingPlane.setFromPlacement(pl, rebase=True)
        FreeCAD.DraftWorkingPlane.weak = False
        self.display(FreeCAD.DraftWorkingPlane.axis,obj.ViewObject.Icon)
        self.wpButton.setText(obj.Label)
        self.wpButton.setToolTip(translate("draft", "Current working plane")+": " + self.wpButton.text())
        m = translate("draft", "Working plane aligned to global placement of")
        _msg(m + " " + obj.Label + ".\n")
        return True

    def setPlaneOnWPProxy(self, obj):
        """Called by handle(): set the working plane according to a WorkingPlaneProxy or a BuildingPart.
        This method also apply the clipping view according to object properties.
        """
        FreeCAD.DraftWorkingPlane.setFromPlacement(obj.Placement, rebase=True)
        FreeCAD.DraftWorkingPlane.weak = False
        if hasattr(obj.ViewObject, "AutoWorkingPlane"):
            if obj.ViewObject.AutoWorkingPlane:
                FreeCAD.DraftWorkingPlane.weak = True
        if hasattr(obj.ViewObject, "CutView") and hasattr(obj.ViewObject, "AutoCutView"):
            if obj.ViewObject.AutoCutView:
                obj.ViewObject.CutView = True
        if hasattr(obj.ViewObject, "RestoreView"):
            if obj.ViewObject.RestoreView:
                if hasattr(obj.ViewObject, "ViewData"):
                    if len(obj.ViewObject.ViewData) >= 12:
                        d = obj.ViewObject.ViewData
                        camtype = "orthographic"
                        if len(obj.ViewObject.ViewData) == 13:
                            if d[12] == 1:
                                camtype = "perspective"
                        c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                        if isinstance(c, coin.SoOrthographicCamera):
                            if camtype == "perspective":
                                FreeCADGui.ActiveDocument.ActiveView.setCameraType("Perspective")
                        elif isinstance(c, coin.SoPerspectiveCamera):
                            if camtype == "orthographic":
                                FreeCADGui.ActiveDocument.ActiveView.setCameraType("Orthographic")
                        c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
                        c.position.setValue([d[0], d[1], d[2]])
                        c.orientation.setValue([d[3], d[4], d[5], d[6]])
                        c.nearDistance.setValue(d[7])
                        c.farDistance.setValue(d[8])
                        c.aspectRatio.setValue(d[9])
                        c.focalDistance.setValue(d[10])
                        if camtype == "orthographic":
                            c.height.setValue(d[11])
                        else:
                            c.heightAngle.setValue(d[11])
        if hasattr(obj.ViewObject, "RestoreState"):
            if obj.ViewObject.RestoreState:
                if hasattr(obj.ViewObject, "VisibilityMap"):
                    if obj.ViewObject.VisibilityMap:
                        for k,v in obj.ViewObject.VisibilityMap.items():
                            o = FreeCADGui.ActiveDocument.getObject(k)
                            if o:
                                if o.Visibility != (v == "True"):
                                    FreeCADGui.doCommand("FreeCADGui.ActiveDocument.getObject(\""+k+"\").Visibility = "+v)
        self.display(FreeCAD.DraftWorkingPlane.axis,obj.ViewObject.Icon)
        self.wpButton.setText(obj.Label)
        self.wpButton.setToolTip(translate("draft", "Current working plane")+": "+self.wpButton.text())

    def getCenterPoint(self, x, y, z):
        """Get the center point."""
        if not self.taskd.form.checkCenter.isChecked():
            return FreeCAD.Vector()
        v = FreeCAD.Vector(x, y, z)
        view = FreeCADGui.ActiveDocument.ActiveView
        camera = view.getCameraNode()
        cam1 = FreeCAD.Vector(camera.position.getValue().getValue())
        cam2 = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
        vcam1 = DraftVecUtils.project(cam1, v)
        a = vcam1.getAngle(cam2)
        if a < 0.0001:
            return FreeCAD.Vector()
        d = vcam1.Length
        L = d/math.cos(a)
        vcam2 = DraftVecUtils.scaleTo(cam2, L)
        cp = cam1.add(vcam2)
        return cp

    def tostr(self, v):
        """Make a string from a vector or tuple."""
        string = "FreeCAD.Vector("
        string += str(v[0]) + ", "
        string += str(v[1]) + ", "
        string += str(v[2]) + ")"
        return string

    def getOffset(self):
        """Return the offset value as a float in mm."""
        try:
            o = float(self.taskd.form.fieldOffset.text())
        except Exception:
            o = FreeCAD.Units.Quantity(self.taskd.form.fieldOffset.text())
            o = o.Value
        return o

    def onClickTop(self):
        """Execute when pressing the top button."""
        _cmd = self.ac
        _cmd += "("
        _cmd += self.tostr(self.getCenterPoint(0, 0, 1)) + ", "
        _cmd += self.tostr((0, 0, 1)) + ", "
        _cmd += str(self.getOffset())
        _cmd += ")"
        FreeCADGui.doCommandGui(_cmd)
        self.display(translate("draft",'Top'),QtGui.QIcon(":/icons/view-top.svg"))
        self.finish()

    def onClickFront(self):
        """Execute when pressing the front button."""
        _cmd = self.ac
        _cmd += "("
        _cmd += self.tostr(self.getCenterPoint(0, -1, 0)) + ", "
        _cmd += self.tostr((0, -1, 0)) + ", "
        _cmd += str(self.getOffset())
        _cmd += ")"
        FreeCADGui.doCommandGui(_cmd)
        self.display(translate("draft",'Front'),QtGui.QIcon(":/icons/view-front.svg"))
        self.finish()

    def onClickSide(self):
        """Execute when pressing the side button."""
        _cmd = self.ac
        _cmd += "("
        _cmd += self.tostr(self.getCenterPoint(1, 0, 0)) + ", "
        _cmd += self.tostr((1, 0, 0)) + ", "
        _cmd += str(self.getOffset())
        _cmd += ")"
        FreeCADGui.doCommandGui(_cmd)
        self.display(translate("draft",'Side'),QtGui.QIcon(":/icons/view-right.svg"))
        self.finish()

    def onClickAlign(self):
        """Execute when pressing the align."""
        dir = self.view.getViewDirection().negative()
        camera = self.view.getCameraNode()
        rot = camera.getField("orientation").getValue()
        coin_up = coin.SbVec3f(0, 1, 0)
        upvec = FreeCAD.Vector(rot.multVec(coin_up).getValue())
        _cmd = self.ac
        _cmd += "("
        _cmd += self.tostr(self.getCenterPoint(dir.x, dir.y, dir.z)) + ", "
        _cmd += self.tostr((dir.x, dir.y, dir.z)) + ", "
        _cmd += str(self.getOffset()) + ", "
        _cmd += self.tostr(upvec)
        _cmd += ")"
        FreeCADGui.doCommandGui(_cmd)
        self.display(dir)
        self.finish()

    def onClickAuto(self):
        """Execute when pressing the auto button."""
        FreeCADGui.doCommandGui("FreeCAD.DraftWorkingPlane.reset()")
        self.display('Auto')
        self.finish()

    def onClickMove(self):
        """Execute when pressing the move button."""
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            verts = []
            import Part
            for s in sel:
                for so in s.SubObjects:
                    if isinstance(so, Part.Vertex):
                        verts.append(so)
            if len(verts) == 1:
                target = verts[0].Point
                FreeCAD.DraftWorkingPlane.position = target
                self.display(target)
                self.finish()
        else:
            # move the WP to the center of the current view
            c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
            p = FreeCAD.Vector(c.position.getValue().getValue())
            d = FreeCADGui.ActiveDocument.ActiveView.getViewDirection()
            pp = FreeCAD.DraftWorkingPlane.projectPoint(p, d)
            FreeCAD.DraftWorkingPlane.position = pp
            self.display(pp)
            self.finish()

    def onClickCenter(self):
        """Execute when pressing the center button."""
        c = FreeCADGui.ActiveDocument.ActiveView.getCameraNode()
        r = FreeCAD.DraftWorkingPlane.getRotation().Rotation.Q
        c.orientation.setValue(r)
        # calculate delta
        p = FreeCAD.Vector(c.position.getValue().getValue())
        pp = FreeCAD.DraftWorkingPlane.projectPoint(p)
        delta = pp.negative()  # to bring it above the (0,0) point
        np = p.add(delta)
        c.position.setValue(tuple(np))
        self.finish()

    def onClickPrevious(self):
        """Execute when pressing the previous button."""
        p = FreeCAD.DraftWorkingPlane
        if len(self.states) > 1:
            self.states.pop()  # discard the last one
            s = self.states[-1]
            p.u = s[0]
            p.v = s[1]
            p.axis = s[2]
            p.position = s[3]
            FreeCADGui.Snapper.setGrid()
            self.finish()

    def onSetGridSize(self, text):
        """Execute when setting the grid size."""
        try:
            q = FreeCAD.Units.Quantity(text)
        except Exception:
            pass
        else:
            self.param.SetString("gridSpacing", q.UserString)
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.setGrid()

    def onSetMainline(self, i):
        """Execute when setting main line grid spacing."""
        if i > 1:
            self.param.SetInt("gridEvery", i)
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.setGrid()

    def onSetExtension(self, i):
        """Execute when setting grid extension."""
        if i > 1:
            self.param.SetInt("gridSize", i)
            if hasattr(FreeCADGui, "Snapper"):
                FreeCADGui.Snapper.setGrid()

    def onSetSnapRadius(self, i):
        """Execute when setting the snap radius."""
        self.param.SetInt("snapRange", i)
        if hasattr(FreeCADGui, "Snapper"):
            FreeCADGui.Snapper.showradius()

    def display(self, arg, icon=None):
        """Set the text and icon of the working plane button in the toolbar."""
        o = self.getOffset()
        if o:
            if o > 0:
                suffix = ' +O'
            else:
                suffix = ' -O'
        else:
            suffix = ''
        _vdir = FreeCAD.DraftWorkingPlane.axis
        vdir = '('
        vdir += str(_vdir.x)[:4] + ','
        vdir += str(_vdir.y)[:4] + ','
        vdir += str(_vdir.z)[:4] + ')' + ' '
        vdir += translate("draft", "Dir", "Dir here means Direction, not Directory. Also shorten the translation because of available space in GUI")
        vdir = ': ' + vdir
        if type(arg).__name__ == 'str':
            self.wpButton.setText(arg + suffix)
            if o != 0:
                o = " " + translate("draft", "Offset") + ": " + str(o)
            else:
                o = ""
            _tool = translate("draft", "Current working plane") + ": "
            _tool += self.wpButton.text() + o + vdir
            self.wpButton.setToolTip(_tool)
        elif type(arg).__name__ == 'Vector':
            plv = '('
            plv += str(arg.x)[:6] + ','
            plv += str(arg.y)[:6] + ','
            plv += str(arg.z)[:6]
            plv += ')'
            self.wpButton.setText(translate("draft", "Custom"))
            _tool = translate("draft", "Current working plane")
            _tool += ": " + plv + vdir
            self.wpButton.setToolTip(_tool)
        if icon:
            self.wpButton.setIcon(icon)
        else:
            self.wpButton.setIcon(QtGui.QIcon(":/icons/Draft_SelectPlane.svg"))
        p = FreeCAD.DraftWorkingPlane
        self.states.append([p.u, p.v, p.axis, p.position])
        FreeCADGui.doCommandGui("FreeCADGui.Snapper.setGrid()")


FreeCADGui.addCommand('Draft_SelectPlane', Draft_SelectPlane())

## @}
