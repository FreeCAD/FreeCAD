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

# COIN
from pivy.coin import *
from pivy import coin

# FreeCAD
import FreeCAD,FreeCADGui
from FreeCAD import Part, Base, Vector

# Surface utils
from surfUtils import Geometry, Math, Translator

class ControlPoints:
    def __init__(self, obj, aim):
        """ Constructor.
        @param obj Part::FeaturePython built object that must be converted into control points.
        @param aim Face object where control poinst must be showed.
        """
        # Ensure that right object has been provided
        if not aim.isDerivedFrom('Part::Feature'):
            msg = Translator.translate("Control points must be generated on top of Part::Feature object.\n")
            App.Console.PrintError(msg)
            return            
        faces = Geometry.getFaces(aim)
        if not faces:
            msg = Translator.translate("Any face object found into provided object.\n")
            App.Console.PrintError(msg)
            return
        if len(faces) > 1:
            msg = Translator.translate("Only first face will considereed.\n")
            App.Console.PrintWarning(msg)
        face = faces[0]
        # Add face object data
        obj.addProperty("App::PropertyString","Object","Face", str(Translator.translate("Object referenced by control points"))).Object=aim.Name
        nUV = self.getNumberOfPoles(face)
        obj.addProperty("App::PropertyInteger","nU","Face", str(Translator.translate("Number of U knots"))).nU=nUV[0]
        obj.addProperty("App::PropertyInteger","nV","Face", str(Translator.translate("Number of V knots"))).nV=nUV[1]
        # Set object as unselectable
        FreeCADGui.ActiveDocument.getObject(aim.Name).Selectable = False
        FreeCADGui.ActiveDocument.getObject(aim.Name).Transparency = 50
        # Perform shape
        obj.Shape = self.buildVertexes(self.getPoles(face))
        obj.Shape = self.buildEdges(obj)
        # Set proxy as provided object
        obj.Proxy = self    
        # Set some visual propeties
        vObj = FreeCADGui.ActiveDocument.getObject(obj.Name)
        vObj.LineColor = (0.0,0.0,1.0)
        vObj.LineWidth = 1.0
        vObj.PointColor = (0.0,0.0,1.0)
        vObj.PointSize = 4.0
        # Add control points data
        obj.addProperty("App::PropertyBool","ValidCtrlPoints","Instance", str(Translator.translate("True if is a valid control points instance"))).ValidCtrlPoints=True
        FreeCADGui.Selection.clearSelection()
        
    def onChanged(self, fp, prop):
        """ Method called when property changes """
        if prop == "Face":
            # Get object
            obj = FreeCAD.ActiveDocument.getObject(fp.Face)
            if not obj:
                msg = Translator.translate("Selected object don't exist.\n")
                App.Console.PrintError(msg)
            faces = obj.Faces
            if not faces:
                msg = Translator.translate("Selected object don't contain faces.\n")
                App.Console.PrintError(msg)
            face = faces[0]
            fp.Shape = self.buildVertexes(self.getPoles(face))
            fp.Shape = self.buildEdges(fp)

    def execute(self, fp):
        """ Method called when entity recomputation is needed """
        # Append edges list to the vertexes array
        vList = fp.Shape.Vertexes[0:]
        eList = fp.Shape.Edges
        for i in range(0,len(eList)):
            vList.append(eList[i])
        fp.Shape = Part.Compound(vList)

    def getNumberOfPoles(self, obj):
        """ Gets face number of poles.
        @param obj Face object.
        @return [nU,nV].
        """
        surf = obj.Surface
        poles  = surf.getPoles()
        nU = len(poles)
        if not nU:
            return [0,0]
        nV = len(poles[0])
        return [nU,nV]

    def getPoles(self, obj):
        """ Gets face object poles (at 3D coordinates).
        @param obj Face object.
        @return Control points coordinates.
        @note Returned points are Base.Vector entities, not Part.Vertexes.
        """
        points = []
        surf   = obj.Surface
        poles  = surf.getPoles()
        # Get Poles UV map positions
        nU = len(poles)
        if not nU:
            return []
        nV = len(poles[0])
        # Loop over knots
        for i in range(0,nU):
            points.append([])
            for j in range(0,nV):
                point = poles[i][j]
                points[i].append(point)
        return points

    def buildVertexes(self, points):
        """ Builds the vertexes array.
        @param points Control points.
        @return Vertexes shape.
        @note Input control points must be Base.Vector entities, not Part.Vertex.
        """
        shape = None
        vList = []
        # Loop over control points
        for i in range(0,len(points)):
            for j in range(0,len(points[i])):
                point = points[i][j]
                v = Part.Vertex(point.x, point.y, point.z)
                vList.append(v)
        shape = Part.Compound(vList)
        return shape

    def buildEdges(self, fp):
        """ Builds the edges array.
        @param fp Part::FeaturePython built object that contains control points data.
        @return Shape filled with the vertexes and the array of control points.
        """
        obj = FreeCAD.ActiveDocument.getObject(fp.Object)
        if not obj:
            msg = Translator.translate("Can't find refered face object. Maybe you delete it?\n")
            App.Console.PrintError(msg)
            return None
        faces = obj.Shape.Faces
        if not faces:
            msg = Translator.translate("Can't find surface into refered object. Maybe you modify it?\n")
            App.Console.PrintError(msg)
            return None
        face  = faces[0]
        surf  = face.Surface
        poles = surf.getPoles()
        # Get Poles UV map positions
        nU = len(poles)
        if not nU:
            return None
        nV = len(poles[0])
        if nU != fp.nU or nV != fp.nV:
            msg = Translator.translate("Surface UV coordinates don't match. Maybe you modify it?\n")
            App.Console.PrintError(msg)
            return None
        shape = None
        eList = []
        # U direction
        for i in range(0,fp.nU):
            uv = surf.parameter(poles[i][0])
            eList.append(surf.uIso(uv[0]).toShape())
        # V direction
        for i in range(0,fp.nV):
            uv = surf.parameter(poles[0][i])
            eList.append(surf.vIso(uv[1]).toShape())
        # Append edges list to the vertexes array
        vList = fp.Shape.Vertexes[0:fp.nU*fp.nV]
        for i in range(0,len(eList)):
            vList.append(eList[i])
        shape = Part.Compound(vList)
        return shape

    def movedVertexes(self,obj,vertexes):
        """ Renew object with modified vertexes
        @param obj Part::FeaturePython built object that contains control points data.
        @param vertexes New vertexes list.
        """
        nU = obj.nU
        nV = obj.nV
        n = nU*nV
        if n != len(vertexes):
            msg = Translator.translate("Number of vertexes don't match.\n")
            App.Console.PrintError(msg)
            return
        self.setSurface(obj, vertexes)
        obj.Shape = Part.Compound(vertexes)
        obj.Shape = self.buildEdges(obj)

    def setSurface(self,fp,vertexes):
        """ Transform surface with modified vertexes
        @param fp Part::FeaturePython built object that contains control points data.
        @param vertexes New vertexes list.
        """
        obj = FreeCAD.ActiveDocument.getObject(fp.Object)
        if not obj:
            msg = Translator.translate("Can't find refered face object. Maybe you delete it?\n")
            App.Console.PrintError(msg)
            return
        faces = obj.Shape.Faces
        if not faces:
            msg = Translator.translate("Can't find surface into refered object. Maybe you modify it?\n")
            App.Console.PrintError(msg)
            return
        face = faces[0]
        surf = face.Surface
        poles = surf.getPoles()
        # Get Poles UV map positions
        nU = len(poles)
        if not nU:
            return None
        nV = len(poles[0])
        if fp.nU != nU or fp.nV != nV:
            msg = Translator.translate("Surface UV coordinates don't match. Maybe you modify it?\n")
            App.Console.PrintError(msg)
            return
        for i in range(0,nU):
            for j in range(0,nV):
                vID  = vertexes[i*nV + j]
                dest = FreeCAD.Base.Vector(vID.X, vID.Y, vID.Z)
                orig = poles[i][j]
                if not Math.isSamePoint(orig,dest):
                    uv = surf.parameter(orig)
                    surf.movePoint(uv[0],uv[1],dest,i,i,j,j)

