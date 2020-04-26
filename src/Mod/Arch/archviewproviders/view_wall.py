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

import FreeCAD as App
from PySide import QtCore,QtGui


class ViewProviderWall(object):

    def __init__(self,vobj=None):

        if vobj:
            vobj.Proxy = self
            self.attach(vobj)
        else:
            self.ViewObject = None


    def updateData(self, obj, prop):

        return


    def attach(self,vobj):

        vobj.addExtension('Gui::ViewProviderOriginGroupExtensionPython', None)
        self.ViewObject = vobj
        print("running  " + vobj.Object.Name + " ViewObject attach() method\n")


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


    def __getstate__(self):
        """ describe """        
        return None


    def __setstate__(self, _state):
        """ describe """        
        return None


    def getDefaultDisplayMode(self):
        """
        Return the name of the default display mode. 
        It must be defined in getDisplayModes.
        """
        return "Flat Lines"


    def setupContextMenu(self, vobj, menu):
        """
        Setup context menu actions:
        - Toggle display mode
        - Flip wall (this is not implemented yet)
        """
        action1 = QtGui.QAction("Toggle display mode", menu)
        QtCore.QObject.connect(action1,QtCore.SIGNAL("triggered()"),lambda f=self.toggle_display_mode, arg=vobj:f(arg))
        menu.addAction(action1)

        action2 = QtGui.QAction("Flip wall", menu)
        QtCore.QObject.connect(action2,QtCore.SIGNAL("triggered()"),lambda f=vobj.Object.Proxy.flip_wall, arg=vobj.Object:f(arg))
        menu.addAction(action2)


    def toggle_display_mode(self, vobj):
        """
        Switches the visualization between the wall object as a compound
        and its grouped children
        TODO: Find a more appropriate name for the end user
        """
        if self.ViewObject.DisplayMode == "Group":
            self.ViewObject.DisplayMode = "Flat Lines"
        elif self.ViewObject.DisplayMode == "Flat Lines":
            self.ViewObject.DisplayMode = "Group"
