# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM mesh gmsh ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemMeshGmsh
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemMeshGmsh

import sys
import FreeCAD
import FreeCADGui
import FemGui

# for the panel
from femobjects import _FemMeshGmsh
from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication
import time


class _ViewProviderFemMeshGmsh:
    "A View Provider for the FemMeshGmsh object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-femmesh-from-shape.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        # hide all meshes
        for o in FreeCAD.ActiveDocument.Objects:
            if o.isDerivedFrom("Fem::FemMeshObject"):
                o.ViewObject.hide()
        # show the mesh we like to edit
        self.ViewObject.show()
        # show task panel
        taskd = _TaskPanelFemMeshGmsh(self.Object)
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        self.ViewObject.hide()  # hide the mesh after edit is finished
        return True

    def doubleClicked(self, vobj):
        # Group meshing is only active on active analysis, we should make sure the analysis the mesh belongs too is active
        gui_doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not gui_doc.getInEdit():
            # may be go the other way around and just activate the analysis the user has doubleClicked on ?!
            # not a fast one, we need to iterate over all member of all analysis to know to which analysis the object belongs too!!!
            # first check if there is an analysis in the active document
            found_an_analysis = False
            for o in gui_doc.Document.Objects:
                if o.isDerivedFrom('Fem::FemAnalysisPython'):
                        found_an_analysis = True
                        break
            if found_an_analysis:
                if FemGui.getActiveAnalysis() is not None:
                    if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                        if self.Object in FemGui.getActiveAnalysis().Group:
                            if not gui_doc.getInEdit():
                                gui_doc.setEdit(vobj.Object.Name)
                            else:
                                FreeCAD.Console.PrintError('Activate the analysis this Gmsh FEM mesh object belongs too!\n')
                        else:
                            print('Gmsh FEM mesh object does not belong to the active analysis.')
                            found_mesh_analysis = False
                            for o in gui_doc.Document.Objects:
                                if o.isDerivedFrom('Fem::FemAnalysisPython'):
                                    for m in o.Group:
                                        if m == self.Object:
                                            found_mesh_analysis = True
                                            FemGui.setActiveAnalysis(o)
                                            print('The analysis the Gmsh FEM mesh object belongs too was found and activated: ' + o.Name)
                                            gui_doc.setEdit(vobj.Object.Name)
                                            break
                            if not found_mesh_analysis:
                                print('Gmsh FEM mesh object does not belong to an analysis. Analysis group meshing will be deactivated.')
                                gui_doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError('Active analysis is not in active document.')
                else:
                    print('No active analysis in active document, we are going to have a look if the Gmsh FEM mesh object belongs to a non active analysis.')
                    found_mesh_analysis = False
                    for o in gui_doc.Document.Objects:
                        if o.isDerivedFrom('Fem::FemAnalysisPython'):
                            for m in o.Group:
                                if m == self.Object:
                                    found_mesh_analysis = True
                                    FemGui.setActiveAnalysis(o)
                                    print('The analysis the Gmsh FEM mesh object belongs to was found and activated: ' + o.Name)
                                    gui_doc.setEdit(vobj.Object.Name)
                                    break
                    if not found_mesh_analysis:
                        print('Gmsh FEM mesh object does not belong to an analysis. Analysis group meshing will be deactivated.')
                        gui_doc.setEdit(vobj.Object.Name)
            else:
                print('No analysis in the active document.')
                gui_doc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = 'Active Task Dialog found! Please close this one before open a new one!'
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + '\n')
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        return (self.Object.MeshRegionList + self.Object.MeshGroupList + self.Object.MeshBoundaryLayerList)

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + err.message)
        return True

    def canDragObjects(self):
        return True

    def canDropObjects(self):
        return True

    def canDragObject(self, dragged_object):
        if hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshBoundaryLayer":
            return True
        elif hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshGroup":
            return True
        elif hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshRegion":
            return True
        else:
            return False

    def canDropObject(self, incoming_object):
        return True

    def dragObject(self, selfvp, dragged_object):
        if hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshBoundaryLayer":
            objs = self.Object.MeshBoundaryLayerList
            objs.remove(dragged_object)
            self.Object.MeshBoundaryLayerList = objs
        elif hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshGroup":
            objs = self.Object.MeshGroupList
            objs.remove(dragged_object)
            self.Object.MeshGroupList = objs
        elif hasattr(dragged_object, "Proxy") and dragged_object.Proxy.Type == "Fem::FemMeshRegion":
            objs = self.Object.MeshRegionList
            objs.remove(dragged_object)
            self.Object.MeshRegionList = objs

    def dropObject(self, selfvp, incoming_object):
        if hasattr(incoming_object, "Proxy") and incoming_object.Proxy.Type == "Fem::FemMeshBoundaryLayer":
            objs = self.Object.MeshBoundaryLayerList
            objs.append(incoming_object)
            self.Object.MeshBoundaryLayerList = objs
        elif hasattr(incoming_object, "Proxy") and incoming_object.Proxy.Type == "Fem::FemMeshGroup":
            objs = self.Object.MeshGroupList
            objs.append(incoming_object)
            self.Object.MeshGroupList = objs
        elif hasattr(incoming_object, "Proxy") and incoming_object.Proxy.Type == "Fem::FemMeshRegion":
            objs = self.Object.MeshRegionList
            objs.append(incoming_object)
            self.Object.MeshRegionList = objs
        FreeCAD.ActiveDocument.recompute()


