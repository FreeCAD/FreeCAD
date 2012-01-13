#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011, 2012                                              *  
#*   Jose Luis Cercós Pita <jlcercos@gmail.com>                            *  
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

# FreeCAD modules
import FreeCAD
import FreeCADGui
from FreeCAD import Base
from FreeCAD import Part

# Qt libraries
from PyQt4 import QtGui,QtCore

# Main object
from surfUtils import Geometry, Translator

def load():
    """ Loads the tool. Getting the border don't require any
     option, so can be executed directly without any task panel. """
    edges = Geometry.getBorders()
    if not edges:
        wrn = Translator.translate("Can't get any edge from selected objects")
        FreeCAD.Console.PrintWarning(wrn)
        return
    obj = edges[0]
    for i in range(0,len(edges)):
        obj = obj.oldFuse(edges[i])
    Part.show(obj)
    objs = FreeCAD.ActiveDocument.Objects
    obj = objs[len(objs)-1]
    obj.Label = 'Border'
    
