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
"""Provides tools for creating arcs with the Draft Workbench."""
## @package gui_arcs
# \ingroup DRAFT
# \brief Provides tools for creating arcs with the Draft Workbench.
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import draftobjects.arc_3points as arc3
import draftguitools.gui_base as gui_base
import draftguitools.gui_trackers as trackers
import draftutils.utils as utils
from draftutils.translate import _tr

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Arc_3Points(gui_base.GuiCommandSimplest):
    """GuiCommand for the Draft_Arc_3Points tool."""

    def __init__(self):
        super().__init__(name=_tr("Arc by 3 points"))

    def GetResources(self):
        """Set icon, menu and tooltip."""
        _menu = "Arc by 3 points"
        _tip = ("Creates a circular arc by picking 3 points.\n"
                "CTRL to snap, SHIFT to constrain.")

        d = {'Pixmap': "Draft_Arc_3Points",
             'MenuText': QT_TRANSLATE_NOOP("Draft_Arc_3Points", _menu),
             'ToolTip': QT_TRANSLATE_NOOP("Draft_Arc_3Points", _tip),
             'Accel': 'A,T'}
        return d

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
            self.tracker.off()
            return

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
        else:
            # If three points were already picked in the 3D view
            # proceed with creating the final object.
            # Draw a simple `Part::Feature` if the parameter is `True`.
            if utils.get_param("UsePartPrimitives", False):
                arc3.make_arc_3points([self.points[0],
                                       self.points[1],
                                       self.points[2]], primitive=True)
            else:
                arc3.make_arc_3points([self.points[0],
                                       self.points[1],
                                       self.points[2]], primitive=False)
            self.tracker.off()
            self.doc.recompute()

    def drawArc(self, point, info):
        """Draw preview arc when we move the pointer in the 3D view.

        It uses the `gui_trackers.arcTracker.setBy3Points` method.

        Parameters
        ----------
        point: Base::Vector
            The dynamic point pased by the callback
            as we move the pointer in the 3D view.

        info: str
            Some information obtained from the point by the Snapper.
        """
        if len(self.points) == 2:
            if point.sub(self.points[1]).Length > 0.001:
                self.tracker.setBy3Points(self.points[0],
                                          self.points[1],
                                          point)


Draft_Arc_3Points = Arc_3Points
Gui.addCommand('Draft_Arc_3Points', Arc_3Points())
