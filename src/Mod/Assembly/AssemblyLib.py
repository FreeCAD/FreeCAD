#! python
# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012                                                    *  
#*   Juergen Riegel <FreeCAD@juergen-riegel.net>                           *  
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



__title__="FreeCAD Assembly Lib"
__author__ = "Juergen Riegel <FreeCAD@juergen-riegel.net>"
__url__ = "http://free-cad.sourceforge.net"

'''
General description:

    This set of classes is aimed for general methods used in the FreeCAD Assembly module

User manual:

    TODO

How it works / how to extend:
	TODO
'''

# import FreeCAD modules
import FreeCAD, Part, Assembly
from FreeCAD import Vector

if FreeCAD.GuiUp:
    import FreeCADGui, AssemblyGui
    gui = True
else:
    gui = False

#---------------------------------------------------------------------------
# General functions
#---------------------------------------------------------------------------




#---------------------------------------------------------------------------
# Import methods 
#---------------------------------------------------------------------------


def importAssembly(FileName,DestItem):
	for i in Part.read(FileName).Solids:
		po = FreeCAD.activeDocument().addObject('Assembly::ItemPart','STP-Part')
		DestItem.Items = DestItem.Items + [po]
		bo = FreeCAD.activeDocument().addObject('PartDesign::Body','STP-Body')
		po.Model = bo
		so = FreeCAD.activeDocument().addObject('PartDesign::Solid','STP-Solid')
		bo.Model = so
		bo.Tip   = so
		so.Shape = i

