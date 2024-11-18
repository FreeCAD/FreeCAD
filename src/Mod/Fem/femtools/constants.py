# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
""" Collection of natural constants for the Fem module.

This module contains natural constants for the Fem module.
All constants are in SI units.
"""


__title__ = "FEM collection of natural constants"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def gravity():
    # https://en.wikipedia.org/wiki/Gravitational_acceleration
    return "9.80665 m/s^2"


def stefan_boltzmann():
    # https://en.wikipedia.org/wiki/Stefan-Boltzmann_constant
    return "5.67037e-8 W/(m^2*K^4)"


def vacuum_permeability():
    # https://en.wikipedia.org/wiki/Vacuum_permeability
    return "1.256637e-6 N/A^2"


def vacuum_permittivity():
    # https://forum.freecad.org/viewtopic.php?f=18&p=400959#p400959
    # https://en.wikipedia.org/wiki/Permittivity#Vacuum_permittivity
    return "8.85419e-12 F/m"


def boltzmann_constant():
    # https://en.wikipedia.org/wiki/Boltzmann_constant
    return "1.38065e-23 J/K"


"""
from FreeCAD import Units
from femtools import constants
Units.Quantity(constants.gravity()).getValueAs("mm/s^2")

"""

# TODO: a unit test to be sure these values are returned!
