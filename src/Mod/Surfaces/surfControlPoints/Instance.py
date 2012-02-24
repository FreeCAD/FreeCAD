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
        nUV = self.getNumberOfKnots(face)
        obj.addProperty("App::PropertyInteger","nU","Face", str(Translator.translate("Number of U knots"))).nU=nUV[0]
        obj.addProperty("App::PropertyInteger","nV","Face", str(Translator.translate("Number of V knots"))).nV=nUV[1]
        # Set object as unselectable
        FreeCADGui.ActiveDocument.getObject(aim.Name).Selectable = False
        FreeCADGui.ActiveDocument.getObject(aim.Name).Transparency = 50
        # Perform shape
        obj.Shape = self.buildVertexes(self.getKnots(face))
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
            fp.Shape = self.buildVertexes(self.getKnots(face))
            fp.Shape = self.buildEdges(fp)

    def execute(self, fp):
        """ Method called when entity recomputation is needed """
        # Append edges list to the vertexes array
        vList = fp.Shape.Vertexes
        eList = fp.Shape.Edges
        for i in range(0,len(eList)):
            vList.append(eList[i])
        fp.Shape = Part.Compound(vList)

    def getNumberOfKnots(self, obj):
        """ Gets face number of knots.
        @param obj Face object.
        @return [nU,nV].
        """
        surf = obj.Surface
        # Get Knots UV map positions
        uKnots = surf.UKnotSequence
        vKnots = surf.VKnotSequence
        return [len(uKnots), len(vKnots)]

    def getKnots(self, obj):
        """ Gets face object knots (at 3D coordinates).
        @param obj Face object.
        @return Control points coordinates.
        @note Returned points are Base.Vector entities, not Part.Vertexes.
        """
        points = []
        surf = obj.Surface
        # Get Knots UV map positions
        uKnots = surf.UKnotSequence
        vKnots = surf.VKnotSequence
        # Loop over knots
        for i in range(0,len(uKnots)):
            points.append([])
            for j in range(0,len(vKnots)):
                uv = [uKnots[i],vKnots[j]]
                point = surf.value(uv[0],uv[1])
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

    def buildEdges(self, obj):
        """ Builds the edges array.
        @param obj Part::FeaturePython built object that contains control points data.
        @return Shape filled with the vertexes and the array of control points.
        """
        shape = None
        eList = []
        # V direction
        for i in range(0,obj.nU):
           for j in range(1,obj.nV):
                v1 = obj.Shape.Vertexes[i*obj.nV + j]
                v0 = obj.Shape.Vertexes[i*obj.nV + j - 1]
                if Math.isSameVertex(v0,v1):
                    continue
                e = Part.Edge(v0,v1)
                eList.append(e)
        # U direction
        for j in range(0,obj.nV):
           for i in range(1,obj.nU):
                v1 = obj.Shape.Vertexes[i*obj.nV + j]
                v0 = obj.Shape.Vertexes[(i-1)*obj.nV + j]
                if Math.isSameVertex(v0,v1):
                    continue
                e = Part.Edge(v0,v1)
                eList.append(e)
        # Append edges list to the vertexes array
        vList = obj.Shape.Vertexes
        for i in range(0,len(eList)):
            vList.append(eList[i])
        shape = Part.Compound(vList)
        return shape

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
