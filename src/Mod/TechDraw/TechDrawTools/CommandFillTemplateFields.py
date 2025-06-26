# ***************************************************************************
# *   Copyright (c) 2023 Syres                                              *
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
"""Provides the TechDraw FillTemplateFields GuiCommand."""

__title__ = "TechDrawTools.CommandFillTemplateFields"
__author__ = "Syres"
__url__ = "https://www.freecad.org"
__version__ = "00.02"
__date__ = "2023/12/07"

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import FreeCADGui as Gui
import codecs
import csv
import os.path

import TechDrawTools


class CommandFillTemplateFields:
    """Use document info to populate the template fields."""

    def GetResources(self):
        """Return a dictionary with data that will be used by the button or menu item."""
        return {
            "Pixmap": "actions/TechDraw_FillTemplateFields.svg",
            "Accel": "",
            "MenuText": QT_TRANSLATE_NOOP(
                "TechDraw_FillTemplateFields", "Update Template Fields"
            ),
            "ToolTip": QT_TRANSLATE_NOOP(
                "TechDraw_FillTemplateFields",
                "Uses document info to populate the template fields",
            ),
        }

    def Activated(self):
        """Run the following code when the command is activated (button press)."""
        TechDrawTools.TaskFillTemplateFields()

    def IsActive(self):
        """Return True when the command should be active
        or False when it should be disabled (greyed)."""
        if App.ActiveDocument:
            objs = App.ActiveDocument.Objects
            for obj in objs:
                if obj.TypeId == "TechDraw::DrawPage":
                    file_path = (
                        App.getResourceDir()
                        + "Mod/TechDraw/CSVdata/FillTemplateFields.csv"
                    )
                    if os.path.exists(file_path):
                        listofkeys = [
                            "CreatedByChkLst",
                            "ScaleChkLst",
                            "LabelChkLst",
                            "CommentChkLst",
                            "CompanyChkLst",
                            "LicenseChkLst",
                            "CreatedDateChkLst",
                            "LastModifiedDateChkLst",
                        ]
                        with codecs.open(file_path, encoding="utf-8") as fp:
                            reader = csv.DictReader(fp)
                            page = obj
                            texts = page.Template.EditableTexts
                            if (
                                texts
                                and os.path.exists(file_path)
                                and listofkeys == reader.fieldnames
                                and obj.Views != []
                            ):
                                return True
        else:
            return False


#
# The command must be "registered" with a unique name by calling its class.
Gui.addCommand("TechDraw_FillTemplateFields", CommandFillTemplateFields())