class ViewProviderShip:
    def __init__(self, obj):
        "Set this object to the proxy object of the actual view provider"
        obj.Proxy = self
        obj.DisplayMode = 'Flat Lines'

    def attach(self, obj):
        ''' Setup the scene sub-graph of the view provider, this method is mandatory '''
        return

    def updateData(self, fp, prop):
        ''' If a property of the handled feature has changed we have the chance to handle this here '''
        return

    def getDisplayModes(self,obj):
        ''' Return a list of display modes. '''
        modes=['Flat Lines']
        return modes

    def getDefaultDisplayMode(self):
        ''' Return the name of the default display mode. It must be defined in getDisplayModes. '''
        return 'Flat Lines'

    def setDisplayMode(self,mode):
        ''' Map the display mode defined in attach with those defined in getDisplayModes.
        Since they have the same names nothing needs to be done. This method is optinal.
        '''
        return mode

    def onChanged(self, vp, prop):
        ''' Print the name of the property that has changed '''
        if prop == "Visibility":
            obj  = vp.Object
            face = obj.Object
            if vp.Visibility:
                FreeCADGui.ActiveDocument.getObject(face).Selectable = False
                FreeCADGui.ActiveDocument.getObject(face).Transparency = 50
            else:
                FreeCADGui.ActiveDocument.getObject(face).Selectable = True
                FreeCADGui.ActiveDocument.getObject(face).Transparency = 0

    def __getstate__(self):
        ''' When saving the document this object gets stored using Python's cPickle module.
        Since we have some un-pickable here -- the Coin stuff -- we must define this method
        to return a tuple of all pickable objects or None.
        '''
        return None

    def __setstate__(self,state):
        ''' When restoring the pickled object from document we have the chance to set some
        internals here. Since no data were pickled nothing needs to be done here.
        '''
        return None

    def getIcon(self):
        return """
        /* XPM */
        static char * ControlPoints_xpm[] = {
        "32 32 3 1",
        " 	c None",
        ".	c #20B0AB",
        "+	c #0000FF",
        "                                ",
        "                                ",
        "       ..                       ",
        "        ..                      ",
        "                                ",
        "                                ",
        "        ..                      ",
        "         .                      ",
        "                                ",
        "          .                     ",
        "         ..                     ",
        "          .                     ",
        "                                ",
        "           .                    ",
        "          ..                    ",
        "          .                     ",
        "       +++++++++++              ",
        "       +++++++++++              ",
        "       ++       ++              ",
        "       ++       ++            . ",
        "       ++       ++    ..  ..  . ",
        "       ++       ++..  ..  ..    ",
        "       ++       ++..            ",
        "       ++       ++              ",
        "       ++       ++              ",
        "       +++++++++++              ",
        "       +++++++++++              ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                "};
        """
