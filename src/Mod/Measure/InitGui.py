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


# Import the measure module here in order to ensure that default measurement types are loaded during startup.
# Note that they won't auto-load in gui-less mode. Ideally they would be loaded from "Init.py", similar
# to how the import/export types are registered. This would require to register measurement types from
# python which could complicate things further.

import Measure
import MeasureGui
from MeasureCOM import makeMeasureCOM, MeasureCOM


# Expose create functions
Measure.makeMeasureCOM = makeMeasureCOM


# Register python measure types
import FreeCAD

FreeCAD.MeasureManager.addMeasureType(
    "CENTEROFMASS",
    "Center of mass",
    MeasureCOM,
)
