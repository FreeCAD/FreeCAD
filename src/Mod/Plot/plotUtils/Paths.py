#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
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

import FreeCAD, FreeCADGui, os

def modulePath():
    """returns the current Plot module path
    @return Module path"""
    path1 = FreeCAD.ConfigGet("AppHomePath") + "Mod/Plot"
    path2 = FreeCAD.ConfigGet("UserAppData") + "Mod/Plot"
    if os.path.exists(path2):
        return path2
    else:
        return path1

def iconsPath():
    """returns the current Plot module icons path
    @return Icons path"""
    path = modulePath() + "/resources/icons"
    return path

def translationsPath():
    """returns the current Plot module translations path
    @return Icons path"""
    path = modulePath() + "/resources/translations"
    return path

def getPathFromFile(fileName):
    """ Gets the directory path from a file name
    @param fileName Name of the file
    @return Directory path.
    """
    if not fileName:
        return ''
    i = 1
    try:
        while 1:
            i = fileName.index("/", i+1)
    except ValueError:
        pass
    return fileName[0:i+1]
