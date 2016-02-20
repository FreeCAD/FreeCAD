#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015                                                    *
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


__title__="FreeCAD Ship module"
__author__ = "Jose Luis Cercos-Pita"
__url__ = "http://www.freecadweb.org"

__doc__="The Ships module provide a set of tools to make some specific Naval" \
        " Architecture computations"

from shipCreateShip.Tools import createShip
from shipHydrostatics.Tools import areas, displacement, wettedArea, moment
from shipHydrostatics.Tools import floatingArea, BMT, mainFrameCoeff
from shipCreateWeight.Tools import createWeight
from shipCreateTank.Tools import createTank
from shipCapacityCurve.Tools import tankCapacityCurve
from shipCreateLoadCondition.Tools import createLoadCondition
from shipGZ.Tools import gz