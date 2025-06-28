# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Daniel Wood <s.d.wood.82@gmail.com>                *
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

from PySide import QtCore

import FreeCAD
import FreeCADGui


class TurnToolHelperPanel:
    """TurnToolHelperPanel ... GUI for Turn Tool Helper
    """
    def __init__(self):

        self.form = self.getForm()

        self.tipAngle = {
            "A": 85,  # Parallelogram (85 degree)
            "B": 82,  # Parallelogram (82 degree)
            "C": 80,  # Rhombic (80 degree)
            "D": 55,  # Rhombic (55 degree)
            "E": 75,  # Rhombic (75 degree)
            "F": 50,  # Rhombic (50 degree)
            "H": 120,  # Hexagonal
            "K": 55,  # Parallelogram (55 degree)
            "L": 90,  # Rectangular
            "M": 86,  # Rhombic (86 degree)
            "O": 135,  # Octagonal
            "P": 108,  # Pentagonal
            "R": 90,  # Round
            "S": 90,  # Square
            "T": 60,  # Triangular
            "V": 35,  # Rhombic (35 degree)
            "W": 60,  # Trigon
            "X": None,  # Special Shape
        }

        self.shapeSize = {
            "C": {
                "03": 3.97,
                "04": 4.76,
                "05": 5.56,
                "06": 6.35,
                "08": 7.94,
                "09": 9.525,
                "12": 12.7,
                "16": 15.875,
                "19": 19.05,
                "22": 22.225,
                "25": 25.4,
            },
            "D": {
                "04": 3.97,
                "05": 4.76,
                "06": 5.56,
                "07": 6.35,
                "09": 7.94,
                "11": 9.525,
                "15": 12.7,
                "19": 15.875,
                "23": 19.05,
            },
            "R": {
                "06": 6.0,
                "08": 8.0,
                "09": 9.525,
                "10": 10,
                "12": 12.0,
                "16": 16,
                "20": 20,
                "25": 25,
            },
            "S": {
                "03": 3.97,
                "04": 4.76,
                "05": 5.56,
                "06": 6.35,
                "08": 7.94,
                "09": 9.525,
                "12": 12.7,
                "16": 15.875,
                "19": 19.05,
                "22": 22.225,
                "25": 25.4,
            },
            "T": {
                "08": 4.76,
                "09": 5.56,
                "11": 6.35,
                "13": 7.94,
                "16": 9.525,
                "22": 12.7,
                "27": 15.875,
                "33": 19.05,
                "38": 22.225,
                "44": 25.4,
            },
            "V": {"08": 4.76, "09": 5.56, "11": 6.35, "13": 7.94, "16": 9.525, "22": 12.7},
            "W": {
                "02": 3.97,
                "L3": 4.76,
                "03": 5.56,
                "04": 6.35,
                "05": 7.94,
                "06": 9.525,
                "08": 12.7,
                "10": 15.875,
                "13": 19.05,
            },
            "X": {},
        }

        self.noseRadius = {
            "00": 0,  # sharp
            "V3": 0.03,
            "V5": 0.05,
            "01": 0.1,
            "02": 0.2,
            "04": 0.4,
            "08": 0.8,
            "12": 1.2,
            "16": 1.6,
            "20": 2.0,
            "24": 2.4,
            "28": 2.8,
            "32": 3.2,
        }

        # Load UI Components
        self.shapeComboBox = self.form.shapeComboBox
        self.sizeComboBox = self.form.sizeComboBox
        self.radiusComboBox = self.form.radiusComboBox
        self.directionComboBox = self.form.directionComboBox

        # self.shapeLabel = self.form.shapeLabel
        self.sizelabel = self.form.sizelabel
        # self.rotationLabel = self.form.rotationLabel
        self.tipangleLabel = self.form.tipangleLabel
        self.radiusLabel = self.form.radiusLabel
        # self.directionLabel = self.form.directionLabel
        self.resultLabel = self.form.resultLabel

        # connect
        self.shapeComboBox.currentIndexChanged.connect(self.loadShapeSize)
        self.shapeComboBox.currentIndexChanged.connect(self.loadToolData)
        self.sizeComboBox.currentIndexChanged.connect(self.loadToolData)
        self.radiusComboBox.currentIndexChanged.connect(self.loadToolData)
        self.directionComboBox.currentIndexChanged.connect(self.loadToolData)

        self.loadShapeSize(0)

    def getForm(self):
        """getForm() ... return UI"""
        return FreeCADGui.PySideUic.loadUi(":/panels/DlgTurnToolHelper.ui")

    def loadToolData(self):
        """
        Load all tool data and create a tool string
        """
        if self.sizeComboBox.currentText():
            _shape = self.shapeComboBox.currentText()
            _size = self.sizeComboBox.currentText()
            _radius = self.radiusComboBox.currentText()
            _direction = self.directionComboBox.currentText()
            _sizelabel = self.getEdgeLength(_shape, _size)
            _radiusLabel = self.getRadiusValue(_radius)
            _tip_angle = self.getTipAngle(_shape)

            # print('tool data', _shape, _size, _radius, _direction)
            # self.shapeLabel.setText(_shape)
            self.sizelabel.setText(str(_sizelabel))
            self.tipangleLabel.setText(str(_tip_angle))

            if _shape == "X":
                _radius = "-"
                _direction = "-"
                self.radiusLabel.setText("-")
                # self.directionLabel.setText("-")
            else:
                self.radiusLabel.setText(str(_radiusLabel))
                # self.directionLabel.setText(_direction)

            toolString = "{shape}---{size}--{radius}--{direction}".format(
                shape=_shape, size=_size, radius=_radius, direction=_direction
            )
            self.resultLabel.setText(toolString)

    def getEdgeLength(self, _shape, _length):
        """
        Return the edge length for the tool
        """

        try:
            edgeLength = self.shapeSize[_shape][_length]
            return edgeLength
        except KeyError:
            return "-"

    def getRadiusValue(self, radius):
        """
        Return the nose radius for the tool
        """

        try:
            _radius = self.noseRadius[radius]
            return _radius
        except KeyError:
            return "-"

    def getTipAngle(self, shape):
        """
        Return the tip angle for the tool
        """

        try:
            _tip_angle = self.tipAngle[shape]
            return _tip_angle
        except KeyError:
            return "-"

    def loadShapeSize(self, index):
        """
        Load the sizes for the selected tool shape
        """
        shape = self.shapeComboBox.itemText(index)
        shape_size = self.shapeSize[shape]
        self.sizeComboBox.clear()

        if shape == "X":
            self.sizeComboBox.addItem("-")
        else:
            for key in shape_size:
                self.sizeComboBox.addItem(key)

    def setupUi(self):
        pass

    def show(self):
        self.form.show()
        self.form.exec_()

    def reject(self):
        FreeCAD.Console.PrintMessage("Reject Signal")
        self.quit()

    def accept(self):
        self.quit()

    def quit(self):
        FreeCADGui.Control.closeDialog(self)

    def reset(self):
        pass


class CommandTurnToolHelper:
    """CommandTurnToolHelper ... Command to open Turn Tool Helper GUI
    """

    def GetResources(self):
        return {
            "Pixmap": "CAM_TurnToolHelper",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("CAM", "Turn Tool Helper"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP("CAM", "Derive toolbit parameters from isocode"),
        }

    def Activated(self):
        panel = TurnToolHelperPanel()
        panel.show()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_TurnToolHelper", CommandTurnToolHelper())
