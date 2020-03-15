# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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
import time

from PySide import QtCore
from PySide import QtGui
from PySide.QtCore import Qt
from PySide.QtGui import QApplication

import FreeCAD
import FreeCADGui

import FemGui
# from . import ViewProviderFemConstraint
from femobjects import _FemMeshGmsh
from femtools.femutils import is_of_type


# class _ViewProviderFemMeshGmsh(ViewProviderFemConstraint.ViewProxy):
class _ViewProviderFemMeshGmsh:
    """
    A View Provider for the FemMeshGmsh object
    """

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/FEM_MeshGmshFromShape.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode):
        # hide all FEM meshes and VTK FemPostPipeline objects
        for o in vobj.Object.Document.Objects:
            if (
                o.isDerivedFrom("Fem::FemMeshObject")
                or o.isDerivedFrom("Fem::FemPostPipeline")
            ):
                o.ViewObject.hide()
        # show the mesh we like to edit
        self.ViewObject.show()
        # show task panel
        taskd = _TaskPanel(self.Object)
        # taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        return True

    """
    def setEdit(self, vobj, mode=0):
        ViewProviderFemConstraint.ViewProxy.setEdit(
            self,
            vobj,
            mode,
            _TaskPanel
        )
    """

    # overwrite unsetEdit, hide mesh object on task panel exit
    def unsetEdit(self, vobj, mode):
        FreeCADGui.Control.closeDialog()
        self.ViewObject.hide()
        return True

    def doubleClicked(self, vobj):
        # Group meshing is only active on active analysis
        # we should make sure the analysis the mesh belongs too is active
        gui_doc = FreeCADGui.getDocument(vobj.Object.Document)
        if not gui_doc.getInEdit():
            # may be go the other way around and just activate the
            # analysis the user has doubleClicked on ?!
            # not a fast one, we need to iterate over all member of all
            # analysis to know to which analysis the object belongs too!!!
            # first check if there is an analysis in the active document
            found_an_analysis = False
            for o in gui_doc.Document.Objects:
                if o.isDerivedFrom("Fem::FemAnalysisPython"):
                        found_an_analysis = True
                        break
            if found_an_analysis:
                if FemGui.getActiveAnalysis() is not None:
                    if FemGui.getActiveAnalysis().Document is FreeCAD.ActiveDocument:
                        if self.Object in FemGui.getActiveAnalysis().Group:
                            if not gui_doc.getInEdit():
                                gui_doc.setEdit(vobj.Object.Name)
                            else:
                                FreeCAD.Console.PrintError(
                                    "Activate the analysis this Gmsh FEM "
                                    "mesh object belongs too!\n"
                                )
                        else:
                            FreeCAD.Console.PrintMessage(
                                "Gmsh FEM mesh object does not belong to the active analysis.\n"
                            )
                            found_mesh_analysis = False
                            for o in gui_doc.Document.Objects:
                                if o.isDerivedFrom("Fem::FemAnalysisPython"):
                                    for m in o.Group:
                                        if m == self.Object:
                                            found_mesh_analysis = True
                                            FemGui.setActiveAnalysis(o)
                                            FreeCAD.Console.PrintMessage(
                                                "The analysis the Gmsh FEM mesh object "
                                                "belongs to was found and activated: {}\n"
                                                .format(o.Name)
                                            )
                                            gui_doc.setEdit(vobj.Object.Name)
                                            break
                            if not found_mesh_analysis:
                                FreeCAD.Console.PrintMessage(
                                    "Gmsh FEM mesh object does not belong to an analysis. "
                                    "Analysis group meshing will be deactivated.\n"
                                )
                                gui_doc.setEdit(vobj.Object.Name)
                    else:
                        FreeCAD.Console.PrintError("Active analysis is not in active document.\n")
                else:
                    FreeCAD.Console.PrintMessage(
                        "No active analysis in active document, "
                        "we are going to have a look if the Gmsh FEM mesh object "
                        "belongs to a non active analysis.\n"
                    )
                    found_mesh_analysis = False
                    for o in gui_doc.Document.Objects:
                        if o.isDerivedFrom("Fem::FemAnalysisPython"):
                            for m in o.Group:
                                if m == self.Object:
                                    found_mesh_analysis = True
                                    FemGui.setActiveAnalysis(o)
                                    FreeCAD.Console.PrintMessage(
                                        "The analysis the Gmsh FEM mesh object "
                                        "belongs to was found and activated: {}\n"
                                        .format(o.Name)
                                    )
                                    gui_doc.setEdit(vobj.Object.Name)
                                    break
                    if not found_mesh_analysis:
                        FreeCAD.Console.PrintMessage(
                            "Gmsh FEM mesh object does not belong to an analysis. "
                            "Analysis group meshing will be deactivated.\n"
                        )
                        gui_doc.setEdit(vobj.Object.Name)
            else:
                FreeCAD.Console.PrintMessage("No analysis in the active document.\n")
                gui_doc.setEdit(vobj.Object.Name)
        else:
            from PySide.QtGui import QMessageBox
            message = "Active Task Dialog found! Please close this one before opening  a new one!"
            QMessageBox.critical(None, "Error in tree view", message)
            FreeCAD.Console.PrintError(message + "\n")
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        reg_childs = self.Object.MeshRegionList
        gro_childs = self.Object.MeshGroupList
        bou_childs = self.Object.MeshBoundaryLayerList
        return (reg_childs + gro_childs + bou_childs)

    def onDelete(self, feature, subelements):
        children = self.claimChildren()
        if len(children) > 0:
            try:
                for obj in children:
                    obj.ViewObject.show()
            except Exception as err:
                FreeCAD.Console.PrintError("Error in onDelete: {0} \n".format(err))
        return True

    def canDragObjects(self):
        return True

    def canDropObjects(self):
        return True

    def canDragObject(self, dragged_object):
        if (
            is_of_type(dragged_object, "Fem::MeshBoundaryLayer")
            or is_of_type(dragged_object, "Fem::MeshGroup")
            or is_of_type(dragged_object, "Fem::MeshRegion")
        ):
            return True
        else:
            return False

    def canDropObject(self, incoming_object):
        return True

    def dragObject(self, selfvp, dragged_object):
        if is_of_type(dragged_object, "Fem::MeshBoundaryLayer"):
            objs = self.Object.MeshBoundaryLayerList
            objs.remove(dragged_object)
            self.Object.MeshBoundaryLayerList = objs
        elif is_of_type(dragged_object, "Fem::MeshGroup"):
            objs = self.Object.MeshGroupList
            objs.remove(dragged_object)
            self.Object.MeshGroupList = objs
        elif is_of_type(dragged_object, "Fem::MeshRegion"):
            objs = self.Object.MeshRegionList
            objs.remove(dragged_object)
            self.Object.MeshRegionList = objs

    def dropObject(self, selfvp, incoming_object):
        if is_of_type(incoming_object, "Fem::MeshBoundaryLayer"):
            objs = self.Object.MeshBoundaryLayerList
            objs.append(incoming_object)
            self.Object.MeshBoundaryLayerList = objs
        elif is_of_type(incoming_object, "Fem::MeshGroup"):
            objs = self.Object.MeshGroupList
            objs.append(incoming_object)
            self.Object.MeshGroupList = objs
        elif is_of_type(incoming_object, "Fem::MeshRegion"):
            objs = self.Object.MeshRegionList
            objs.append(incoming_object)
            self.Object.MeshRegionList = objs
        incoming_object.Document.recompute()


