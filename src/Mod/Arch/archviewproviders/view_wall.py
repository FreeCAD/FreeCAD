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

from PySide import QtCore,QtGui
from pivy import coin

import FreeCAD as App

import Part


class ViewProviderWall(object):
    
    def __init__(self,vobj=None):
        self.child_node = None
        self.mode_node = None
        self.shape_node = None
        self.shape_mode = None
        if vobj:
            vobj.Proxy = self
            self.attach(vobj)
        else:
            self.ViewObject = None


    def attach(self, vobj):
        vobj.addExtension('Gui::ViewProviderGeoFeatureGroupExtensionPython', None)
        self.ViewObject = vobj
        self.setupShapeGroup()


    def setupShapeGroup(self):
        vobj = self.ViewObject
        if getattr(self, 'shape_mode', None) or \
                vobj.SwitchNode.getNumChildren() < 2:
            return
        self.child_node = vobj.SwitchNode.getChild(0)
        self.shape_node = vobj.SwitchNode.getChild(1)
        self.mode_node = coin.SoSeparator()
        self.mode_node.addChild(self.child_node)
        self.mode_node.addChild(self.shape_node)
        self.shape_mode = vobj.SwitchNode.getNumChildren()
        # TODO: overwrite existing display modes with the new ones so
        #       when DisplayModes are set generally, the objects are
        #       correctly displayed
        vobj.addDisplayMode(self.mode_node, 'ShapeGroup')
        try:
            vobj.SwitchNode.defaultChild = self.shape_mode
        except Exception:
            pass


    def getDisplayModes(self, _vobj):
        return ["ShapeGroup"]


    def getDefaultDisplayMode(self):
        return "ShapeGroup"


    def getDetailPath(self,subname,path,append):
        if not subname or not getattr(self, 'shape_mode', None):
            raise NotImplementedError
        subs = Part.splitSubname(subname)
        objs = subs[0].split('.')

        vobj = self.ViewObject
        mode = vobj.SwitchNode.whichChild.getValue()
        if mode < 0:
            raise NotImplementedError

        if append:
            path.append(vobj.RootNode)
            path.append(vobj.SwitchNode)
            path.append(vobj.SwitchNode.getChild(mode))
            if mode == self.shape_mode:
                if not objs[0]:
                    path.append(self.shape_node)
                else:
                    path.append(self.child_node)
            if not objs[0]:
                return vobj.getDetailPath(subname,path,False)
        for child in vobj.claimChildren():
            if child.Name == objs[0]:
                sub = Part.joinSubname('.'.join(objs[1:]),subs[1],subs[2])
                return child.ViewObject.getDetailPath(sub,path,True)


    def getElementPicked(self, pp):
        if not getattr(self, 'shape_mode', None):
            raise NotImplementedError
        vobj = self.ViewObject
        path = pp.getPath()
        if path.findNode(self.child_node) < 0:
            raise NotImplementedError
        for child in vobj.claimChildren():
            if path.findNode(child.ViewObject.RootNode) < 0:
                continue
            return child.Name + '.' + child.ViewObject.getElementPicked(pp)


    def onChanged(self, _vobj,prop):
        if prop == 'DisplayMode':
            self.setupShapeGroup()


    def __getstate__(self):
        return None


    def __setstate__(self, _state):
        return None


    def onDelete(self, vobj, subelements): # subelements is a tuple of strings
        """
        Activated when object is deleted
        """
        # ask if the user is sure and wants to delete contained objects
        msgBox = QtGui.QMessageBox()
        msgBox.setText("Deleting wall object " + vobj.Object.Label + ".")
        msgBox.setInformativeText("Do you want to delete also contained objects?")
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

        vobj.Object.Proxy.remove_linked_walls_references(vobj.Object)

        if delete_children:
            for o in vobj.Object.Group:
                App.ActiveDocument.removeObject(o.Name)

        # the object will be deleted
        return True
    

    def setupContextMenu(self, vobj, menu):
        """
        Setup context menu actions:
        - Flip wall (this is not implemented yet)
        """
        action1 = QtGui.QAction("Flip wall", menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),lambda f=vobj.Object.Proxy.flip_wall, arg=vobj.Object:f(arg))
        menu.addAction(action1)
