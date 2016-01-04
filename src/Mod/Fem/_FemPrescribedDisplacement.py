#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command Prescribed Displacement"
__author__ = "Alfred Bogaers and Michael Hindley"
__url__ = "http://www.freecadweb.org"

class MakePrescribedDisplacement:

    # def __init__(self, obj,objectComponent):
    def __init__(self):
        import FreeCAD
        if FreeCAD.GuiUp:
           import FreeCADGui
           import FemGui
        from PySide import QtGui
        from PySide import QtCore
        from _ViewProviderFemPrescribedDisplacement import viewProviderPrescribedDisplacement

        selection = FreeCADGui.Selection.getSelectionEx()

        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython", "PrescribedDisplacement")

        obj.addProperty("App::PropertyLink", "Object", "DisplacementSettings")
        obj.addProperty("App::PropertyStringList", "partNameList", "DisplacementSettings")

        # Define variables used
        obj.addProperty("App::PropertyFloat", "xDisplacement", "DisplacementSettings",
                        "x Displacement").xDisplacement = 0.0
        obj.addProperty("App::PropertyFloat", "yDisplacement", "DisplacementSettings",
                        "y Displacement").yDisplacement = 0.0
        obj.addProperty("App::PropertyFloat", "zDisplacement", "DisplacementSettings",
                        "z Displacement").zDisplacement = 0.0
        obj.addProperty("App::PropertyFloat", "xRotation", "DisplacementSettings", "x Rotation").xRotation = 0.0
        obj.addProperty("App::PropertyFloat", "yRotation", "DisplacementSettings", "y Rotation").yRotation = 0.0
        obj.addProperty("App::PropertyFloat", "zRotation", "DisplacementSettings", "z Rotation").zRotation = 0.0
        obj.addProperty("App::PropertyBool", "xFree", "DisplacementSettings", "x Free").xFree = True
        obj.addProperty("App::PropertyBool", "yFree", "DisplacementSettings", "y Free").yFree = True
        obj.addProperty("App::PropertyBool", "zFree", "DisplacementSettings", "z Free").zFree = True
        obj.addProperty("App::PropertyBool", "xFix", "DisplacementSettings", "x Fix").xFix = False
        obj.addProperty("App::PropertyBool", "yFix", "DisplacementSettings", "y Fix").yFix = False
        obj.addProperty("App::PropertyBool", "zFix", "DisplacementSettings", "z Fix").zFix = False
        obj.addProperty("App::PropertyBool", "rotxFree", "DisplacementSettings", "rot x Free").rotxFree = True
        obj.addProperty("App::PropertyBool", "rotyFree", "DisplacementSettings", "rot y Free").rotyFree = True
        obj.addProperty("App::PropertyBool", "rotzFree", "DisplacementSettings", "rot z Free").rotzFree = True
        obj.addProperty("App::PropertyBool", "rotxFix", "DisplacementSettings", "rot x Fix").rotxFix = False
        obj.addProperty("App::PropertyBool", "rotyFix", "DisplacementSettings", "rot y Fix").rotyFix = False
        obj.addProperty("App::PropertyBool", "rotzFix", "DisplacementSettings", "rot z Fix").rotzFix = False
        obj.addProperty("App::PropertyBool", "element", "DisplacementSettings", "element").element = False
        # Define part settings
        # obj.addProperty("App::PropertyStringList","subPartLink","DisplacementSettings")
        obj.addProperty("Part::PropertyPartShape", "Shape", "DisplacementSettings")

        obj.Proxy = self

        obj.setEditorMode("Object", 2)
        obj.setEditorMode("partNameList", 1)
        # obj.setEditorMode("subPartLink",2)
        # obj.setEditorMode("Shape",0)
        #Assume no object selected
        partNameList = []
        obj.partNameList = partNameList


        objectComponent = selection[0]
        subComponents = objectComponent.SubElementNames
        for i in range(len(subComponents)):
            partNameList.append(str(subComponents[i]))
        obj.Object = objectComponent.Object

        viewProviderPrescribedDisplacement(obj.ViewObject)


    def execute(self, fp):
        import Part
        import Fem
        import FemGui
        listOfShapes = []
        if len(fp.partNameList) >= 1:
            for i in range(len(fp.partNameList)):
                if fp.partNameList[i][0:4] == 'Face':
                    ind = int(fp.partNameList[i][4::]) - 1
                    listOfShapes.append(fp.Object.Shape.Faces[ind])
                if fp.partNameList[i][0:4] == 'Edge':
                    ind = int(fp.partNameList[i][4::]) - 1
                    listOfShapes.append(fp.Object.Shape.Edges[ind])
                if fp.partNameList[i][0:4] == "Vert":
                    ind = int(fp.partNameList[i][6::]) - 1
                    listOfShapes.append(fp.Object.Shape.Vertexes[ind])
            if len(listOfShapes) > 0:
               fp.Shape = Part.makeCompound(listOfShapes)
            else:
               fp.Shape = Part.Shape()

        return





