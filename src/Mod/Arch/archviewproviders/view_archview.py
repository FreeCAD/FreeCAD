#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2020 Carlo Pavan                                        *
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
"""Provide the object code for Arch Wall viewprovider."""
## @package view_wall
# \ingroup ARCH
# \brief Provide the viewprovider code for Arch Wall.

from PySide import QtCore, QtGui
from PySide.QtCore import QT_TRANSLATE_NOOP
from pivy import coin

import FreeCAD as App
import FreeCADGui as Gui

import Part
import Draft

import DraftVecUtils


class ViewProviderArchView(object):
    
    def __init__(self,vobj=None):
        if vobj:
            vobj.Proxy = self
            self.attach(vobj)
        else:
            self.ViewObject = None


    def getIcon(self):
        """Return the path to the appropriate icon.
        """ 
        return ":/icons/Arch_SectionPlane_Tree.svg"


    def set_properties(self, vobj):
        pl = vobj.PropertiesList
        d = 0
        if "DisplaySize" in pl:
            d = vobj.DisplaySize.Value
            vobj.removeProperty("DisplaySize")
        if not "DisplayLength" in pl:
            vobj.addProperty("App::PropertyLength","DisplayLength","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The display length of this section plane"))
            if d:
                vobj.DisplayLength = d
            else:
                vobj.DisplayLength = 1000
        if not "DisplayHeight" in pl:
            vobj.addProperty("App::PropertyLength","DisplayHeight","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The display height of this section plane"))
            if d:
                vobj.DisplayHeight = d
            else:
                vobj.DisplayHeight = 1000
        if not "ArrowSize" in pl:
            vobj.addProperty("App::PropertyLength","ArrowSize","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The size of the arrows of this section plane"))
            vobj.ArrowSize = 50
        if not "Transparency" in pl:
            vobj.addProperty("App::PropertyPercent","Transparency","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The transparency of this object"))
            vobj.Transparency = 85
        if not "LineWidth" in pl:
            vobj.addProperty("App::PropertyFloat","LineWidth","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The line width of this object"))
            vobj.LineWidth = 1
        if not "CutDistance" in pl:
            vobj.addProperty("App::PropertyLength","CutDistance","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Show the cut in the 3D view"))
        if not "LineColor" in pl:
            vobj.addProperty("App::PropertyColor","LineColor","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The color of this object"))
            #vobj.LineColor = ArchCommands.getDefaultColor("Helpers")
        if not "CutView" in pl:
            vobj.addProperty("App::PropertyBool","CutView","SectionPlane",QT_TRANSLATE_NOOP("App::Property","Show the cut in the 3D view"))
        if not "CutMargin" in pl:
            vobj.addProperty("App::PropertyLength","CutMargin","SectionPlane",QT_TRANSLATE_NOOP("App::Property","The distance between the cut plane and the actual view cut (keep this a very small value but not zero)"))
            vobj.CutMargin = 1
        

    def attach(self, vobj):
        vobj.addExtension('Gui::ViewProviderGeoFeatureGroupExtensionPython', None)
        self.ViewObject = vobj
        self.set_properties(vobj)
        
        self.setup_default_display_mode(vobj)

        self.onChanged(vobj,"DisplayLength")
        self.onChanged(vobj,"LineColor")
        self.onChanged(vobj,"Transparency")
        self.onChanged(vobj,"CutView")

        self.is_active = False

    def updateData(self, obj, prop):
        """
        This method is called when an Object property changes.
        """
        if prop in ["Placement"]:
            self.onChanged(obj.ViewObject,"DisplayLength")
            self.onChanged(obj.ViewObject,"CutView")
        return

    def onChanged(self, vobj, prop):
        """
        This method is called when a ViewObject property changes.
        """
        if prop == "LineColor":
            if hasattr(vobj,"LineColor"):
                l = vobj.LineColor
                self.mat1.diffuseColor.setValue([l[0],l[1],l[2]])
                self.mat2.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "Transparency":
            if hasattr(vobj,"Transparency"):
                self.mat2.transparency.setValue(vobj.Transparency/100.0)
        elif prop in ["DisplayLength","DisplayHeight","ArrowSize"]:
            self.setup_sectionplane_marker(vobj)
        elif prop == "LineWidth":
            self.drawstyle.lineWidth = vobj.LineWidth
        elif prop in ["CutView","CutMargin"]:
            if hasattr(vobj,"CutView") and Gui.ActiveDocument.ActiveView:
                if vobj.CutView:
                    self.setup_clipping_plane(vobj)
                else:
                    self.remove_clipping_plane(vobj)
        return

    def onDelete(self, vobj, subelements): # subelements is a tuple of strings
        """
        Activated when object is deleted.

        Before deleting:
        Set CutView to False.
        If subobjects are present, ask for deleting them.

        Return
        ------
        bool : True the object will be deleted
               False the object will not be deleted
        """
        # ask if the user is sure and wants to delete contained objects
        if not vobj.Object.Group:
            self.set_cutview(vobj, False)
            return True

        msgBox = QtGui.QMessageBox()
        msgBox.setText("Deleting Arch View object " + vobj.Object.Label + ".")
        msgBox.setInformativeText("Do you want to delete also grouped objects?")
        msgBox.setStandardButtons(QtGui.QMessageBox.Yes | QtGui.QMessageBox.No | QtGui.QMessageBox.Cancel)
        msgBox.setDefaultButton(QtGui.QMessageBox.Yes)
        ret = msgBox.exec_()

        if ret == QtGui.QMessageBox.Yes:
            delete_children = True
        elif ret == QtGui.QMessageBox.No:
            delete_children = False
        elif ret == QtGui.QMessageBox.Cancel:
            # the object won't be deleted
            return False
        else:
            # the object won't be deleted
            return False

        if delete_children:
            for o in vobj.Object.Group:
                App.ActiveDocument.removeObject(o.Name)

        self.set_cutview(vobj, False)
        # the object will be deleted
        return True
    
    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode

    def __getstate__(self):
        return None

    def __setstate__(self, _state):
        return None

    def doubleClicked(self,vobj):
        self.toggle_activate(vobj)
        '''if (not hasattr(vobj,"DoubleClickActivates")) or vobj.DoubleClickActivates:
            FreeCADGui.Selection.clearSelection()''' # don't know what this is for
        return True # why?

    def setupContextMenu(self, vobj, menu):
        """CONTEXT MENU setup"""
        # toggle active
        action1 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"),"Toggle Activate", menu)
        action1.triggered.connect(lambda f=self.toggle_activate, arg=vobj:f(arg))
        menu.addAction(action1)

        # toggle cutview
        action2 = QtGui.QAction(QtGui.QIcon(":/icons/Draft_Edit.svg"),"Toggle Cutview", menu)
        action2.triggered.connect(lambda f=self.toggle_cutview, arg=vobj:f(arg))
        menu.addAction(action2)


    # Section plane marker methods ++++++++++++++++++++++++++++++++++++++++++

    def setup_default_display_mode(self, vobj):
        self.clip = None
        self.mat1 = coin.SoMaterial()
        self.mat2 = coin.SoMaterial()
        self.fcoords = coin.SoCoordinate3()
        #fs = coin.SoType.fromName("SoBrepFaceSet").createInstance() # this causes a FreeCAD freeze for me
        fs = coin.SoIndexedFaceSet()
        fs.coordIndex.setValues(0,7,[0,1,2,-1,0,2,3])
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES
        self.lcoords = coin.SoCoordinate3()
        ls = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        ls.coordIndex.setValues(0,57,[0,1,-1,2,3,4,5,-1,6,7,8,9,-1,10,11,-1,12,13,14,15,-1,16,17,18,19,-1,20,21,-1,22,23,24,25,-1,26,27,28,29,-1,30,31,-1,32,33,34,35,-1,36,37,38,39,-1,40,41,42,43,44])
        sep = coin.SoSeparator()
        psep = coin.SoSeparator()
        fsep = coin.SoSeparator()
        fsep.addChild(self.mat2)
        fsep.addChild(self.fcoords)
        fsep.addChild(fs)
        psep.addChild(self.mat1)
        psep.addChild(self.drawstyle)
        psep.addChild(self.lcoords)
        psep.addChild(ls)
        sep.addChild(fsep)
        sep.addChild(psep)
        vobj.addDisplayMode(sep,"Default")

    def setup_sectionplane_marker(self, vobj):
        if hasattr(vobj,"DisplayLength") and hasattr(vobj,"DisplayHeight"):
            ld = vobj.DisplayLength.Value/2
            hd = vobj.DisplayHeight.Value/2
        elif hasattr(vobj,"DisplaySize"):
            # old objects
            ld = vobj.DisplaySize.Value/2
            hd = vobj.DisplaySize.Value/2
        else:
            ld = 1
            hd = 1
        verts = []
        fverts = []
        for v in [[-ld,-hd],[ld,-hd],[ld,hd],[-ld,hd]]:
            if hasattr(vobj,"ArrowSize"):
                l1 = vobj.ArrowSize.Value if vobj.ArrowSize.Value > 0 else 0.1
            else:
                l1 = 0.1
            l2 = l1/3
            p1 = App.Vector(v[0],v[1],0)
            p2 = App.Vector(v[0],v[1],-l1)
            p3 = App.Vector(v[0]-l2,v[1],-l1+l2)
            p4 = App.Vector(v[0]+l2,v[1],-l1+l2)
            p5 = App.Vector(v[0],v[1]-l2,-l1+l2)
            p6 = App.Vector(v[0],v[1]+l2,-l1+l2)
            verts.extend([[p1.x,p1.y,p1.z],[p2.x,p2.y,p2.z]])
            fverts.append([p1.x,p1.y,p1.z])
            verts.extend([[p2.x,p2.y,p2.z],[p3.x,p3.y,p3.z],[p4.x,p4.y,p4.z],[p2.x,p2.y,p2.z]])
            verts.extend([[p2.x,p2.y,p2.z],[p5.x,p5.y,p5.z],[p6.x,p6.y,p6.z],[p2.x,p2.y,p2.z]])
        verts.extend(fverts+[fverts[0]])
        if hasattr(self, "lcoords") and self.lcoords: # guarded when changed object type
            self.lcoords.point.setValues(verts)
            self.fcoords.point.setValues(fverts)


    # CutView methods +++++++++++++++++++++++++++++++++++++++++++++++++++++++

    def set_cutview(self, vobj, mode=True):
        """ set_cutview(vobj, mode=True)
        
        Setter for CutView Property.
        """
        if mode == True:
            vobj.CutView = True
        else: 
            vobj.CutView = False        

    def toggle_cutview(self, vobj):
        """ toggle_cutview(vobj)
        
        Toggler for CutView Property.
        """
        if vobj.CutView:
            self.set_cutview(vobj, False)
        else:
            self.set_cutview(vobj, True)

    def setup_clipping_plane(self, vobj):
        """ setup_clipping_plane(vobj)
        
        Set-up the clipping plane of the scenegraph.
        This method is called when the property CutView or the object Placement changes.
        """
        sg = Gui.ActiveDocument.ActiveView.getSceneGraph()
        if self.clip:
            sg.removeChild(self.clip)
            self.clip = None
        for o in Draft.getGroupContents(vobj.Object.Objects,walls=True):
            if hasattr(o.ViewObject,"Lighting"):
                o.ViewObject.Lighting = "One side"
        self.clip = coin.SoClipPlane()
        self.clip.on.setValue(True)
        norm = vobj.Object.Proxy.getNormal(vobj.Object)
        mp = vobj.Object.Shape.CenterOfMass
        mp = DraftVecUtils.project(mp,norm)
        dist = mp.Length #- 0.1 # to not clip exactly on the section object
        norm = norm.negative()
        marg = 1
        if hasattr(vobj,"CutMargin"):
            marg = vobj.CutMargin.Value
        if mp.getAngle(norm) > 1:
            dist += marg
            dist = -dist
        else:
            dist -= marg
        plane = coin.SbPlane(coin.SbVec3f(norm.x,norm.y,norm.z),dist)
        self.clip.plane.setValue(plane)
        sg.insertChild(self.clip,0)

    def remove_clipping_plane(self, vobj):
        """ setup_clipping_plane(vobj)
        
        Set-up the clipping plane of the scenegraph.
        This method is called when the property CutView or the object Placement changes.
        """
        sg = Gui.ActiveDocument.ActiveView.getSceneGraph()
        if hasattr(self, "clip") and self.clip: # added guard when changed object type
            sg.removeChild(self.clip)
            self.clip = None


    # Activate arch view methods ++++++++++++++++++++++++++++++++++++++++++++

    def toggle_activate(self, vobj):
        """ Not implemented yet
        """
        if self.is_active:
            self.deactivate(vobj)
        else:
            self.activate(vobj)

    def activate(self, vobj):
        """ Not implemented yet
        """
        print("Activating View" + vobj.Object.Name)
        vobj.CutView = True
        if hasattr(App, "DraftWorkingPlane") and hasattr(Gui, "Snapper"):
            App.DraftWorkingPlane.alignToFace(vobj.Object.Shape.Faces[0])
            Gui.Snapper.setGrid()
        self.align_camera(vobj)
        vobj.DisplayMode = "Group"
        '''
        if FreeCADGui.ActiveDocument.ActiveView.getActiveObject("Arch") == vobj.Object:
            FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",None)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane(restore=True)
        else:
            if (not hasattr(vobj,"DoubleClickActivates")) or vobj.DoubleClickActivates:
                FreeCADGui.ActiveDocument.ActiveView.setActiveObject("Arch",vobj.Object)
            if vobj.SetWorkingPlane:
                self.setWorkingPlane()'''
        self.is_active = True
        return
    
    def deactivate(self,vobj):
        """ Not implemented yet
        """
        print("Deactivating View" + vobj.Object.Name)
        vobj.CutView = False
        vobj.DisplayMode = "Default"
        self.is_active = False
        return
    
    def align_camera(self, vobj):
        
        dir = vobj.Object.Proxy.getNormal(vobj.Object)
        cam = Gui.ActiveDocument.ActiveView.getCameraNode()

        if dir.z == 1 :
            rot = pointAt(dir, App.Vector(0.0,1.0,0.0))
        elif dir.z == -1 :
            rot = pointAt(dir, App.Vector(0.0,1.0,0.0))
        else :
            rot = pointAt(dir, App.Vector(0.0,0.0,1.0))

        cam.orientation.setValue(rot.Q)

    def writeCamera(self, vobj):
        """ Not implemented yet
        """
        vobj.CutView = False
        if hasattr(self,"Object"):
            n = Gui.ActiveDocument.ActiveView.getCameraNode()
            App.Console.PrintMessage(QT_TRANSLATE_NOOP("Draft","Writing camera position")+"\n")
            cdata = list(n.position.getValue().getValue())
            cdata.extend(list(n.orientation.getValue().getValue()))
            cdata.append(n.nearDistance.getValue())
            cdata.append(n.farDistance.getValue())
            cdata.append(n.aspectRatio.getValue())
            cdata.append(n.focalDistance.getValue())
            if isinstance(n,coin.SoOrthographicCamera):
                cdata.append(n.height.getValue())
                cdata.append(0.0) # orthograhic camera
            elif isinstance(n,coin.SoPerspectiveCamera):
                cdata.append(n.heightAngle.getValue())
                cdata.append(1.0) # perspective camera
            self.Object.ViewObject.ViewData = cdata


def pointAt(normal, up):
    z = normal
    y = up
    x = y.cross(z)
    y = z.cross(x)

    rot = App.Matrix()
    rot.A11 = x.x
    rot.A21 = x.y
    rot.A31 = x.z

    rot.A12 = y.x
    rot.A22 = y.y
    rot.A32 = y.z

    rot.A13 = z.x
    rot.A23 = z.y
    rot.A33 = z.z

    return App.Placement(rot).Rotation
