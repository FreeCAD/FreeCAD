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
"""Provides GUI tools to project an object into a Drawing Workbench page.

This commands takes a 2D geometrical element and creates a projection
that is displayed in a drawing page in the Drawing Workbench.

This command should be considered obsolete as the Drawing Workbench
is obsolete since 0.17.

A similar command is not planned for the TechDraw Workbench because
it is not really necessary. TechDraw has its own set of tools
to create 2D projections of 2D and 3D objects.
"""
## @package gui_drawing
# \ingroup draftguitools
# \brief Provides GUI tools to project an object into a Drawing Workbench page.

## \addtogroup draftguitools
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import Draft_rc
import Draft
import draftutils.utils as utils
import draftguitools.gui_base_original as gui_base_original
import draftguitools.gui_tool_utils as gui_tool_utils
from draftutils.messages import _msg, _wrn
from draftutils.translate import translate

# The module is used to prevent complaints from code checkers (flake8)
True if Draft_rc.__name__ else False


class Drawing(gui_base_original.Modifier):
    """Gui Command for the Drawing tool.

    This command should be considered obsolete as the Drawing Workbench
    is obsolete since 0.17.
    """

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {'Pixmap': 'Draft_Drawing',
                # 'Accel': "D, D",
                'MenuText': QT_TRANSLATE_NOOP("Draft_Drawing", "Drawing"),
                'ToolTip': QT_TRANSLATE_NOOP("Draft_Drawing", "Creates a 2D projection on a Drawing Workbench page from the selected objects.\nThis command is OBSOLETE since the Drawing Workbench became obsolete in 0.17.\nUse TechDraw Workbench instead for generating technical drawings.")}

    def Activated(self):
        """Execute when the command is called."""
        super(Drawing, self).Activated(name="Drawing")
        _wrn(translate("draft","The Drawing Workbench is obsolete since 0.17, consider using the TechDraw Workbench instead."))
        if not Gui.Selection.getSelection():
            self.ghost = None
            self.ui.selectUi()
            _msg(translate("draft", "Select an object to project"))
            self.call = \
                self.view.addEventCallback("SoEvent",
                                           gui_tool_utils.selectObject)
        else:
            self.proceed()

    def proceed(self):
        """Proceed with execution of the command after proper selection."""
        if self.call:
            self.view.removeEventCallback("SoEvent", self.call)

        sel = Gui.Selection.getSelection()
        if not sel:
            self.page = self.createDefaultPage()
        else:
            self.page = None
            # if the user selected a page, put the objects on that page
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeaturePage"):
                    self.page = obj
                    break
            if not self.page:
                # no page selected, default to the first page in the document
                for obj in self.doc.Objects:
                    if obj.isDerivedFrom("Drawing::FeaturePage"):
                        self.page = obj
                        break
            if not self.page:
                # no page in the document, create a default page.
                self.page = self.createDefaultPage()
            otherProjection = None
            # if an existing projection is selected,
            # reuse its projection properties
            for obj in sel:
                if obj.isDerivedFrom("Drawing::FeatureView"):
                    otherProjection = obj
                    break
            sel.reverse()
            for obj in sel:
                if (obj.ViewObject.isVisible()
                        and not obj.isDerivedFrom("Drawing::FeatureView")
                        and not obj.isDerivedFrom("Drawing::FeaturePage")):
                    # name = 'View' + obj.Name
                    # no reason to remove the old one...
                    # oldobj = self.page.getObject(name)
                    # if oldobj:
                    #     self.doc.removeObject(oldobj.Name)
                    Draft.make_drawing_view(obj, self.page,
                                            otherProjection=otherProjection)
            self.doc.recompute()

    def createDefaultPage(self):
        """Create a default Drawing Workbench page."""
        _t = App.getResourceDir() + 'Mod/Drawing/Templates/A3_Landscape.svg'
        template = utils.getParam("template", _t)
        page = self.doc.addObject('Drawing::FeaturePage', 'Page')
        page.ViewObject.HintOffsetX = 200
        page.ViewObject.HintOffsetY = 100
        page.ViewObject.HintScale = 2
        page.Template = template
        self.doc.recompute()
        return page


Gui.addCommand('Draft_Drawing', Drawing())

## @}