class TaskPanelPrescribedDisplacement:
    def __init__(self, obj):
        import FreeCAD
        from PySide import QtGui
        if FreeCAD.GuiUp:
           import FreeCADGui
           import FemGui
        from PySide import QtGui
        from PySide import QtCore
        from _ViewProviderFemPrescribedDisplacement import viewProviderPrescribedDisplacement

        self.obj = obj
        # self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath()+ "Mod/Fem/Resources/TaskFemConstraintDisplacement.ui")
        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/FemPrescribedDisplacement.ui")
        # Store initial value in case of cancel
        self.setIntialValues()
        self.xOld = self.obj.xDisplacement
        self.yOld = self.obj.yDisplacement
        self.zOld = self.obj.zDisplacement
        self.xrOld = self.obj.xRotation
        self.yrOld = self.obj.yRotation
        self.zrOld = self.obj.zRotation
        self.xfixold = self.obj.xFix
        self.yfixold = self.obj.yFix
        self.zfixold = self.obj.zFix
        self.rotxfixold = self.obj.rotxFix
        self.rotyfixold = self.obj.rotyFix
        self.rotzfixold = self.obj.rotzFix
        self.xFreeold = self.obj.xFree
        self.yFreeold = self.obj.yFree
        self.zFreeold = self.obj.zFree
        self.rotxFreeold = self.obj.rotxFree
        self.rotyFreeold = self.obj.rotyFree
        self.rotzFreeold = self.obj.rotzFree

        # self.fixxOld = self.obj.xFix

        # Link GUI to variables
        # Define Qt object variables
        # Variables in GUI with decimal values
        QtCore.QObject.connect(self.form.spinxDisplacement, QtCore.SIGNAL("valueChanged(double)"), self.x_changed)
        QtCore.QObject.connect(self.form.spinyDisplacement, QtCore.SIGNAL("valueChanged(double)"), self.y_changed)
        QtCore.QObject.connect(self.form.spinzDisplacement, QtCore.SIGNAL("valueChanged(double)"), self.z_changed)
        QtCore.QObject.connect(self.form.rotxv, QtCore.SIGNAL("valueChanged(double)"), self.x_rot)
        QtCore.QObject.connect(self.form.rotyv, QtCore.SIGNAL("valueChanged(double)"), self.y_rot)
        QtCore.QObject.connect(self.form.rotzv, QtCore.SIGNAL("valueChanged(double)"), self.z_rot)
        QtCore.QObject.connect(self.form.localcoord, QtCore.SIGNAL("stateChanged(int)"), self.localcoord)
        # Connect check box values displacements
        QtCore.QObject.connect(self.form.dispxfix, QtCore.SIGNAL("stateChanged(int)"), self.fixx)
        QtCore.QObject.connect(self.form.dispxfree, QtCore.SIGNAL("stateChanged(int)"), self.freex)
        QtCore.QObject.connect(self.form.dispyfix, QtCore.SIGNAL("stateChanged(int)"), self.fixy)
        QtCore.QObject.connect(self.form.dispyfree, QtCore.SIGNAL("stateChanged(int)"), self.freey)
        QtCore.QObject.connect(self.form.dispzfix, QtCore.SIGNAL("stateChanged(int)"), self.fixz)
        QtCore.QObject.connect(self.form.dispzfree, QtCore.SIGNAL("stateChanged(int)"), self.freez)
        # Connect to check box values for rotations
        QtCore.QObject.connect(self.form.rotxfix, QtCore.SIGNAL("stateChanged(int)"), self.rotfixx)
        QtCore.QObject.connect(self.form.rotxfree, QtCore.SIGNAL("stateChanged(int)"), self.rotfreex)
        QtCore.QObject.connect(self.form.rotyfix, QtCore.SIGNAL("stateChanged(int)"), self.rotfixy)
        QtCore.QObject.connect(self.form.rotyfree, QtCore.SIGNAL("stateChanged(int)"), self.rotfreey)
        QtCore.QObject.connect(self.form.rotzfix, QtCore.SIGNAL("stateChanged(int)"), self.rotfixz)
        QtCore.QObject.connect(self.form.rotzfree, QtCore.SIGNAL("stateChanged(int)"), self.rotfreez)

        # QtCore.QObject.connect(self.form.listWidget, QtCore.SIGNAL("itemClicked("+str(self.form.listWidget.item(0))+")"),self.setSelection)

        # QtCore.QObject.connect(self.form.pushButtonDelete, QtCore.SIGNAL("clicked(QListWidgetItem)"), self.z_changed)
        # Selections Buttons
        self.form.listWidget.itemClicked.connect(self.setSelection)
        self.form.pushButtonDelete.clicked.connect(self.deleteFeature)
        self.form.pushButtonAdd.clicked.connect(self.addFeature)

        # self.form.pushButtonDelete.released.connect(self.deleteFeature)
        self.form.pushButtonDelete.setEnabled(False)

        #****************************************************************
        #Set UI values to current values
        # X
        self.form.spinxDisplacement.setValue(obj.xDisplacement)
        self.form.dispxfix.setChecked(obj.xFix)
        self.form.dispxfree.setChecked(obj.xFree)

        # Y
        self.form.spinyDisplacement.setValue(obj.yDisplacement)
        self.form.dispyfix.setChecked(obj.yFix)
        self.form.dispyfree.setChecked(obj.yFree)

        # Z
        self.form.spinzDisplacement.setValue(obj.zDisplacement)
        self.form.dispzfix.setChecked(obj.zFix)
        self.form.dispzfree.setChecked(obj.zFree)

        # rotX
        self.form.rotxv.setValue(obj.xRotation)
        self.form.rotxfix.setChecked(obj.rotxFix)
        self.form.rotxfree.setChecked(obj.rotxFree)

        # rotY
        self.form.rotyv.setValue(obj.yRotation)
        self.form.rotyfix.setChecked(obj.rotyFix)
        self.form.rotyfree.setChecked(obj.rotyFree)

         # rotZ
        self.form.rotzv.setValue(obj.zRotation)
        self.form.rotzfix.setChecked(obj.rotzFix)
        self.form.rotzfree.setChecked(obj.rotzFree)


    # Set values if value in spin-box changed
    # if value changed save value and uncheck tick boxes
    def x_changed(self, value):
        if value != 0.0:
            self.obj.xDisplacement = value
            self.obj.xFix = False
            self.obj.xFree = False
            self.form.dispxfree.setChecked(False)  # setFree check box off
            self.form.dispxfix.setChecked(False)  # setfix check box off
        else:
            self.obj.xFree = True
        return

    def y_changed(self, value):
        if value != 0.0:
            self.obj.yDisplacement = value
            self.obj.yFix = False
            self.obj.yFree = False
            self.form.dispyfree.setChecked(False)  # setFree check box off
            self.form.dispyfix.setChecked(False)  # setfix check box off
        else:
            self.obj.yFree = True
        return

    def z_changed(self, value):
        if value != 0.0:
            self.obj.zDisplacement = value
            self.obj.zFix = False
            self.obj.zFree = False
            self.form.dispzfree.setChecked(False)  # setFree check box off
            self.form.dispzfix.setChecked(False)  # setfix check box off
        else:
            self.obj.zFree = True
        return

    # rotations
    def x_rot(self, value):
        if value != 0.0:
            self.obj.rotxFix = False
            self.obj.rotxFree = False
            self.form.rotxfree.setChecked(False)  # setFree check box off
            self.form.rotxfix.setChecked(False)  # setfix check box off
            self.obj.xRotation = value
        return

    def y_rot(self, value):
        if value != 0.0:
            self.obj.rotyFix = False
            self.obj.rotyFree = False
            self.form.rotyfree.setChecked(False)  # setFree check box off
            self.form.rotyfix.setChecked(False)  # setfix check box off
            self.obj.yRotation = value
        return

    def z_rot(self, value):
        if value != 0.0:
            self.obj.rotzFix = False
            self.obj.rotzFree = False
            self.form.rotzfree.setChecked(False)  # setFree check box off
            self.form.rotzfix.setChecked(False)  # setfix check box off
            self.obj.zRotation = value
        return

        # Set value for fixed check boxes

    # When set to checked set displacement to zero and uncheck free box

    def fixx(self, value):
        if value == 2:
            self.obj.xDisplacement = 0.0  # set global displacement to 0
            self.obj.xFix = True  # save value of check box 0 open 2 checked
            self.obj.xFree = False
            self.form.dispxfree.setChecked(False)  # setFree check box off
            self.form.spinxDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.xFix = False  # save value of check box 0 open 2 checked
        return

    def fixy(self, value):
        if value == 2:
            self.obj.yDisplacement = 0.0  # set global displacement to 0
            self.obj.yFix = True  # save value of check box 0 open 2 checked
            self.obj.yFree = False
            self.form.dispyfree.setChecked(False)  # setFree check box off
            self.form.spinyDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.yFix = False  # save value of check box 0 open 2 checked
        return

    def fixz(self, value):
        if value == 2:
            self.obj.zDisplacement = 0.0  # set global displacement to 0
            self.obj.zFix = True  # save value of check box 0 open 2 checked
            self.obj.zFree = False
            self.form.dispzfree.setChecked(False)  # setFree check box off
            self.form.spinzDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.zFix = False  # save value of check box 0 open 2 checked
        return
    # rotations

    def rotfixx(self, value):
        if value == 2:
            self.obj.xRotation = 0.0  # set global displacement to 0
            self.obj.rotxFix = True  # save value of check box 0 open 2 checked
            self.obj.rotxFree = False
            self.form.rotxfree.setChecked(False)  # setFree check box off
            self.form.rotxv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotxFix = False  # save value of check box 0 open 2 checked
        return

    def rotfixy(self, value):
        if value == 2:
            self.obj.yRotation = 0.0  # set global displacement to 0
            self.obj.rotyFix = True  # save value of check box 0 open 2 checked
            self.obj.rotyFree = False
            self.form.rotyfree.setChecked(False)  # setFree check box off
            self.form.rotyv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotyFix = False  # save value of check box 0 open 2 checked
        return

    def rotfixz(self, value):
        if value == 2:
            self.obj.zRotation = 0.0  # set global displacement to 0
            self.obj.rotzFix = True  # save value of check box 0 open 2 checked
            self.obj.rotzFree = False
            self.form.rotzfree.setChecked(False)  # setFree check box off
            self.form.rotzv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotzFix = False  # save value of check box 0 open 2 checked
        return

        # Set value for free check boxes

    # When set to checked set displacement to zero and uncheck free box
    # QtGui.QMessageBox.information(  QtGui.qApp.activeWindow(), "Freex", str(value) )

    def freex(self, value):
        if value == 2:
            self.obj.xFree = True  # save value of check box 0 open 2 checked
            self.obj.xDisplacement = 0  # set global displacement to 0
            self.form.dispxfix.setChecked(False)  # setFix check box off
            self.form.spinxDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.xFree = False  # save value of check box 0 open 2 checked
        return

    def freey(self, value):
        if value == 2:
            self.obj.yFree = True  # save value of check box 0 open 2 checked
            self.obj.yDisplacement = 0  # set global displacement to 0
            self.form.dispyfix.setChecked(False)  # setFix check box off
            self.form.spinyDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.yFree = False  # save value of check box 0 open 2 checked
        return

    def freez(self, value):
        if value == 2:
            self.obj.zFree = True  # save value of check box 0 open 2 checked
            self.obj.yDisplacement = 0  # set global displacement to 0
            self.form.dispzfix.setChecked(False)  # setFix check box off
            self.form.spinzDisplacement.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.zFree = False  # save value of check box 0 open 2 checked
        return

    def rotfreex(self, value):
        if value == 2:
            self.obj.rotxFree = True  # save value of check box 0 open 2 checked
            self.obj.xRotation = 0  # set global displacement to 0
            self.form.rotxfix.setChecked(False)  # setFix check box off
            self.form.rotxv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotxFree = False  # save value of check box 0 open 2 checked
        return

    def rotfreey(self, value):
        if value == 2:
            self.obj.rotyFree = True  # save value of check box 0 open 2 checked
            self.obj.yRotation = 0  # set global displacement to 0
            self.form.rotyfix.setChecked(False)  # setFix check box off
            self.form.rotyv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotyFree = False  # save value of check box 0 open 2 checked
        return

    def rotfreez(self, value):
        if value == 2:
            self.obj.rotzFree = True  # save value of check box 0 open 2 checked
            self.obj.zRotation = 0  # set global displacement to 0
            self.form.rotzfix.setChecked(False)  # setFix check box off
            self.form.rotzv.setValue(0.0)  # set value of spin-box to 0
        else:
            self.obj.rotzFree = False  # save value of check box 0 open 2 checked
        return

    def localcoord(self, value):
        if value == 2:
            self.obj.element = True
        else:
            self.obj.element = false
        return

    def setSelection(self, value):
        import FreeCAD
        import FreeCADGui
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.obj.Object, str(value.text()))
        self.form.pushButtonDelete.setEnabled(True)
        return

    # Add a geometry to select only if selected first.
    def addFeature(self):
        import FreeCAD
        if FreeCAD.GuiUp:
           import FreeCADGui
           import FemGui
        #from PySide import QtGui
        selection = FreeCADGui.Selection.getSelectionEx()

        #FreeCAD.Console.PrintMessage("Entered Add Feature \n")
        #FreeCAD.Console.PrintMessage(str(len(selection)) + " \n")


        if len(selection) == 1 and selection[0].Object.isDerivedFrom("Part::Feature") and not (
                    "DisplacementSettings" in selection[0].Object.Content):
            subComponents = selection[0].SubElementNames

            partNameList = []
            for i in range(len(subComponents)):
                self.form.listWidget.addItem(str(subComponents[i]))

            partNameList = []
            for i in range(self.form.listWidget.count()):
                item = self.form.listWidget.item(i)
                partNameList.append(item.text())
            self.obj.partNameList = partNameList


            FreeCADGui.doCommand("App.activeDocument().recompute()")

        else:
            QtGui.QMessageBox.information(QtGui.qApp.activeWindow(), "Error", "Select either edges, vertices or faces.")

    # Delete a feature
    def deleteFeature(self):
        import FreeCAD
        import FreeCADGui
        #from _ViewProviderFemPrescribedDisplacement import viewProviderPrescribedDisplacement

        currentItem = self.form.listWidget.currentItem()
        row = self.form.listWidget.row(currentItem)
        self.form.listWidget.takeItem(row)

        partNameList = []
        for i in range(self.form.listWidget.count()):
            item = self.form.listWidget.item(i)
            partNameList.append(item.text())
            FreeCAD.Console.PrintMessage(item.text())

        self.obj.partNameList = partNameList

        FreeCADGui.doCommand("App.activeDocument().recompute()")
        FreeCADGui.Selection.clearSelection()
        self.form.pushButtonDelete.setEnabled(False)

        return
    #Set inital values when from opens
    def setIntialValues(self):
        self.form.spinxDisplacement.setValue(self.obj.xDisplacement)
        self.form.spinyDisplacement.setValue(self.obj.yDisplacement)
        self.form.spinzDisplacement.setValue(self.obj.zDisplacement)
        for i in range(len(self.obj.partNameList)):
            # self.form.listWidget.addItem(str(self.obj.Object.Name)+"::"+str(self.obj.partNameList[i]))
            self.form.listWidget.addItem(str(self.obj.partNameList[i]))

    # self.form.listWidget.editItem(self.form.listWidget.item(0))

    def accept(self):
        import FreeCAD
        import FreeCADGui
        FreeCADGui.ActiveDocument.resetEdit()

    # cancel changes and reset values to initialvales saved
    def reject(self):
        import FreeCAD
        import FreeCADGui
        self.obj.xDisplacement = self.xOld
        self.obj.yDisplacement = self.yOld
        self.obj.zDisplacement = self.zOld
        self.obj.xRotation = self.xrOld
        self.obj.yRotation = self.yrOld
        self.obj.zRotation = self.zrOld
        self.obj.xFix = self.xfixold
        self.obj.yFix = self.yfixold
        self.obj.zFix = self.zfixold
        self.obj.rotxFix = self.rotxfixold
        self.obj.rotyFix = self.rotyfixold
        self.obj.rotzFix = self.rotzfixold
        self.obj.xFree = self.xFreeold
        self.obj.yFree = self.yFreeold
        self.obj.zFree = self.zFreeold
        self.obj.rotxFree = self.rotxFreeold
        self.obj.rotyFree = self.rotyFreeold
        self.obj.rotzFree = self.rotzFreeold


        FreeCADGui.ActiveDocument.resetEdit()

        # Open prescribed displacement GUI


