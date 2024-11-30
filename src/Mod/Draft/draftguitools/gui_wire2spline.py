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
"""Provides GUI tools to convert polylines to B-splines and back.

These tools work on polylines and B-splines which have multiple points.

Essentially, the points of the original object are extracted
and passed to the `make_wire` or `make_bspline` functions,
depending on the desired result.
"""
## @package gui_wire2spline
# \ingroup draftguitools
# \brief Provides GUI tools to convert polylines to B-splines and back.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCADGui as Gui
import Draft_rc
import Draft
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original

from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class WireToBSpline(gui_base_original.Modifier):
    """Gui Command for the Wire to BSpline tool."""

    def __init__(self):
        super().__init__()
        self.running = False

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_WireToBSpline',
                'MenuText': QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Wire to B-spline"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_WireToBSpline", "Converts a selected polyline to a B-spline, or a B-spline to a polyline.")}

    def Activated(self):
        """Execute when the command is called."""
        if self.running:
            self.finish()

        # TODO: iterate over all selected items to transform
        # many objects. As it is right now, it only works on the first object
        # in the selection.
        # Also, it is recommended to use the `self.commit` function
        # in order to properly open a transaction and commit it.
        selection = Gui.Selection.getSelection()
        if selection:
            if utils.getType(selection[0]) in ['Wire', 'BSpline']:
                super(WireToBSpline, self).Activated(name="Convert polyline/B-spline")
                if self.doc:
                    self.obj = Gui.Selection.getSelection()
                    if self.obj:
                        self.obj = self.obj[0]
                        self.pl = None
                        if "Placement" in self.obj.PropertiesList:
                            self.pl = self.obj.Placement
                        self.Points = self.obj.Points
                        self.closed = self.obj.Closed
                        n = None
                        if utils.getType(self.obj) == 'Wire':
                            n = Draft.make_bspline(self.Points,
                                                   closed=self.closed,
                                                   placement=self.pl)
                        elif utils.getType(self.obj) == 'BSpline':
                            self.bs2wire = True
                            n = Draft.make_wire(self.Points,
                                                closed=self.closed,
                                                placement=self.pl,
                                                face=None,
                                                support=None,
                                                bs2wire=self.bs2wire)
                        if n:
                            Draft.formatObject(n, self.obj)
                            self.doc.recompute()
                    else:
                        self.finish()


Gui.addCommand('Draft_WireToBSpline', WireToBSpline())

## @}
