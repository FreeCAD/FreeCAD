# /***************************************************************************
#  *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
#  *                                                                         *
#  *   This file is part of FreeCAD.                                         *
#  *                                                                         *
#  *   FreeCAD is free software: you can redistribute it and/or modify it    *
#  *   under the terms of the GNU Lesser General Public License as           *
#  *   published by the Free Software Foundation, either version 2.1 of the  *
#  *   License, or (at your option) any later version.                       *
#  *                                                                         *
#  *   FreeCAD is distributed in the hope that it will be useful, but        *
#  *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#  *   Lesser General Public License for more details.                       *
#  *                                                                         *
#  *   You should have received a copy of the GNU Lesser General Public      *
#  *   License along with FreeCAD. If not, see                               *
#  *   <https://www.gnu.org/licenses/>.                                      *
#  *                                                                         *
#  **************************************************************************/

import FreeCAD
from FreeCAD import Units, Placement
from UtilsMeasure import MeasureBasePython
from PySide.QtCore import QT_TRANSLATE_NOOP


__title__ = "Measure Center of Mass Object"
__author__ = "David Friedli"
__url__ = "http://www.freecad.org"


"""
    The Measure cpp object defines a result and a placement property. The Python measure type
    adds it's own specific properties. Once the object is recomputed the parent properties are updated
    based on the specific python properties.

    We'll need some kind of interface for the measure command which exposes "parseSelection", "isValidSelection" etc.

"""


def makeMeasureCOM(name="CenterOfMass"):
    """makeMeasureCOM(name): make a CenterofMass measurement"""
    obj = FreeCAD.ActiveDocument.addObject("Measure::MeasurePython", name)
    MeasureCOM(obj)
    return obj


class MeasureCOM(MeasureBasePython):
    "The MeasureCOM object"

    def __init__(self, obj):
        obj.Proxy = self

        obj.addProperty(
            "App::PropertyLinkSubGlobal",
            "Element",
            "",
            QT_TRANSLATE_NOOP("App::Property", "Element to measure"),
            locked=True,
        )
        obj.addProperty(
            "App::PropertyPosition",
            "Result",
            "",
            QT_TRANSLATE_NOOP("App::PropertyVector", "The result location"),
            locked=True,
        )

    @classmethod
    def isValidSelection(cls, selection):
        if not len(selection) == 1:
            return False

        element = selection[0]
        ob = element["object"]
        subName = element["subName"]

        if not ob:
            return

        sub = ob.getSubObject(subName)
        if not sub:
            return

        if not hasattr(sub, "CenterOfMass"):
            return

        return True

    @classmethod
    def isPrioritySelection(cls, selection):
        return False

    @classmethod
    def getInputProps(cls):
        return ("Element",)

    def getSubject(self, obj):
        if not obj:
            return ()

        element = obj.Element
        if not element:
            return ()

        ob = element[0]
        if not ob:
            return ()
        return (ob,)

    def parseSelection(self, obj, selection):
        item = selection[0]
        o = item["object"]
        obj.Element = (o, item["subName"])

    def getResultString(self, obj):
        values = [Units.Quantity(v, Units.Length).getUserPreferred()[0] for v in obj.Result]
        return "COM\nX: {}\nY: {}\nZ: {}".format(*values)

    def execute(self, obj):
        element = obj.Element
        if not element:
            return

        ob = element[0]
        subElements = element[1]

        if subElements:
            subName = subElements[0]
            sub = ob.getSubObject(subName)

            if not sub or not hasattr(sub, "CenterOfMass"):
                return
            com = sub.CenterOfMass

        else:
            # Get Center of Mass of the object
            if not hasattr(ob, "Shape"):
                return

            shape = ob.Shape
            if not hasattr(shape, "CenterOfMass"):
                return

            com = shape.CenterOfMass

        obj.Result = com
        placement = Placement()
        placement.Base = com
        obj.Placement = placement

    def onChanged(self, obj, prop):
        """Do something when a property has changed"""

        if prop == "Element":
            self.execute(obj)
