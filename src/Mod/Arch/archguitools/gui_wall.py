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
"""Provide the Arch_Wall command."""
## @package gui_wall
# \ingroup ARCH
# \brief Provide the Arch_Wall command used in Arch to create an Arch Wall.

import FreeCAD as App
import FreeCADGui as Gui
import Draft
import archmake.make_wall as make_wall
from PySide import QtCore,QtGui

# ---------------------------------------------------------------------------
# this is just a very rough implementation to test the objects
# ---------------------------------------------------------------------------

class Arch_Wall:

    "the Arch Wall command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Wall',
                'MenuText': "Wall_EXPERIMENTAL",
                'Accel': "W, A",
                'ToolTip': "EXPERIMENTAL\nCreates a wall object from scratch or from a selected object (wire, face or solid)"}

    def IsActive(self):

        return not App.ActiveDocument is None

    def Activated(self):
        self.first_end = None
        self.last_end = None
        self.points = []

        Gui.Snapper.getPoint(callback=self.getPoint,
                             extradlg=self.taskbox(),
                             title="First point of wall"+":")


    def taskbox(self):
        "sets up a taskbox widget"

        w = QtGui.QWidget()
        ui = Gui.UiLoader()
        w.setWindowTitle("Wall options")
        grid = QtGui.QGridLayout(w)

        matCombo = QtGui.QComboBox()
        matCombo.addItem("Wall Presets...")
        matCombo.setToolTip("This list shows all the MultiMaterials objects of this document. Create some to define wall types.")
        self.multimats = []
        self.MultiMat = None
        for o in App.ActiveDocument.Objects:
            if Draft.getType(o) == "MultiMaterial":
                self.multimats.append(o)
                matCombo.addItem(o.Label)
        if hasattr(App,"LastArchMultiMaterial"):
            for i,o in enumerate(self.multimats):
                if o.Name == App.LastArchMultiMaterial:
                    matCombo.setCurrentIndex(i+1)
                    self.MultiMat = o
        grid.addWidget(matCombo,0,0,1,2)

        label5 = QtGui.QLabel("Length")
        self.Length = ui.createWidget("Gui::InputField")
        self.Length.setText("0.00 mm")
        grid.addWidget(label5,1,0,1,1)
        grid.addWidget(self.Length,1,1,1,1)
        return w

    def getPoint(self,point=None, snapped_wall=None):
        """ this function is called by the snapper when it has a 3D point """

        if len(self.points) == 0:
            # picked first point
            if snapped_wall:
                print("\njoin first end to: "+snapped_wall.Name+"\n")
                # TODO: check if it is a wall
                self.first_end = snapped_wall.Name

            self.points.append(point)

            Gui.Snapper.getPoint(last=self.points[0],
                                 callback=self.getPoint,
                                 movecallback=None,
                                 extradlg=self.taskbox(),
                                 title="Next point"+":",mode="line")

        elif len(self.points) == 1:
            # picked last point
            if snapped_wall:
                print("\njoin first end to: "+snapped_wall.Name+"\n")
                # TODO: check if it is a wall
                self.last_end = snapped_wall.Name

            self.points.append(point) 

            wall = make_wall.make_wall_from_points(p1 = self.points[0], 
                                                   p2 = self.points[1],
                                                   join_first = self.first_end, 
                                                   join_last = self.last_end)



Gui.addCommand('Arch_Wall2', Arch_Wall())