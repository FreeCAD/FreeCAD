# -*- coding: utf-8 -*-
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2010 Heiko Jakob <heiko.jakob@gediegos.de>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
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

import re, FreeCAD, FreeCADGui, Part, cProfile, os, string
from FreeCAD import Vector, Base
from Draft import *

pythonopen = open
useDraftWire = True

def decodeName(name):
	"decodes encoded strings"
	try:
		decodedName = (name.decode("utf8"))
	except UnicodeDecodeError:
		try:
			decodedName = (name.decode("latin1"))
		except UnicodeDecodeError:
			print "AirfoilDAT: error: couldn't determine character encoding"
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
	except:
		doc=FreeCAD.newDocument(docname)
	importgroup = doc.addObject("App::DocumentObjectGroup",groupname)
	importgroup.Label = decodeName(groupname)
	process(doc,filename)

def process(doc,filename):    
        # The common airfoil dat format has many flavors
        # This code should work with almost every dialect
    
        # Regex to identify data rows and throw away unused metadata
        expre = r"^\s*(?P<xval>\-*\d*?\.\d\d+)\s+(?P<yval>\-*\d*?\.\d+)\s*$"
        afile = pythonopen(filename,'r')
        # read the airfoil name which is always at the first line
        airfoilname = afile.readline().strip()
    
        upper=[]
        lower=[]
        upside=True
        last_x=None
    

        # Collect the data for the upper and the lower side seperately if possible     
        for lin in afile:
                curdat = re.match(expre,lin)
                if curdat != None:         
                        x = float(curdat.group("xval"))
                        y = float(curdat.group("yval"))

                        if last_x == None:
                                last_x=x

                        # Separation between the sides is done in many different ways.
                        # The only way to safely detect the swap is when x is getting smaller
                        if x < last_x:
                                # The swap
                                upside=False        
                                # Not needed because this will happen anyhow at the end of the loop last_x=x 

                        # the normal processing
                        if upside:
                                upper.append(Vector(x,y,0))
                        else:
                                lower.append(Vector(x,y,0))              
                        last_x=x
                # End of if curdat != None
        # End of for lin in file
        afile.close()
  
        # reverse the lower side and append it to the upper to 
        # make the complete data be oriented clockwise
        lower.reverse() 
        for i in lower:
                upper.append(i)
        # End of for i in lower
       
        # do we use the parametric Draft Wire?
        if useDraftWire:
                face = makeWire ( upper, True, None, True )
        else:
                # alternate solution, uses common Part Faces
                lines = []
                first_v = None
                last_v = None
                for v in upper:
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
                Part.show(face)
  
        doc.recompute()
