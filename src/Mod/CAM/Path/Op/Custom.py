# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2014 Yorik van Havre yorik@uncreated.net
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import re
import os
import Path
import Path.Op.Base as PathOp
import Constants
from PathScripts import PathUtils

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

        data = []
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

        obj.addProperty(
            "App::PropertyBool",
            "PostProcessOutput",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Pass Custom G-code through Post Processor"),
        )
        obj.PostProcessOutput = True

        # populate the property enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        obj.Proxy = self
        self.setEditorModes(obj)

    def onChanged(self, obj, prop):
        if prop == "Source":
            self.setEditorModes(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

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
        if not hasattr(obj, "PostProcessOutput"):
            obj.addProperty(
                "App::PropertyBool",
                "PostProcessOutput",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Pass Custom G-code through Post Processor"),
            )
            obj.PostProcessOutput = True
        # populate the property enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

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

    def parseExpressions(self, obj, line, index):
        pattern = r"\{\{(.+?)\}\}"
        while match := re.search(pattern, line):
            expr = match.group(1)
            try:
                value = obj.evalExpression(expr)
            except Exception:
                Path.Log.warning(
                    translate("PathCustom", "Can not parse expression from line %s: %s")
                    % (index, line)
                )
                obj.Path = Path.Path()
                raise Exception("Can not parse expression!")
            line = re.sub(pattern, str(value), line, count=1)
        return line

    def processLines(self, obj, lines):
        for i, line in enumerate(lines, 1):
            if line.startswith("!"):
                newcommand = Path.Command("", {}, {Constants.ANNOT_AS_IS: line[1:]})
                self.commandlist.append(newcommand)
            elif not obj.PostProcessOutput:
                newcommand = Path.Command("", {}, {Constants.ANNOT_AS_IS: line})
                self.commandlist.append(newcommand)
            else:
                line = line.strip()
                line = self.parseExpressions(obj, line, i)
                try:
                    # Custom is strict: supported+non-conforming, or mark "as-is"
                    newcommand = Path.Command(line, {})
                    self.commandlist.append(newcommand)
                except ValueError:
                    if len(self.errors) < 7:
                        self.errors.append((i, line))
                    else:
                        self.errors.append((i, None))

    def opExecute(self, obj):
        self.errors = []
        self.commandlist.append(Path.Command("(Begin Custom)"))

        if not obj.PostProcessOutput and (job := PathUtils.findParentJob(obj)) and not job.Machine:
            Path.Log.warning(
                translate(
                    "PathCustom",
                    "Pass Custom G-code through Post Processor"
                    " should be enabled for legacy post processor",
                )
            )
        if obj.Source == "Text" and obj.Gcode:
            self.processLines(obj, obj.Gcode)
        elif obj.Source == "File" and obj.GcodeFile:
            gcode_file = self.findGcodeFile(obj.GcodeFile)
            if not gcode_file:  # could not determine the path
                Path.Log.error(
                    translate("PathCustom", "Custom file %s could not be found.") % obj.GcodeFile
                )
                obj.Path = Path.Path()
                return
            with open(gcode_file) as fd:
                self.processLines(obj, fd.readlines())

        if self.errors:
            Path.Log.warning(
                translate("PathCustom", "Total invalid lines in Custom G-code: %s")
                % len(self.errors)
            )

            errorNums = ", ".join((str(i) for i, _ in self.errors))
            Path.Log.warning(translate("PathCustom", "Check lines: %s") % errorNums)

            errorLines = "\n" + "\n".join((f"{i} {line}" for i, line in self.errors[:7]))
            if len(self.errors) > 7:
                errorLines += "\n..."
            Path.Log.warning(errorLines)

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
