# ***************************************************************************
# *   (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>                  *
# *   (c) 2009, 2010 Ken Cline <cline@frii.com>                             *
# *   (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de>           *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides GUI tools to create circular arc objects."""
## @package gui_arcs
# \ingroup draftguitools
# \brief Provides GUI tools to create circular arc objects.

## \addtogroup draftguitools
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import Draft_rc
import DraftVecUtils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_base as gui_base
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers
import draftutils.utils as utils

from FreeCAD import Units as U
from draftutils.messages import _msg, _err
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Arc(gui_base_original.Creator):
    """Gui command for the Circular Arc tool."""

    def __init__(self):
        super().__init__()
        self.closedCircle = False
        self.featureName = "Arc"

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': 'Draft_Arc',
                'Accel': "A, R",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Arc", "Arc"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Arc", "Creates a circular arc by a center point and a radius.\nCTRL to snap, SHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name=self.featureName)
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.angle = 0  # angle inscribed by arc
            self.tangents = []
            self.tanpoints = []
            if self.featureName == "Arc":
                self.ui.arcUi()
            else:
                self.ui.circleUi()
            self.altdown = False
            self.ui.sourceCmd = self
            self.linetrack = trackers.lineTracker(dotted=True)
            self.arctrack = trackers.arcTracker()
            self.call = self.view.addEventCallback("SoEvent", self.action)
            _msg(translate("draft", "Pick center point"))

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        super().finish()
        if self.ui:
            self.linetrack.finalize()
            self.arctrack.finalize()
            self.doc.recompute()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()

    def updateAngle(self, angle):
        """Update the angle with the new value."""
        # previous absolute angle
        lastangle = self.firstangle + self.angle
        if lastangle <= -2 * math.pi:
            lastangle += 2 * math.pi
        if lastangle >= 2 * math.pi:
            lastangle -= 2 * math.pi
        # compute delta = change in angle:
        d0 = angle - lastangle
        d1 = d0 + 2 * math.pi
        d2 = d0 - 2 * math.pi
        if abs(d0) < min(abs(d1), abs(d2)):
            delta = d0
        elif abs(d1) < abs(d2):
            delta = d1
        else:
            delta = d2
        newangle = self.angle + delta
        # normalize angle, preserving direction
        if newangle >= 2 * math.pi:
            newangle -= 2 * math.pi
        if newangle <= -2 * math.pi:
            newangle += 2 * math.pi
        self.angle = newangle

    def action(self, arg):
        """Handle the 3D scene events.

        This is installed as an EventCallback in the Inventor view.

        Parameters
        ----------
        arg: dict
            Dictionary with strings that indicates the type of event received
            from the 3D view.
        """
        import DraftGeomUtils

        if arg["Type"] == "SoKeyboardEvent":
            if arg["Key"] == "ESCAPE":
                self.finish()
        elif arg["Type"] == "SoLocation2Event":  # mouse movement detection
            self.point, ctrlPoint, info = gui_tool_utils.getPoint(self, arg)
            # this is to make sure radius is what you see on screen
            if self.center and DraftVecUtils.dist(self.point, self.center) > 0:
                viewdelta = DraftVecUtils.project(self.point.sub(self.center),
                                                  self.wp.axis)
                if not DraftVecUtils.isNull(viewdelta):
                    self.point = self.point.add(viewdelta.negative())
            if self.step == 0:  # choose center
                if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                    if not self.altdown:
                        self.altdown = True
                        self.ui.switchUi(True)
                    else:
                        if self.altdown:
                            self.altdown = False
                            self.ui.switchUi(False)
            elif self.step == 1:  # choose radius
                if len(self.tangents) == 2:
                    cir = DraftGeomUtils.circleFrom2tan1pt(self.tangents[0],
                                                           self.tangents[1],
                                                           self.point)
                    _c = DraftGeomUtils.findClosestCircle(self.point, cir)
                    self.center = _c.Center
                    self.arctrack.setCenter(self.center)
                elif self.tangents and self.tanpoints:
                    cir = DraftGeomUtils.circleFrom1tan2pt(self.tangents[0],
                                                           self.tanpoints[0],
                                                           self.point)
                    _c = DraftGeomUtils.findClosestCircle(self.point, cir)
                    self.center = _c.Center
                    self.arctrack.setCenter(self.center)
                if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                    if not self.altdown:
                        self.altdown = True
                    if info:
                        ob = self.doc.getObject(info['Object'])
                        num = int(info['Component'].lstrip('Edge')) - 1
                        ed = ob.Shape.Edges[num]
                        if len(self.tangents) == 2:
                            cir = DraftGeomUtils.circleFrom3tan(self.tangents[0],
                                                                self.tangents[1],
                                                                ed)
                            cl = DraftGeomUtils.findClosestCircle(self.point, cir)
                            self.center = cl.Center
                            self.rad = cl.Radius
                            self.arctrack.setCenter(self.center)
                        else:
                            self.rad = self.center.add(DraftGeomUtils.findDistance(self.center, ed).sub(self.center)).Length
                    else:
                        self.rad = DraftVecUtils.dist(self.point, self.center)
                else:
                    if self.altdown:
                        self.altdown = False
                    self.rad = DraftVecUtils.dist(self.point, self.center)
                self.ui.setRadiusValue(self.rad, "Length")
                self.arctrack.setRadius(self.rad)
                self.linetrack.p1(self.center)
                self.linetrack.p2(self.point)
                self.linetrack.on()
            elif (self.step == 2):  # choose first angle
                currentrad = DraftVecUtils.dist(self.point, self.center)
                if currentrad != 0:
                    angle = DraftVecUtils.angle(self.wp.u, self.point.sub(self.center), self.wp.axis)
                else:
                    angle = 0
                self.linetrack.p2(DraftVecUtils.scaleTo(self.point.sub(self.center), self.rad).add(self.center))
                self.ui.setRadiusValue(math.degrees(angle), unit="Angle")
                self.firstangle = angle
            else:
                # choose second angle
                currentrad = DraftVecUtils.dist(self.point, self.center)
                if currentrad != 0:
                    angle = DraftVecUtils.angle(self.wp.u, self.point.sub(self.center), self.wp.axis)
                else:
                    angle = 0
                self.linetrack.p2(DraftVecUtils.scaleTo(self.point.sub(self.center), self.rad).add(self.center))
                self.updateAngle(angle)
                self.ui.setRadiusValue(math.degrees(self.angle), unit="Angle")
                self.arctrack.setApertureAngle(self.angle)

            gui_tool_utils.redraw3DView()

        elif arg["Type"] == "SoMouseButtonEvent":  # mouse click
            if arg["State"] == "DOWN" and arg["Button"] == "BUTTON1":
                if self.point:
                    if self.step == 0:  # choose center
                        if not self.support:
                            gui_tool_utils.getSupport(arg)
                            (self.point,
                             ctrlPoint, info) = gui_tool_utils.getPoint(self, arg)
                        if gui_tool_utils.hasMod(arg, gui_tool_utils.MODALT):
                            snapped = self.view.getObjectInfo((arg["Position"][0],
                                                               arg["Position"][1]))
                            if snapped:
                                ob = self.doc.getObject(snapped['Object'])
                                num = int(snapped['Component'].lstrip('Edge')) - 1
                                ed = ob.Shape.Edges[num]
                                self.tangents.append(ed)
                                if len(self.tangents) == 2:
                                    self.arctrack.on()
                                    self.ui.radiusUi()
                                    self.step = 1
                                    self.ui.setNextFocus()
                                    self.linetrack.on()
                                    _msg(translate("draft", "Pick radius"))
                        else:
                            if len(self.tangents) == 1:
                                self.tanpoints.append(self.point)
                            else:
                                self.center = self.point
                                self.node = [self.point]
                                self.arctrack.setCenter(self.center)
                                self.linetrack.p1(self.center)
                                self.linetrack.p2(self.view.getPoint(arg["Position"][0],
                                                                     arg["Position"][1]))
                            self.arctrack.on()
                            self.ui.radiusUi()
                            self.step = 1
                            self.ui.setNextFocus()
                            self.linetrack.on()
                            _msg(translate("draft", "Pick radius"))
                            if self.planetrack:
                                self.planetrack.set(self.point)
                    elif self.step == 1:  # choose radius
                        if self.closedCircle:
                            self.drawArc()
                        else:
                            self.ui.labelRadius.setText(translate("draft", "Start angle"))
                            self.ui.radiusValue.setToolTip(translate("draft", "Start angle"))
                            self.ui.radiusValue.setText(U.Quantity(0, U.Angle).UserString)
                            self.linetrack.p1(self.center)
                            self.linetrack.on()
                            self.step = 2
                            _msg(translate("draft", "Pick start angle"))
                    elif self.step == 2:  # choose first angle
                        self.ui.labelRadius.setText(translate("draft", "Aperture angle"))
                        self.ui.radiusValue.setToolTip(translate("draft", "Aperture angle"))
                        self.step = 3
                        # scale center->point vector for proper display
                        # u = DraftVecUtils.scaleTo(self.point.sub(self.center), self.rad) obsolete?
                        self.arctrack.setStartAngle(self.firstangle)
                        _msg(translate("draft", "Pick aperture"))
                    else:  # choose second angle
                        self.step = 4
                        self.drawArc()

    def drawArc(self):
        """Actually draw the arc object."""
        rot, sup, pts, fil = self.getStrings()
        if self.closedCircle:
            try:
                # The command to run is built as a series of text strings
                # to be committed through the `draftutils.todo.ToDo` class.
                Gui.addModule("Draft")
                if utils.getParam("UsePartPrimitives", False):
                    # Insert a Part::Primitive object
                    _base = DraftVecUtils.toString(self.center)
                    _cmd = 'FreeCAD.ActiveDocument.'
                    _cmd += 'addObject("Part::Circle", "Circle")'
                    _cmd_list = ['circle = ' + _cmd,
                                 'circle.Radius = ' + str(self.rad),
                                 'pl = FreeCAD.Placement()',
                                 'pl.Rotation.Q = ' + rot,
                                 'pl.Base = ' + _base,
                                 'circle.Placement = pl',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Create Circle (Part)"),
                                _cmd_list)
                else:
                    # Insert a Draft circle
                    _base = DraftVecUtils.toString(self.center)
                    _cmd = 'Draft.make_circle'
                    _cmd += '('
                    _cmd += 'radius=' + str(self.rad) + ', '
                    _cmd += 'placement=pl, '
                    _cmd += 'face=' + fil + ', '
                    _cmd += 'support=' + sup
                    _cmd += ')'
                    _cmd_list = ['pl=FreeCAD.Placement()',
                                 'pl.Rotation.Q=' + rot,
                                 'pl.Base=' + _base,
                                 'circle = ' + _cmd,
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Create Circle"),
                                _cmd_list)
            except Exception:
                _err("Draft: error delaying commit")
        else:
            # Not a closed circle, therefore a circular arc
            sta = math.degrees(self.firstangle)
            end = math.degrees(self.firstangle + self.angle)
            if end < sta:
                sta, end = end, sta
            sta %= 360
            end %= 360

            try:
                Gui.addModule("Draft")
                if utils.getParam("UsePartPrimitives", False):
                    # Insert a Part::Primitive object
                    _base = DraftVecUtils.toString(self.center)
                    _cmd = 'FreeCAD.ActiveDocument.'
                    _cmd += 'addObject("Part::Circle", "Circle")'
                    _cmd_list = ['circle = ' + _cmd,
                                 'circle.Radius = ' + str(self.rad),
                                 'circle.Angle1 = ' + str(sta),
                                 'circle.Angle2 = ' + str(end),
                                 'pl = FreeCAD.Placement()',
                                 'pl.Rotation.Q = ' + rot,
                                 'pl.Base = ' + _base,
                                 'circle.Placement = pl',
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Create Arc (Part)"),
                                _cmd_list)
                else:
                    # Insert a Draft circle
                    _base = DraftVecUtils.toString(self.center)
                    _cmd = 'Draft.make_circle'
                    _cmd += '('
                    _cmd += 'radius=' + str(self.rad) + ', '
                    _cmd += 'placement=pl, '
                    _cmd += 'face=' + fil + ', '
                    _cmd += 'startangle=' + str(sta) + ', '
                    _cmd += 'endangle=' + str(end) + ', '
                    _cmd += 'support=' + sup
                    _cmd += ')'
                    _cmd_list = ['pl = FreeCAD.Placement()',
                                 'pl.Rotation.Q = ' + rot,
                                 'pl.Base = ' + _base,
                                 'circle = ' + _cmd,
                                 'Draft.autogroup(circle)',
                                 'FreeCAD.ActiveDocument.recompute()']
                    self.commit(translate("draft", "Create Arc"),
                                _cmd_list)
            except Exception:
                _err("Draft: error delaying commit")

        # Finalize full circle or circular arc
        self.finish(cont=None)

    def numericInput(self, numx, numy, numz):
        """Validate the entry fields in the user interface.

        This function is called by the toolbar or taskpanel interface
        when valid x, y, and z have been entered in the input fields.
        """
        self.center = App.Vector(numx, numy, numz)
        self.node = [self.center]
        self.arctrack.setCenter(self.center)
        self.arctrack.on()
        self.ui.radiusUi()
        self.step = 1
        self.ui.setNextFocus()
        _msg(translate("draft", "Pick radius"))

    def numericRadius(self, rad):
        """Validate the entry radius in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        import DraftGeomUtils

        if self.step == 1:
            self.rad = rad
            if len(self.tangents) == 2:
                cir = DraftGeomUtils.circleFrom2tan1rad(self.tangents[0],
                                                        self.tangents[1],
                                                        rad)
                if self.center:
                    _c = DraftGeomUtils.findClosestCircle(self.center, cir)
                    self.center = _c.Center
                else:
                    self.center = cir[-1].Center
            elif self.tangents and self.tanpoints:
                cir = DraftGeomUtils.circleFrom1tan1pt1rad(self.tangents[0],
                                                           self.tanpoints[0],
                                                           rad)
                if self.center:
                    _c = DraftGeomUtils.findClosestCircle(self.center, cir)
                    self.center = _c.Center
                else:
                    self.center = cir[-1].Center
            if self.closedCircle:
                self.drawArc()
            else:
                self.step = 2
                self.arctrack.setCenter(self.center)
                self.ui.labelRadius.setText(translate("draft", "Start angle"))
                self.ui.radiusValue.setToolTip(translate("draft", "Start angle"))
                self.linetrack.p1(self.center)
                self.linetrack.on()
                self.ui.radiusValue.setText("")
                self.ui.radiusValue.setFocus()
                _msg(translate("draft", "Pick start angle"))
        elif self.step == 2:
            self.ui.labelRadius.setText(translate("draft", "Aperture angle"))
            self.ui.radiusValue.setToolTip(translate("draft", "Aperture angle"))
            self.firstangle = math.radians(rad)
            if DraftVecUtils.equals(self.wp.axis, App.Vector(1, 0, 0)):
                u = App.Vector(0, self.rad, 0)
            else:
                u = DraftVecUtils.scaleTo(App.Vector(1, 0, 0).cross(self.wp.axis), self.rad)
            urotated = DraftVecUtils.rotate(u, math.radians(rad), self.wp.axis)
            self.arctrack.setStartAngle(self.firstangle)
            self.step = 3
            self.ui.radiusValue.setText("")
            self.ui.radiusValue.setFocus()
            _msg(translate("draft", "Pick aperture angle"))
        else:
            self.updateAngle(rad)
            self.angle = math.radians(rad)
            self.step = 4
            self.drawArc()


Gui.addCommand('Draft_Arc', Arc())


class Arc_3Points(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Arc_3Points tool."""

    def __init__(self):
        super().__init__(name="Arc by 3 points")

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'Pixmap': "Draft_Arc_3Points",
                'Accel': "A,T",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Arc_3Points", "Arc by 3 points"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Arc_3Points", "Creates a circular arc by picking 3 points.\nCTRL to snap, SHIFT to constrain.")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated()

        # Reset the values
        self.points = []
        self.normal = None
        self.tracker = trackers.arcTracker()
        self.tracker.autoinvert = False

        # Set up the working plane and launch the Snapper
        # with the indicated callbacks: one for when the user clicks
        # on the 3D view, and another for when the user moves the pointer.
        if hasattr(App, "DraftWorkingPlane"):
            App.DraftWorkingPlane.setup()

        Gui.Snapper.getPoint(callback=self.getPoint,
                             movecallback=self.drawArc)
        Gui.Snapper.ui.sourceCmd = self
        Gui.Snapper.ui.setTitle(title=translate("draft", "Arc by 3 points"),
                                icon="Draft_Arc_3Points")
        Gui.Snapper.ui.continueCmd.show()

    def getPoint(self, point, info):
        """Get the point by clicking on the 3D view.

        Every time the user clicks on the 3D view this method is run.
        In this case, a point is appended to the list of points,
        and the tracker is updated.
        The object is finally created when three points are picked.

        Parameters
        ----------
        point: Base::Vector
            The point selected in the 3D view.

        info: str
            Some information obtained about the point passed by the Snapper.
        """
        # If there is not point, the command was cancelled
        # so the command exits.
        if not point:
            return None

        # Avoid adding the same point twice
        if point not in self.points:
            self.points.append(point)

        if len(self.points) < 3:
            # If one or two points were picked, set up again the Snapper
            # to get further points, but update the `last` property
            # with the last selected point.
            #
            # When two points are selected then we can turn on
            # the arc tracker to show the preview of the final curve.
            if len(self.points) == 2:
                self.tracker.on()
            Gui.Snapper.getPoint(last=self.points[-1],
                                 callback=self.getPoint,
                                 movecallback=self.drawArc)
            Gui.Snapper.ui.sourceCmd = self
            Gui.Snapper.ui.setTitle(title=translate("draft", "Arc by 3 points"),
                                    icon="Draft_Arc_3Points")
            Gui.Snapper.ui.continueCmd.show()

        else:
            # If three points were already picked in the 3D view
            # proceed with creating the final object.
            # Draw a simple `Part::Feature` if the parameter is `True`.
            if utils.get_param("UsePartPrimitives", False):
                Draft.make_arc_3points([self.points[0],
                                        self.points[1],
                                        self.points[2]], primitive=True)
            else:
                Draft.make_arc_3points([self.points[0],
                                        self.points[1],
                                        self.points[2]], primitive=False)

            self.finish(cont=None)

    def drawArc(self, point, info):
        """Draw preview arc when we move the pointer in the 3D view.

        It uses the `gui_trackers.arcTracker.setBy3Points` method.

        Parameters
        ----------
        point: Base::Vector
            The dynamic point passed by the callback
            as we move the pointer in the 3D view.

        info: str
            Some information obtained from the point by the Snapper.
        """
        if len(self.points) == 2:
            if point.sub(self.points[1]).Length > 0.001:
                self.tracker.setBy3Points(self.points[0],
                                          self.points[1],
                                          point)

    def finish(self, cont=False):
        """Terminate the operation.

        Parameters
        ----------
        cont: bool or None, optional
            Restart (continue) the command if `True`, or if `None` and
            `ui.continueMode` is `True`.
        """
        self.tracker.finalize()
        self.doc.recompute()
        if cont or (cont is None and Gui.Snapper.ui and Gui.Snapper.ui.continueMode):
            self.Activated()


Draft_Arc_3Points = Arc_3Points
Gui.addCommand('Draft_Arc_3Points', Arc_3Points())


class ArcGroup:
    """Gui Command group for the Arc tools."""

    def GetResources(self):
        """Set icon, menu and tooltip."""
        return {'MenuText': QT_TRANSLATE_NOOP("Draft_ArcTools", "Arc tools"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_ArcTools", "Create various types of circular arcs.")}

    def GetCommands(self):
        """Return a tuple of commands in the group."""
        return ('Draft_Arc', 'Draft_Arc_3Points')

    def IsActive(self):
        """Return True when this command should be available.

        It is `True` when there is a document.
        """
        if Gui.ActiveDocument:
            return True
        else:
            return False


Gui.addCommand('Draft_ArcTools', ArcGroup())

## @}
