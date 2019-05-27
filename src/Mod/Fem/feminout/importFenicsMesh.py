# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Johannes Hartung <j.hartung@gmx.net>             *
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

__title__ = "FreeCAD Fenics mesh reader and writer"
__author__ = "Johannes Hartung"
__url__ = "http://www.freecadweb.org"

## @package importFenicsMesh
#  \ingroup FEM
#  \brief FreeCAD Fenics Mesh reader and writer for FEM workbench

import os

import FreeCAD
import FreeCADGui

if FreeCAD.GuiUp == 1:
    from PySide import QtGui, QtCore


from . import importToolsFem
from . import readFenicsXML
from . import writeFenicsXML
from . import writeFenicsXDMF


# Template copied from importZ88Mesh.py. Thanks Bernd!
# ********* generic FreeCAD import and export methods *********
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open

if FreeCAD.GuiUp == 1:
    class WriteXDMFTaskPanel:
        """
        This task panel is used to write mesh groups with user defined values.
        It will called if there are mesh groups detected. Else it will be bypassed.
        """
        def __init__(self, fem_mesh_obj, fileString):
            self.form = FreeCADGui.PySideUic.loadUi(os.path.join(
                FreeCAD.getHomePath(),
                "Mod/Fem/Resources/ui/MeshGroupXDMFExport.ui"
            ))
            self.result_dict = {}
            self.fem_mesh_obj = fem_mesh_obj
            self.fileString = fileString

            self.convert_fem_mesh_obj_to_table()

        def convert_fem_mesh_obj_to_table(self):

            def ro(item):
                item.setFlags(~QtCore.Qt.ItemIsEditable & ~QtCore.Qt.ItemIsEnabled)
                return item

            gmshgroups = importToolsFem.get_FemMeshObjectMeshGroups(
                self.fem_mesh_obj
            )
            fem_mesh = self.fem_mesh_obj.FemMesh

            self.form.tableGroups.setRowCount(0)
            self.form.tableGroups.setRowCount(len(gmshgroups))

            for (ind, gind) in enumerate(gmshgroups):
                # group number
                self.form.tableGroups.setItem(ind, 0,
                                              ro(QtGui.QTableWidgetItem(
                                                  str(gind))))
                # group name
                self.form.tableGroups.setItem(ind, 1,
                                              ro(QtGui.QTableWidgetItem(
                                                  fem_mesh.getGroupName(gind))))
                # group elements
                self.form.tableGroups.setItem(ind, 2,
                                              ro(QtGui.QTableWidgetItem(
                                                  fem_mesh.getGroupElementType(
                                                      gind))))
                # default value for not marked elements
                self.form.tableGroups.setItem(ind, 3,
                                              QtGui.QTableWidgetItem(str(0)))
                # default value for marked elements
                self.form.tableGroups.setItem(ind, 4,
                                              QtGui.QTableWidgetItem(str(1)))

            header = self.form.tableGroups.horizontalHeader()
            header.setResizeMode(0, QtGui.QHeaderView.ResizeToContents)
            header.setResizeMode(1, QtGui.QHeaderView.ResizeToContents)
            header.setResizeMode(2, QtGui.QHeaderView.ResizeToContents)
            header.setResizeMode(3, QtGui.QHeaderView.ResizeToContents)
            header.setResizeMode(4, QtGui.QHeaderView.Stretch)

        def convert_table_to_group_dict(self):
            group_values_dict = {}
            num_rows = self.form.tableGroups.rowCount()

            for r in range(num_rows):
                g = int(self.form.tableGroups.item(r, 0).text())
                # read-only no prob
                default_value = 0
                marked_value = 1
                try:
                    default_value = int(self.form.tableGroups.item(r, 3).text())
                    marked_value = int(self.form.tableGroups.item(r, 4).text())
                except ValueError:
                    FreeCAD.Console.PrintError(
                        "ERROR: value conversion failed "
                        "in table to dict: assuming 0 for default, "
                        "1 for marked.\n"
                    )

                group_values_dict[g] = (marked_value, default_value)

            return group_values_dict

        def accept(self):
            group_values_dict = self.convert_table_to_group_dict()

            writeFenicsXDMF.write_fenics_mesh_xdmf(
                self.fem_mesh_obj, self.fileString,
                group_values_dict=group_values_dict)

            FreeCADGui.Control.closeDialog()


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_fenics_mesh(filename)


def export(objectslist, fileString, group_values_dict_nogui=None):
    """
    Called when freecad exports a file.
    group_dict_no_gui: dictionary with group_numbers as keys and tuples
    of (marked_value (default=1), default_value (default=0))
    """
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError(
            "This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return

    if fileString != "":
        fileName, fileExtension = os.path.splitext(fileString)
        if fileExtension.lower() == '.xml':
            FreeCAD.Console.PrintWarning(
                "XML is not designed to save higher order elements.\n")
            FreeCAD.Console.PrintWarning(
                "Reducing order for second order mesh.\n")
            FreeCAD.Console.PrintWarning("Tri6 -> Tri3, Tet10 -> Tet4, etc.\n")
            writeFenicsXML.write_fenics_mesh_xml(obj, fileString)
        elif fileExtension.lower() == '.xdmf':
            mesh_groups = importToolsFem.get_FemMeshObjectMeshGroups(obj)
            if mesh_groups is not ():
                # if there are groups found, make task panel available if GuiUp
                if FreeCAD.GuiUp == 1:
                    panel = WriteXDMFTaskPanel(obj, fileString)
                    FreeCADGui.Control.showDialog(panel)
                else:
                    # create default dict if groupdict_nogui is not None
                    if group_values_dict_nogui is None:
                        group_values_dict_nogui = dict([(g, (1, 0))
                                                        for g in mesh_groups])
                    writeFenicsXDMF.write_fenics_mesh_xdmf(
                        obj, fileString,
                        group_values_dict=group_values_dict_nogui)
            else:
                writeFenicsXDMF.write_fenics_mesh_xdmf(obj, fileString)


# ********* module specific methods *********
def import_fenics_mesh(filename, analysis=None):
    '''insert a FreeCAD FEM Mesh object in the ActiveDocument
    '''
    mesh_data = readFenicsXML.read_fenics_mesh_xml(filename)
    # xdmf not operational

    mesh_name = os.path.basename(os.path.splitext(filename)[0])
    femmesh = importToolsFem.make_femmesh(mesh_data)
    if femmesh:
        mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = femmesh