class _TaskPanelFemMeshGmsh:
    '''The TaskPanel for editing References property of FemMeshGmsh objects and creation of new FEM mesh'''

    def __init__(self, obj):
        self.mesh_obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGmsh.ui")

        self.Timer = QtCore.QTimer()
        self.Timer.start(100)  # 100 milli seconds
        self.gmsh_runs = False
        self.console_message_gmsh = ''

        QtCore.QObject.connect(self.form.if_max, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.max_changed)
        QtCore.QObject.connect(self.form.if_min, QtCore.SIGNAL("valueChanged(Base::Quantity)"), self.min_changed)
        QtCore.QObject.connect(self.form.cb_dimension, QtCore.SIGNAL("activated(int)"), self.choose_dimension)
        QtCore.QObject.connect(self.Timer, QtCore.SIGNAL("timeout()"), self.update_timer_text)

        self.form.cb_dimension.addItems(_FemMeshGmsh._FemMeshGmsh.known_element_dimensions)

        self.get_mesh_params()
        self.get_active_analysis()
        self.update()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel)
        # show a OK, a apply and a Cancel button
        # def reject() is called on Cancel button
        # def clicked(self, button) is needed, to access the apply button

    def accept(self):
        self.set_mesh_params()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()
        return True

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            self.set_mesh_params()
            self.run_gmsh()

    def get_mesh_params(self):
        self.clmax = self.mesh_obj.CharacteristicLengthMax
        self.clmin = self.mesh_obj.CharacteristicLengthMin
        self.dimension = self.mesh_obj.ElementDimension

    def set_mesh_params(self):
        self.mesh_obj.CharacteristicLengthMax = self.clmax
        self.mesh_obj.CharacteristicLengthMin = self.clmin
        self.mesh_obj.ElementDimension = self.dimension

    def update(self):
        'fills the widgets'
        self.form.if_max.setText(self.clmax.UserString)
        self.form.if_min.setText(self.clmin.UserString)
        index_dimension = self.form.cb_dimension.findText(self.dimension)
        self.form.cb_dimension.setCurrentIndex(index_dimension)

    def console_log(self, message="", color="#000000"):
        if (not isinstance(message, bytes)) and (sys.version_info.major < 3):
            message = message.encode('utf-8', 'replace')
        self.console_message_gmsh = self.console_message_gmsh + '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'.\
            format(time.time() - self.Start, color, message)
        self.form.te_output.setText(self.console_message_gmsh)
        self.form.te_output.moveCursor(QtGui.QTextCursor.End)

    def update_timer_text(self):
        # print('timer1')
        if self.gmsh_runs:
            print('timer2')
            # print('Time: {0:4.1f}: '.format(time.time() - self.Start))
            self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))

    def max_changed(self, base_quantity_value):
        self.clmax = base_quantity_value

    def min_changed(self, base_quantity_value):
        self.clmin = base_quantity_value

    def choose_dimension(self, index):
        if index < 0:
            return
        self.form.cb_dimension.setCurrentIndex(index)
        self.dimension = str(self.form.cb_dimension.itemText(index))  # form returns unicode

    def run_gmsh(self):
        QApplication.setOverrideCursor(Qt.WaitCursor)
        part = self.obj.Part
        if self.mesh_obj.MeshRegionList:
            if part.Shape.ShapeType == "Compound" and hasattr(part, "Proxy"):  # other part obj might not have a Proxy, thus an exception would be raised
                if (part.Proxy.Type == "FeatureBooleanFragments" or part.Proxy.Type == "FeatureSlice" or part.Proxy.Type == "FeatureXOR"):
                    error_message = (
                        'The mesh to shape is a boolean split tools Compound and the mesh has mesh region list. '
                        'Gmsh could return unexpected meshes in such circumstances. '
                        'It is strongly recommended to extract the shape to mesh from the Compound and use this one.'
                    )
                    QtGui.QMessageBox.critical(None, "Shape to mesh is a BooleanFragmentsCompound and mesh regions are defined", error_message)
        self.Start = time.time()
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.console_message_gmsh = ''
        self.gmsh_runs = True
        self.console_log("We are going to start ...")
        self.get_active_analysis()
        import femmesh.gmshtools as gmshtools
        gmsh_mesh = gmshtools.GmshTools(self.obj, self.analysis)
        self.console_log("Start Gmsh ...")
        error = ''
        try:
            error = gmsh_mesh.create_mesh()
        except:
            import sys
            print("Unexpected error when creating mesh: ", sys.exc_info()[0])
        if error:
            print(error)
            self.console_log('Gmsh had warnings ...')
            self.console_log(error, '#FF0000')
        else:
            self.console_log('Clean run of Gmsh')
        self.console_log("Gmsh done!")
        self.form.l_time.setText('Time: {0:4.1f}: '.format(time.time() - self.Start))
        self.Timer.stop()
        self.update()
        QApplication.restoreOverrideCursor()

    def get_active_analysis(self):
        import FemGui
        self.analysis = FemGui.getActiveAnalysis()
        if self.analysis:
            for m in FemGui.getActiveAnalysis().Group:
                if m.Name == self.mesh_obj.Name:
                    print('Active analysis found: ' + self.analysis.Name)
                    return
            else:
                # print('Mesh is not member of active analysis, means no group meshing')
                self.analysis = None  # no group meshing
        else:
            # print('No active analysis, means no group meshing')
            self.analysis = None  # no group meshing
