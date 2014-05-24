#***************************************************************************
#*																		 *
#*   Copyright (c) 2011, 2012											  *  
#*   Jose Luis Cercos Pita <jlcercos@gmail.com>							*  
#*																		 *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)	*
#*   as published by the Free Software Foundation; either version 2 of	 *
#*   the License, or (at your option) any later version.				   *
#*   for detail see the LICENCE text file.								 *
#*																		 *
#*   This program is distributed in the hope that it will be useful,	   *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of		*
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		 *
#*   GNU Library General Public License for more details.				  *
#*																		 *
#*   You should have received a copy of the GNU Library General Public	 *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA																   *
#*																		 *
#***************************************************************************

import FreeCAD
import Units


# Systems of length units
LENGTH_UNITS = ('mm', 'm', 'in', 'in')


def getLengthUnits():
    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
    units_id = param.GetInt('UserSchema', 0)
    return LENGTH_UNITS[units_id]


def getLengthFormat():
    param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
    decimals = param.GetInt("Decimals", 2)
    units_id = param.GetInt('UserSchema', 0)
    return '{0:.' + str(decimals) + 'f} ' + LENGTH_UNITS[units_id]