class _TaskPanel:
    """
    The TaskPanel for editing References property of
    FemMeshGmsh objects and creation of new FEM mesh
    """

    def __init__(self, obj):
        self.mesh_obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(
            FreeCAD.getHomePath() + "Mod/Fem/Resources/ui/MeshGmsh.ui"
        )

        self.Timer = QtCore.QTimer()
        self.Timer.start(100)  # 100 milli seconds
        self.gmsh_runs = False
        self.console_message_gmsh = ""

        QtCore.QObject.connect(
            self.form.if_max,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.max_changed
        )
        QtCore.QObject.connect(
            self.form.if_min,
            QtCore.SIGNAL("valueChanged(Base::Quantity)"),
            self.min_changed
        )
        QtCore.QObject.connect(
            self.form.cb_dimension,
            QtCore.SIGNAL("activated(int)"),
            self.choose_dimension
        )
        QtCore.QObject.connect(
            self.Timer,
            QtCore.SIGNAL("timeout()"),
            self.update_timer_text
        )

        self.form.cb_dimension.addItems(
            _FemMeshGmsh._FemMeshGmsh.known_element_dimensions
        )

        self.get_mesh_params()
        self.get_active_analysis()
        self.update()

    def getStandardButtons(self):
        button_value = int(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel
        )
        return button_value
        # show a OK, a apply and a Cancel button
        # def reject() is called on Cancel button
        # def clicked(self, button) is needed, to access the apply button

    def accept(self):
        self.set_mesh_params()
        self.mesh_obj.ViewObject.Document.resetEdit()
        self.mesh_obj.Document.recompute()
        return True

    def reject(self):
        self.mesh_obj.ViewObject.Document.resetEdit()
        self.mesh_obj.Document.recompute()
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
        "fills the widgets"
        self.form.if_max.setText(self.clmax.UserString)
        self.form.if_min.setText(self.clmin.UserString)
        index_dimension = self.form.cb_dimension.findText(self.dimension)
        self.form.cb_dimension.setCurrentIndex(index_dimension)

    def console_log(self, message="", color="#000000"):
        if (not isinstance(message, bytes)) and (sys.version_info.major < 3):
            message = message.encode("utf-8", "replace")
        self.console_message_gmsh = self.console_message_gmsh + (
            '<font color="#0000FF">{0:4.1f}:</font> <font color="{1}">{2}</font><br>'
            .format(time.time() - self.Start, color, message)
        )
        self.form.te_output.setText(self.console_message_gmsh)
        self.form.te_output.moveCursor(QtGui.QTextCursor.End)

    def update_timer_text(self):
        # FreeCAD.Console.PrintMessage("timer1\n")
        if self.gmsh_runs:
            FreeCAD.Console.PrintMessage("timer2\n")
            # FreeCAD.Console.PrintMessage("Time: {0:4.1f}: \n".format(time.time() - self.Start))
            self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))

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
        part = self.mesh_obj.Part
        if (
            self.mesh_obj.MeshRegionList and part.Shape.ShapeType == "Compound"
            and (
                is_of_type(part, "FeatureBooleanFragments")
                or is_of_type(part, "FeatureSlice")
                or is_of_type(part, "FeatureXOR")
            )
        ):
            error_message = (
                "The shape to mesh is a boolean split tools Compound "
                "and the mesh has mesh region list. "
                "Gmsh could return unexpected meshes in such circumstances. "
                "It is strongly recommended to extract the shape "
                "to mesh from the Compound and use this one."
            )
            qtbox_title = (
                "Shape to mesh is a BooleanFragmentsCompound "
                "and mesh regions are defined"
            )
            QtGui.QMessageBox.critical(
                None,
                qtbox_title,
                error_message
            )
        self.Start = time.time()
        self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))
        self.console_message_gmsh = ""
        self.gmsh_runs = True
        self.console_log("We are going to start ...")
        self.get_active_analysis()
        from femmesh import gmshtools
        gmsh_mesh = gmshtools.GmshTools(self.mesh_obj, self.analysis)
        self.console_log("Start Gmsh ...")
        error = ""
        try:
            error = gmsh_mesh.create_mesh()
        except:
            import sys
            FreeCAD.Console.PrintMessage(
                "Unexpected error when creating mesh: {}\n"
                .format(sys.exc_info()[0])
            )
        if error:
            FreeCAD.Console.PrintMessage("Gmsh had warnings ...\n")
            FreeCAD.Console.PrintMessage("{}\n".format(error))
            self.console_log("Gmsh had warnings ...")
            self.console_log(error, "#FF0000")
        else:
            FreeCAD.Console.PrintMessage("Clean run of Gmsh\n")
            self.console_log("Clean run of Gmsh")
        self.console_log("Gmsh done!")
        self.form.l_time.setText("Time: {0:4.1f}: ".format(time.time() - self.Start))
        self.Timer.stop()
        self.update()
        QApplication.restoreOverrideCursor()

    def get_active_analysis(self):
        import FemGui
        self.analysis = FemGui.getActiveAnalysis()
        if self.analysis:
            for m in FemGui.getActiveAnalysis().Group:
                if m.Name == self.mesh_obj.Name:
                    FreeCAD.Console.PrintMessage(
                        "Active analysis found: {}\n"
                        .format(self.analysis.Name)
                    )
                    return
            else:
                FreeCAD.Console.PrintLog(
                    "Mesh is not member of active analysis, means no group meshing.\n"
                )
                self.analysis = None  # no group meshing
        else:
            FreeCAD.Console.PrintLog("No active analysis, means no group meshing.\n")
            self.analysis = None  # no group meshing
