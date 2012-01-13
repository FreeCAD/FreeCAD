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

# Standart libraries
import math
# Import FreeCAD modules
import FreeCAD as App
import FreeCADGui as Gui
# Import Qt libraries
from PyQt4 import QtGui,QtCore
# Import surface module utils
from surfUtils import Paths, Geometry, Math, Translator
# Import tool utils
import Preview

class TaskPanel:
    def __init__(self):
        """ Class constructor, that loads the user interface and set
        minimum data as empty values.
        """
        self.ui = Paths.modulePath() + "/surfConvert/TaskPanel.ui"
        self.objs = []
        self.edges = []
        self.faces = []
        self.surf = None
        self.preview = None

    def accept(self):
        """ Method called when accept button is clicked.
        @return True if all gone right, False otherwise.
        """
        if not self.surf:
            return False
        App.Part.show(self.surf.toShape())
        objs = App.ActiveDocument.Objects
        obj = objs[len(objs)-1]
        obj.Label = 'Surface'
        self.preview.clean()
        return True

    def reject(self):
        """ Method called when cancel button is clicked.
        @return True if all gone right, False otherwise.
        """     
        if self.preview :
            self.preview.clean()
        return True

    def clicked(self, index):
        pass

    def open(self):
        pass

    def needsFullSpace(self):
        return True

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return False

    def helpRequested(self):
        pass

    def setupUi(self):
        """ Setups the user interface taking and storing all controls.
        """
        mw = self.getMainWindow()
        form = mw.findChild(QtGui.QWidget, "TaskPanel")
        form.nU = form.findChild(QtGui.QSpinBox, "nU")
        form.nV = form.findChild(QtGui.QSpinBox, "nV")
        form.deg = form.findChild(QtGui.QSpinBox, "deg")
        self.form = form
        # Initial values
        if self.initValues():
            return True
        self.retranslateUi()
        # Connect Signals and Slots
        QtCore.QObject.connect(form.nU, QtCore.SIGNAL("valueChanged(int)"), self.onNUV)
        QtCore.QObject.connect(form.nV, QtCore.SIGNAL("valueChanged(int)"), self.onNUV)
        QtCore.QObject.connect(form.deg, QtCore.SIGNAL("valueChanged(int)"), self.onNUV)
        return False

    def getMainWindow(self):
        """ returns the main window """
        # using QtGui.qApp.activeWindow() isn't very reliable because if another
        # widget than the mainwindow is active (e.g. a dialog) the wrong widget is
        # returned
        toplevel = QtGui.qApp.topLevelWidgets()
        for i in toplevel:
            if i.metaObject().className() == "Gui::MainWindow":
                return i
        raise Exception("No main window found")
        
    def initValues(self):
        """ Setup initial values. """
        # Objects to work
        self.objs = Gui.Selection.getSelection()
        if not self.objs:
            msg = Translator.translate("4 connected edges and at least 1 surface must be selected (Any object has been selected)")
            App.Console.PrintError(msg)
            return True
        if len(self.objs) < 5:
            msg = Translator.translate("4 connected edges and at least 1 surface must be selected (Less than 5 objects selected)")
            App.Console.PrintError(msg)
            return True
        # Separate edge objects and face objects.
        if self.getEdgesFaces():
            return True
        # Ensure that closed object given
        if not self.isClosed():
            return True
        # Sort edges (as connect chain)
        if self.sortEdges():
            return True
        # First surface draft
        if self.getPoints():
            return True
        if self.createSurface():
            return True
        # On screen data
        self.preview = Preview.Preview(self.edges[0],self.edges[1])
        msg = Translator.translate("Ready to work")
        App.Console.PrintMessage(msg)
        return False

    def retranslateUi(self):
        """ Set user interface locale strings. 
        """
        self.form.setWindowTitle(Translator.translate("Convert to 4 side surface"))
        nUString = Translator.translate("Points at U")
        self.form.findChild(QtGui.QLabel, "nULabel").setText("<font color=#0000ff>" + nUString + "</font>")
        nVString = Translator.translate("Points at V2")
        self.form.findChild(QtGui.QLabel, "nVLabel").setText("<font color=#ff0000>" + nVString + "</font>")
        self.form.findChild(QtGui.QLabel, "degLabel").setText(Translator.translate("Surface degree"))        

    def onNUV(self, value):
        """ Answer to nU points or nV points modification events.
        @param value Changed value.
        """
        if self.getPoints():
            return
        if self.createSurface():
            return
        self.form.deg.setMaximum(self.surf.MaxDegree)

    def getEdgesFaces(self):
        """ Returns two arrays filled with the edges and faces selected.
        @return True if error happens. False otherwise
        """
        self.edges = []
        self.faces = []
        for i in range(0,len(self.objs)):
            objFaces = Geometry.getFaces(self.objs[i])
            if not objFaces:
                objEdges = Geometry.getEdges([self.objs[i]])
                if not objEdges:
                    msg = Translator.translate("4 connected edges and at least 1 surface must be selected (Any edge found)")
                    App.Console.PrintError(msg)
                    return True
                for j in range(0, len(objEdges)):
                    self.edges.append(objEdges[j])
            else:
                for j in range(0, len(objFaces)):
                    self.faces.append(objFaces[j])
        if len(self.edges) != 4:
            msg = Translator.translate("4 connected edges and at least 1 surface must be selected (More/less edges not allowed)")
            App.Console.PrintError(msg)
            return True
        return False

    def isClosed(self):
        """ Returns if the edges objects are a closed curve.
        @return True if a closed curve can be built with edge objects.
        False otherwise
        """
        edges = self.edges
        for i in range(0,len(edges)):
            edge = edges[i]
            vertex1 = edge.Vertexes
            count = [0,0]
            for j in range(0,len(edges)):
                if j== i:
                    continue
                vertex2 = edges[j].Vertexes
                for k in range(0,2):
                    if Math.isSameVertex(vertex1[0],vertex2[k]):
                        count[0] = count[0]+1
                    if Math.isSameVertex(vertex1[1],vertex2[k]):
                        count[1] = count[1]+1
            if count != [1,1]:
                msg = Translator.translate("4 Edges curve must be closed")
                App.Console.PrintError(msg)
                return False
        return True

    def sortEdges(self):
        """ Sort the edges by their connection points. Also gives if
        an edge is ill oriented (thir connection point is the end
        point, not the starting point), with True when is good oriented,
        and False otherwise.
        @return True if error happens. False otherwise
        """
        edges = self.edges
        self.edges = [edges[0]]
        self.direction = [True]
        Vertex = self.edges[0].Vertexes[1]
        for i in range(0,len(edges)-1):
            for j in range(0,len(edges)):
                if self.edges[i] == edges[j]:
                    continue
                Vertexes = edges[j].Vertexes
                if Math.isSameVertex(Vertex,Vertexes[0]):
                    Vertex = Vertexes[1]
                    self.edges.append(edges[j])
                    self.direction.append(True)
                    break
                if Math.isSameVertex(Vertex,Vertexes[1]):
                    Vertex = Vertexes[0]
                    self.edges.append(edges[j])
                    self.direction.append(False)
                    break
        return False

    def getPoints(self):
        """ Get points that will make the surface.
        @return True if error happens. False otherwise.
        """
        if self.divideEdges():
            return True
        if self.samplePoints():
            return True
        if self.getSurfacePoints():
            return True
        while self.redistributePointsU():
            continue
        while self.redistributePointsV():
            continue
        return False
    

    def divideEdges(self):
        """ Get points along the edges.
        @return True if error happens. False otherwise.
        """
        # Get curves from edges
        curves=[]
        for i in range(0,len(self.edges)):
            curves.append(self.edges[i].Curve)
        # Get number of divisions at U,V
        nU = self.form.nU.value()
        nV = self.form.nV.value()
        # Get points at U direction (length parameter will used)
        vertexes = self.edges[0].Vertexes
        u    = curves[0].parameter(App.Base.Vector(vertexes[0].X, vertexes[0].Y, vertexes[0].Z))
        uEnd = curves[0].parameter(App.Base.Vector(vertexes[1].X, vertexes[1].Y, vertexes[1].Z))
        du   = (uEnd - u) / (nU-1.0)
        Points0=[]
        for i in range(0, nU):
            knots  = curves[0].KnotSequence
            point0 = curves[0].value(u)
            Points0.append(point0)
            u = u + du
        vertexes = self.edges[2].Vertexes
        u    = curves[2].parameter(App.Base.Vector(vertexes[1].X, vertexes[1].Y, vertexes[1].Z))
        uEnd = curves[2].parameter(App.Base.Vector(vertexes[0].X, vertexes[0].Y, vertexes[0].Z))
        du   = (uEnd - u) / (nU-1.0)
        Points2=[]
        for i in range(0, nU):
            knots  = curves[2].KnotSequence
            point2 = curves[2].value(u)
            Points2.append(point2)
            u = u + du
        # Get points at V direction (length parameter will used)
        vertexes = self.edges[1].Vertexes
        v    = curves[1].parameter(App.Base.Vector(vertexes[0].X, vertexes[0].Y, vertexes[0].Z))
        vEnd = curves[1].parameter(App.Base.Vector(vertexes[1].X, vertexes[1].Y, vertexes[1].Z))
        dv   = (vEnd - v) / (nV-1.0)
        Points1=[]
        for i in range(0, nV):
            knots  = curves[1].KnotSequence
            point1 = curves[1].value(v)
            Points1.append(point1)
            v = v + dv
        vertexes = self.edges[3].Vertexes
        v     = curves[3].parameter(App.Base.Vector(vertexes[1].X, vertexes[1].Y, vertexes[1].Z))
        vEnd  = curves[3].parameter(App.Base.Vector(vertexes[0].X, vertexes[0].Y, vertexes[0].Z))
        dv    = (vEnd - v) / (nV-1.0)
        Points3=[]
        for i in range(0, nV):
            knots  = curves[3].KnotSequence
            point3 = curves[3].value(v)
            Points3.append(point3)
            v = v + dv
        # Store points arrays
        self.EdgePoints=[Points0,Points1,Points2,Points3]
        # Ensure that is good oriented
        for i in range(0,4):
            if not self.direction[i]:
                self.EdgePoints[i].reverse()
        return False

    def samplePoints(self):
        """ Builds a set of points between the edges points.
        @return True if error happens. False otherwise.
        """
        # We will advance in V direction, getting point columns, so two
        # points columns are knowed from U starting and end curves
        startU = self.EdgePoints[0]
        endU   = self.EdgePoints[2]
        endV   = self.EdgePoints[1]
        startV = self.EdgePoints[3]
        self.sample = [startU]
        # Take two guides froms the starting and end U curves. All
        # sample points will be reprojects with this guides
        Guide0  = startU[len(startU)-1] - startU[0]
        Length0 = Guide0.Length
        Guide0.normalize()
        Guide1  = endU[len(endU)-1] - endU[0]
        Length1 = Guide1.Length
        Guide1.normalize()
        # Loop over columns to get
        for i in range(1,len(startV)-1):
            # Get the guide
            Guide  = endV[i] - startV[i]
            Length = Guide.Length
            Guide.normalize()
            # Loop over points of the column
            points = []
            points.append(startV[i])
            for j in range(1,len(startU)-1):
                # Get deviation at starting U
                v0 = (startU[j] - startU[0]).multiply(1.0 / Length0) - Guide0
                # Get deviation at ending U
                v1 = (endU[j] - endU[0]).multiply(1.0 / Length1) - Guide1
                # Get factor
                factor = float(i) / len(startV)
                # Interpolate
                v = (v0.multiply(1.0-factor) + v1.multiply(factor) + Guide).multiply(Length)
                points.append(startV[i]+v)
            points.append(endV[i])
            self.sample.append(points)
        # Append las points column (end U curve)
        self.sample.append(endU)
        return False

    def getSurfacePoints(self):
        """ Get the points of the surface.
        @return True if error happens. False otherwise.
        """
        surf = self.faces[0].Surface
        self.uv=[]
        self.points=[]
        self.surfacesID=[]
        for j in range(0,len(self.sample)):
            points=[]
            uv=[]
            surfacesID=[]
            for k in range(0,len(self.sample[j])):
                UV = surf.parameter(self.sample[j][k])
                umax = surf.UKnotSequence[len(surf.UKnotSequence)-1]
                umin = surf.UKnotSequence[0]
                vmax = surf.VKnotSequence[len(surf.VKnotSequence)-1]
                vmin = surf.VKnotSequence[0]
                uv.append([UV[0],UV[1]])
                uv[k][0] = max(min(uv[k][0],umax), umin)
                uv[k][1] = max(min(uv[k][1],vmax), vmin)
                point = surf.value(uv[k][0], uv[k][1])
                points.append(point)
                surfacesID.append(0)
            self.uv.append(uv)
            self.points.append(points)
            self.surfacesID.append(surfacesID)

        for i in range(1,len(self.faces)):
            surf = self.faces[i].Surface
            for j in range(0,len(self.sample)):
                for k in range(0,len(self.sample[j])):
                    UV = surf.parameter(self.sample[j][k])
                    uv = [UV[0],UV[1]]
                    UV = uv[:]
                    umax = surf.UKnotSequence[len(surf.UKnotSequence)-1]
                    umin = surf.UKnotSequence[0]
                    vmax = surf.VKnotSequence[len(surf.VKnotSequence)-1]
                    vmin = surf.VKnotSequence[0]
                    UV[0] = uv[0]/(surf.UKnotSequence[len(surf.UKnotSequence)-1])
                    UV[1] = uv[1]/(surf.VKnotSequence[len(surf.VKnotSequence)-1])
                    ru=abs(UV[0]-0.5)
                    rv=abs(UV[1]-0.5)
                    surf2 = self.faces[self.surfacesID[j][k]].Surface
                    UV2 = surf2.parameter(self.sample[j][k])
                    uv2 = [UV2[0],UV2[1]]
                    UV2 = uv2[:]
                    umax2 = surf2.UKnotSequence[len(surf2.UKnotSequence)-1]
                    umin2 = surf2.UKnotSequence[0]
                    vmax2 = surf2.VKnotSequence[len(surf2.VKnotSequence)-1]
                    vmin2 = surf2.VKnotSequence[0]
                    UV2[0] = uv2[0]/(surf2.UKnotSequence[len(surf2.UKnotSequence)-1])
                    UV2[1] = uv2[1]/(surf2.VKnotSequence[len(surf2.VKnotSequence)-1])
                    ru2=abs(UV2[0]-0.5)
                    rv2=abs(UV2[1]-0.5)
                    point=surf.value(UV[0],UV[1])
                    point2=surf2.value(UV2[0],UV2[1])
                    dif= math.sqrt((point[0]-self.sample[j][k][0])**2+(point[1]-self.sample[j][k][1])**2+(point[2]-self.sample[j][k][2])**2)
                    dif2= math.sqrt((point2[0]-self.sample[j][k][0])**2+(point2[1]-self.sample[j][k][1])**2+(point2[2]-self.sample[j][k][2])**2)
                    nU = len(self.points)
                    nV = len(self.points[j])
                    deltaU = 1/(2*nU)
                    deltaV = 1/(2*nV)
                    if ((ru < deltaU) and (rv < deltaV)):
                        uv[0] = max(min(uv[0],umax), umin)
                        uv[1] = max(min(uv[1],vmax), vmin)
                        self.uv[j][k]=uv
                        point = surf.value(uv[0], uv[1])
                        self.points[j][k]=point
                        self.surfacesID[j][k] = i
                    elif (dif2 > dif):
                        uv[0] = max(min(uv[0],umax), umin)
                        uv[1] = max(min(uv[1],vmax), vmin)
                        self.uv[j][k]=uv
                        point = surf.value(uv[0], uv[1])
                        self.points[j][k]=point
                        self.surfacesID[j][k] = i

        return False

    def redistributePointsU(self):
        """ Redistributes the points of the surface (U direction).
        @return False if all points are right placed. True otherwise.
        """
        #Redistribute columns
        for i in range(0,len(self.points)):
            sumdif=0.0
            difs=[]
            for j in range(1,len(self.points[i])):
                vdif=self.points[i][j]-self.points[i][j-1]
                dif=vdif.Length
                sumdif=sumdif+dif
                difs.append(dif)
            for k in range(0,len(difs)):
                if self.surfacesID[i][k] != self.surfacesID[i][k+1]:
                    if difs[k]>=(2*sumdif/len(difs)):
                        self.points[i][k][0]=self.points[i][k][0]+0.33*(self.points[i][k+1][0]-self.points[i][k][0])
                        self.points[i][k][1]=self.points[i][k][1]+0.33*(self.points[i][k+1][1]-self.points[i][k][1])
                        self.points[i][k][2]=self.points[i][k][2]+0.33*(self.points[i][k+1][2]-self.points[i][k][2])
                        self.points[i][k+1][0]=self.points[i][k+1][0]-0.33*(self.points[i][k+1][0]-self.points[i][k][0])
                        self.points[i][k+1][1]=self.points[i][k+1][1]-0.33*(self.points[i][k+1][1]-self.points[i][k][1])
                        self.points[i][k+1][2]=self.points[i][k+1][2]-0.33*(self.points[i][k+1][2]-self.points[i][k][2])
                        surf1 = self.faces[self.surfacesID[i][k]].Surface
                        surf2 = self.faces[self.surfacesID[i][k+1]].Surface
                        uv = surf1.parameter(self.points[i][k])
                        self.uv[i][k] = [uv[0], uv[1]]
                        uv = surf2.parameter(self.points[i][k+1]) 
                        self.uv[i][k+1] = [uv[0], uv[1]]
                        self.points[i][k]=surf1.value(self.uv[i][k][0],self.uv[i][k][1])
                        self.points[i][k+1]=surf2.value(self.uv[i][k+1][0],self.uv[i][k+1][1])
                        return True
                        break
                    continue
                if difs[k]>=(2*sumdif/len(difs)):
                    self.uv[i][k][0]=self.uv[i][k][0]+0.33*(self.uv[i][k+1][0]-self.uv[i][k][0])
                    self.uv[i][k][1]=self.uv[i][k][1]+0.33*(self.uv[i][k+1][1]-self.uv[i][k][1])
                    self.uv[i][k+1][0]=self.uv[i][k+1][0]-0.33*(self.uv[i][k+1][0]-self.uv[i][k][0])
                    self.uv[i][k+1][1]=self.uv[i][k+1][1]-0.33*(self.uv[i][k+1][1]-self.uv[i][k][1])
                    surf = self.faces[self.surfacesID[i][k]].Surface
                    self.points[i][k] = surf.value(self.uv[i][k][0], self.uv[i][k][1])
                    self.points[i][k+1] = surf.value(self.uv[i][k+1][0], self.uv[i][k+1][1])
                    return True
                    break
        return False
    def redistributePointsV(self):
        """ Redistributes the points of the surface (V direction).
        @return False if all points are right placed. True otherwise.
        """
        #Redistribute files
        for i in range(0,len(self.points[0])):
            sumdif=0
            difs=[]
            for j in range(1,len(self.points)):
                vdif=self.points[j][i]-self.points[j-1][i]
                dif=vdif.Length
                sumdif=sumdif+dif
                difs.append(dif)
            for k in range(0,len(difs)):
                if self.surfacesID[k][i] != self.surfacesID[k+1][i]:
                    if difs[k]>=(2*sumdif/len(difs)):
                        self.points[k][i][0]=self.points[k][i][0]+0.33*(self.points[k+1][i][0]-self.points[k][i][0])
                        self.points[k][i][1]=self.points[k][i][1]+0.33*(self.points[k+1][i][1]-self.points[k][i][1])
                        self.points[k][i][2]=self.points[k][i][2]+0.33*(self.points[k+1][k+1][2]-self.points[k][i][2])
                        self.points[k+1][i][0]=self.points[k+1][i][0]-0.33*(self.points[k+1][i][0]-self.points[k][i][0])
                        self.points[k+1][i][1]=self.points[k+1][i][1]-0.33*(self.points[k+1][i][1]-self.points[k][i][1])
                        self.points[k+1][i][2]=self.points[k+1][i][2]-0.33*(self.points[k+1][i][2]-self.points[k][i][2])
                        surf1 = self.faces[self.surfacesID[k][i]].Surface
                        surf2 = self.faces[self.surfacesID[k+1][i]].Surface
                        uv = surf1.parameter(self.points[k][i])
                        self.uv[k][i] = [uv[0], uv[1]]
                        uv = surf2.parameter(self.points[k+1][i]) 
                        self.uv[k+1][i] = [uv[0], uv[1]]
                        self.points[k][i]=surf1.value(self.uv[k][i][0],self.uv[k][i][1])
                        self.points[k+1][i]=surf2.value(self.uv[k+1][i][0],self.uv[k+1][i][1])
                        return True
                        break
                    continue
                if difs[k]>=(2*sumdif/len(difs)):       
                    self.uv[k][i][0]=self.uv[k][i][0]+0.33*(self.uv[k+1][i][0]-self.uv[k][i][0])
                    self.uv[k][i][1]=self.uv[k][i][1]+0.33*(self.uv[k+1][i][1]-self.uv[k][i][1])
                    self.uv[k+1][i][0]=self.uv[k+1][i][0]-0.33*(self.uv[k+1][i][0]-self.uv[k][i][0])
                    self.uv[k+1][i][1]=self.uv[k+1][i][1]-0.33*(self.uv[k+1][i][1]-self.uv[k][i][1])
                    surf = self.faces[self.surfacesID[k][i]].Surface
                    self.points[k][i] = surf.value(self.uv[k][i][0], self.uv[k][i][1])
                    self.points[k+1][i] = surf.value(self.uv[k+1][i][0], self.uv[k+1][i][1])
                    return True
                    break
        return False
        
    def createSurface(self):
        """ Generates the BSpline surface.
        @return False if all gone right. True otherwise.
        """
        # Create a (1 x 1) plane with (nU x nV) knots 
        self.surf = App.Part.BSplineSurface()
        nU = len(self.points)
        nV = len(self.points[0])
        for i in range(1,nU-1):
            u = i / float(nU-1)
            self.surf.insertUKnot(u,i,0.000001)
        for i in range(1,nV-1):
            v = i / float(nV-1)
            self.surf.insertVKnot(v,i,0.000001)
        # Reposition points of surface
        for i in range(0,nU):
            for j in range(0,nV):
                u = i / float(nU-1)
                v = j / float(nV-1)
                point = self.points[i][j]
                self.surf.movePoint(u,v,point,i+1,i+1,j+1,j+1)
        # Reaconditionate surface
        self.surf.exchangeUV()
        deg = self.form.deg.value()
        if (deg > 1) and (deg < self.surf.MaxDegree):
            self.surf.increaseDegree(deg,deg)
        return False

def createTask():
    """ Create a task panel.
    @return Task panel.
    """
    panel = TaskPanel()
    Gui.Control.showDialog(panel)
    if panel.setupUi():
        Gui.Control.closeDialog(panel)
        return None
    return panel
