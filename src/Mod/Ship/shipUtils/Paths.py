# ##### BEGIN GPL LICENSE BLOCK #####
#
#  Author: Jose Luis Cercos Pita <jlcercos@gmail.com>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

import FreeCAD, FreeCADGui, os

def modulePath():
    """returns the current Ship design module path
    @return Module path"""
    path1 = FreeCAD.ConfigGet("AppHomePath") + "Mod/Ship"
    path2 = FreeCAD.ConfigGet("UserAppData") + "Mod/Ship"
    if os.path.exists(path2):
        return path2
    else:
        return path1

def iconsPath():
    """returns the current Ship design module icons path
    @return Icons path"""
    path = modulePath() + "/Icons"
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
