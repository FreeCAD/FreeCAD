# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2010 Heiko Jakob <heiko.jakob@gediegos.de>              *
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

__title__="FreeCAD Draft Workbench - Airfoil data importer"
__author__ = "Heiko Jakob <heiko.jakob@gediegos.de>"

## @package importAirfoilDAT
#  \ingroup DRAFT
#  \brief Airfoil (.dat) file importer
#
# This module provides support for importing airfoil .dat files

import re, FreeCAD, FreeCADGui, Part, cProfile, os, string
from FreeCAD import Vector, Base
from Draft import makeWire

if open.__module__ in ['__builtin__','io']:
        pythonopen = open

useDraftWire = True

def decodeName(name):
	"decodes encoded strings"
	try:
		decodedName = name
	except UnicodeDecodeError:
		try:
			decodedName = (name.decode("latin1"))
		except UnicodeDecodeError:
			try:
				decodedName = (name.decode("utf8"))
			except UnicodeDecodeError:
				print("AirfoilDAT: error: couldn't determine character encoding")
				decodedName = name
	return decodedName

def open(filename):
	"called when freecad opens a file"
	docname = os.path.splitext(os.path.basename(filename))[0]
	doc = FreeCAD.newDocument(docname)
	doc.Label = decodeName(docname[:-4])
	process(doc,filename)

def insert(filename,docname):
	"called when freecad imports a file"
	groupname = os.path.splitext(os.path.basename(filename))[0]
	try:
		doc=FreeCAD.getDocument(docname)
	except NameError:
		doc=FreeCAD.newDocument(docname)
	importgroup = doc.addObject("App::DocumentObjectGroup",groupname)
	importgroup.Label = decodeName(groupname)
	process(doc,filename)

def process(doc,filename):    
        # The common airfoil dat format has many flavors
        # This code should work with almost every dialect
    
        # Regex to identify data rows and throw away unused metadata
        regex = re.compile(r'^\s*(?P<xval>(\-|\d*)\.\d+(E\-?\d+)?)\,?\s*(?P<yval>\-?\s*\d*\.\d+(E\-?\d+)?)\s*$')
        afile = pythonopen(filename,'r')
        # read the airfoil name which is always at the first line
        airfoilname = afile.readline().strip()
    
        coords=[]
        upside=True
        last_x=None

    

        # Collect the data for the upper and the lower side separately if possible     
        for lin in afile:
                curdat = regex.match(lin)
                if curdat != None:   
                        
                        x = float(curdat.group("xval"))
                        y = float(curdat.group("yval"))

                        # the normal processing
                        coords.append(Vector(x,y,0))               

                # End of if curdat != None
        # End of for lin in file
        afile.close()
        
        
        
        if len(coords) < 3:
                print('Did not find enough coordinates\n')
                return 
                
        # sometimes coords are divided in upper an lower side
        # so that x-coordinate begin new from leading or trailing edge
        # check for start coordinates in the middle of list                

        if coords[0:-1].count(coords[0]) > 1:
                flippoint = coords.index(coords[0],1)
                upper = coords[0:flippoint]
                lower = coords[flippoint+1:]
                lower.reverse()
                for i in lower:
                        upper.append(i)  
                coords = upper
                
      
        # do we use the parametric Draft Wire?
        if useDraftWire:
                obj = makeWire ( coords, True )
                #obj.label = airfoilname
        else:
                # alternate solution, uses common Part Faces
                lines = []
                first_v = None
                last_v = None
                for v in coords:
                        if first_v == None:
                                first_v = v
                        # End of if first_v == None
    
                        # Line between v and last_v if they're not equal
                        if (last_v != None) and (last_v != v):
                                lines.append(Part.makeLine(last_v, v))       
                        # End of if (last_v != None) and (last_v != v)
                        # The new last_v
                        last_v = v     
                # End of for v in upper
                # close the wire if needed
                if last_v != first_v:
                        lines.append(Part.makeLine(last_v, first_v))
                # End of if last_v != first_v
  
                wire = Part.Wire(lines)
                face = Part.Face(wire) 
                obj = FreeCAD.ActiveDocument.addObject('Part::Feature',airfoilname) 
                obj.Shape = face
  
        doc.recompute()
