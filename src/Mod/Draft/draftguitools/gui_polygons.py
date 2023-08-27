# ***************************************************************************
# *   (c) 2009 Yorik van Havre <yorik@uncreated.net>                        *
# *   (c) 2010 Ken Cline <cline@frii.com>                                   *
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
"""Provides GUI tools to create regular Polygon objects.

Minimum number of sides is three (equilateral triangles).
"""
## @package gui_polygons
# \ingroup draftguitools
# \brief Provides GUI tools to create regular Polygon objects.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
import draftguitools.gui_trackers as trackers

from draftutils.messages import _msg
from draftutils.translate import translate


class Polygon(gui_base_original.Creator):
    """Gui command for the Polygon tool."""

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Polygon',
                'Accel': "P, G",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Polygon", "Polygon"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Polygon", "Creates a regular polygon (triangle, square, pentagon, ...), by defining the number of sides and the circumscribed radius.\nCTRL to snap, SHIFT to constrain")}

    def Activated(self):
        """Execute when the command is called."""
        super().Activated(name="Polygon")
        if self.ui:
            self.step = 0
            self.center = None
            self.rad = None
            self.tangents = []
            self.tanpoints = []
            self.ui.pointUi(title=translate("draft", "Polygon"), icon="Draft_Polygon")
            self.ui.extUi()
            self.ui.isRelative.hide()
            self.ui.numFaces.show()
            self.ui.numFacesLabel.show()
            self.altdown = False
            self.ui.sourceCmd = self
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
        super().finish(self)
        if self.ui:
            self.arctrack.finalize()
            self.doc.recompute()
        if cont or (cont is None and self.ui and self.ui.continueMode):
            self.Activated()

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
            else:  # choose radius
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
                    snapped = self.view.getObjectInfo((arg["Position"][0],
                                                       arg["Position"][1]))
                    if snapped:
                        ob = self.doc.getObject(snapped['Object'])
                        num = int(snapped['Component'].lstrip('Edge')) - 1
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
                            self.rad = self.center.add(DraftGeomUtils.findDistance(self.center,ed).sub(self.center)).Length
                    else:
                        self.rad = DraftVecUtils.dist(self.point, self.center)
                else:
                    if self.altdown:
                        self.altdown = False
                    self.rad = DraftVecUtils.dist(self.point, self.center)
                self.ui.setRadiusValue(self.rad, 'Length')
                self.arctrack.setRadius(self.rad)

            gui_tool_utils.redraw3DView()

        elif (arg["Type"] == "SoMouseButtonEvent"
              and arg["State"] == "DOWN"
              and arg["Button"] == "BUTTON1"):  # mouse click
            if self.point:
                if self.step == 0:  # choose center
                    if (not self.node) and (not self.support):
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
                                _msg(translate("draft", "Pick radius"))
                    else:
                        if len(self.tangents) == 1:
                            self.tanpoints.append(self.point)
                        else:
                            self.center = self.point
                            self.node = [self.point]
                            self.arctrack.setCenter(self.center)
                        self.arctrack.on()
                        self.ui.radiusUi()
                        self.step = 1
                        _msg(translate("draft", "Pick radius"))
                        if self.planetrack:
                            self.planetrack.set(self.point)
                elif self.step == 1:  # choose radius
                    self.drawPolygon()

    def drawPolygon(self):
        """Draw the actual object."""
        rot, sup, pts, fil = self.getStrings()
        Gui.addModule("Draft")
        if utils.getParam("UsePartPrimitives", False):
            # Insert a Part::Primitive object
            Gui.addModule("Part")
            _cmd = 'FreeCAD.ActiveDocument.'
            _cmd += 'addObject("Part::RegularPolygon","RegularPolygon")'
            _cmd_list = ['pl = FreeCAD.Placement()',
                         'pl.Rotation.Q = ' + rot,
                         'pl.Base = ' + DraftVecUtils.toString(self.center),
                         'pol = ' + _cmd,
                         'pol.Polygon = ' + str(self.ui.numFaces.value()),
                         'pol.Circumradius = ' + str(self.rad),
                         'pol.Placement = pl',
                         'Draft.autogroup(pol)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Polygon (Part)"),
                        _cmd_list)
        else:
            # Insert a Draft polygon
            _cmd = 'Draft.make_polygon'
            _cmd += '('
            _cmd += str(self.ui.numFaces.value()) + ', '
            _cmd += 'radius=' + str(self.rad) + ', '
            _cmd += 'inscribed=True, '
            _cmd += 'placement=pl, '
            _cmd += 'face=' + fil + ', '
            _cmd += 'support=' + sup
            _cmd += ')'
            _cmd_list = ['pl = FreeCAD.Placement()',
                         'pl.Rotation.Q = ' + rot,
                         'pl.Base = ' + DraftVecUtils.toString(self.center),
                         'pol = ' + _cmd,
                         'Draft.autogroup(pol)',
                         'FreeCAD.ActiveDocument.recompute()']
            self.commit(translate("draft", "Create Polygon"),
                        _cmd_list)
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
        self.ui.radiusValue.setFocus()
        _msg(translate("draft", "Pick radius"))

    def numericRadius(self, rad):
        """Validate the entry radius in the user interface.

        This function is called by the toolbar or taskpanel interface
        when a valid radius has been entered in the input field.
        """
        import DraftGeomUtils

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
        self.drawPolygon()


Gui.addCommand('Draft_Polygon', Polygon())

## @}
