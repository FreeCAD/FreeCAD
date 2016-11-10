#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2016                                              *  
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

from PySide import QtCore


def toString(valueStr):
    """Natural extension of QtCore.QLocale.toString method, in this case
    conveniently transforming a value string"""
    dec_sep = QtCore.QLocale.system().decimalPoint()
    return valueStr.replace(".", dec_sep)

def fromString(valueStr):
    """Natural extension of QtCore.QLocale.toFloat method, in this case
    conveniently transforming a value string"""
    grp_sep = QtCore.QLocale.system().groupSeparator()
    return valueStr.replace(grp_sep, "")
