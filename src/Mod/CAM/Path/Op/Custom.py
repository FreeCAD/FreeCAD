# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
import os
import Path
import Path.Op.Base as PathOp

from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Custom Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "CAM Custom object and FreeCAD command"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ObjectCustom(PathOp.ObjectOp):
    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """customOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties

        enums = {
            "Source": [
                (translate("PathCustom", "Text"), "Text"),
                (translate("PathCustom", "File"), "File"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def opFeatures(self, obj):
        return PathOp.FeatureTool | PathOp.FeatureCoolant

    def initOperation(self, obj):
        obj.addProperty(
            "App::PropertyEnumeration",
            "Source",
            "Path",
            "Source of gcode (text, file, ...)",
        )

        obj.addProperty(
            "App::PropertyFile",
            "GcodeFile",
            "Path",
            "File containing gcode to be inserted",
        )

        obj.addProperty(
            "App::PropertyStringList",
            "Gcode",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The G-code to be inserted"),
        )

        # populate the property enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        obj.Proxy = self
        self.setEditorModes(obj)

    def onChanged(self, obj, prop):
        if prop == "Source":
            self.setEditorModes(obj)

    def opOnDocumentRestored(self, obj):
        if not hasattr(obj, "Source"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "Source",
                "Path",
                "Source of gcode (text, file, ...)",
            )

        if not hasattr(obj, "GcodeFile"):
            obj.addProperty(
                "App::PropertyFile",
                "GcodeFile",
                "Path",
                "File containing gcode to be inserted",
            )

        # populate the property enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def onDocumentRestore(self, obj):
        self.setEditorModes(self, obj)

    def setEditorModes(self, obj, features=None):
        if not hasattr(obj, "Source"):
            return

        if obj.Source == "Text":
            obj.setEditorMode("GcodeFile", 2)
            obj.setEditorMode("Gcode", 0)
        elif obj.Source == "File":
            obj.setEditorMode("GcodeFile", 0)
            obj.setEditorMode("Gcode", 2)

    def findGcodeFile(self, filename):
        if os.path.exists(filename):
            # probably absolute, just return
            return filename

        doc_path = os.path.dirname(FreeCAD.ActiveDocument.FileName)
        prospective_path = os.path.join(doc_path, filename)

        if os.path.exists(prospective_path):
            return prospective_path

    def opExecute(self, obj):
        self.commandlist.append(Path.Command("(Begin Custom)"))

        if obj.Source == "Text" and obj.Gcode:
            for l in obj.Gcode:
                newcommand = Path.Command(str(l))
                self.commandlist.append(newcommand)
        elif obj.Source == "File" and len(obj.GcodeFile) > 0:
            gcode_file = self.findGcodeFile(obj.GcodeFile)

            # could not determine the path
            if not gcode_file:
                Path.Log.error(
                    translate("PathCustom", "Custom file %s could not be found.")
                    % obj.GcodeFile
                )

            with open(gcode_file) as fd:
                for l in fd.readlines():
                    try:
                        newcommand = Path.Command(str(l))
                        self.commandlist.append(newcommand)
                    except ValueError:
                        Path.Log.warning(
                            translate("PathCustom", "Invalid Gcode line: %s") % l
                        )
                        continue

        self.commandlist.append(Path.Command("(End Custom)"))


def SetupProperties():
    setup = []
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Custom operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectCustom(obj, name, parentJob)
    return obj
