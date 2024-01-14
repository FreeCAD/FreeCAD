#***************************************************************************
#*   Copyright (c) 2002,2003 Jürgen Riegel <juergen.riegel@Measure.de>         *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#***************************************************************************/


# Import the measure module here in order to ensure that default measurement types are loaded during startup.
# Note that they won't auto-load in gui-less mode. Ideally they would be loaded from "Init.py", similar
# to how the import/export types are registered. This would require to register measurement types from
# python which could complicate things further.

import Measure
from MeasureCOM import makeMeasureCOM, MeasureCOM


# Expose create functions
Measure.makeMeasureCOM = makeMeasureCOM


# Register python measure types
import FreeCAD
FreeCAD.addMeasureType(
        "CENTEROFMASS",
        "Center of Mass",
        MeasureCOM,
)

