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

# FreeCAD modules
import FreeCAD,FreeCADGui
from FreeCAD import Base, Part, Vector
# FreeCADShip modules
from shipUtils import Paths, Translator

class Preview(object):
    def __init__(self):
        """ Constructor.
        """
        self.obj = None
        self.reinit()

    def reinit(self):
        """ Reinitializate drawer.
        """
        self.clean()

    def update(self, L, B, T, sectionsL, sectionsB, sectionsT, shape):
        """ Update the 3D view printing annotations.
        @param L Ship Lpp.
        @param B Ship beam.
        @param T Ship draft.
        @param sectionsL Transversal sections.
        @param sectionsB Longitudinal sections.
        @param sectionsT Water lines.
        @param shape Ship surfaces shell
        @return Sections object. None if errors happens.
        """
        FreeCAD.Console.PrintMessage(Translator.translate('Computing sections...\n'))
        # Destroy all previous entities
        self.clean()
        # Receive data
        nL = len(sectionsL)
        nB = len(sectionsB)
        nT = len(sectionsT)
        if not (nL or nB or nT):
            return None
        # Found sections
        sections = []
        for i in range(0,nL):
            pos = sectionsL[i]
            section = shape.slice(Vector(1.0,0.0,0.0), pos)
            for j in range(0,len(section)):
                edges = section[j].Edges
                if pos == 0.0:
                    section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0))
                    edges2 = section[j].Edges
                    for k in range(0,len(edges2)):
                        edges.append(edges2[k])
                elif pos < 0:
                    section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0))
                    edges = section[j].Edges                    
                for k in range(0,len(edges)):
                    sections.append(edges[k])
        for i in range(0,nB):
            pos = sectionsB[i]
            section = shape.slice(Vector(0.0,1.0,0.0), pos)
            for j in range(0,len(section)):
                edges = section[j].Edges
                section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0))
                edges2 = section[j].Edges
                for k in range(0,len(edges2)):
                    edges.append(edges2[k])
                for k in range(0,len(edges)):
                    sections.append(edges[k])
        for i in range(0,nT):
            pos = sectionsT[i]
            section = shape.slice(Vector(0.0,0.0,1.0), pos)
            for j in range(0,len(section)):
                edges = section[j].Edges
                if pos == T:
                    section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0))
                    edges2 = section[j].Edges
                    for k in range(0,len(edges2)):
                        edges.append(edges2[k])
                elif pos > T:
                    section[j] = section[j].mirror(Vector(0.0, 0.0, 0.0),Vector(0.0, 1.0, 0.0))
                    edges = section[j].Edges                    
                for k in range(0,len(edges)):
                    sections.append(edges[k])
        # Convert all BSplines into a shape
        if not sections:
            msg = Translator.translate('Any valid ship section found\n')
            FreeCAD.Console.PrintWarning(msg)
            return
        obj = sections[0]
        for i in range(1,len(sections)):
            obj = obj.oldFuse(sections[i])  # Only group the edges, don't try to build more complex entities
        # Create the representable object
        Part.show(obj)
        objs = FreeCAD.ActiveDocument.Objects
        self.obj = objs[len(objs)-1]
        self.obj.Label = 'OutlineDraw'
        return self.obj
        
    def clean(self):
        """ Erase all annotations from screen.
        """
        if not self.obj:
            return
        FreeCAD.ActiveDocument.removeObject(self.obj.Name)
        self.obj = None
