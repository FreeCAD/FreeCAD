# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU General Public License (GPL)            *
# *   as published by the Free Software Foundation; either version 3 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU General Public License for more details.                          *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import importlib
import os
import time

import FreeCAD
from nativeifc import ifc_tools
from nativeifc import ifc_psets
from nativeifc import ifc_materials
from nativeifc import ifc_layers
from nativeifc import ifc_status

if FreeCAD.GuiUp:
    import FreeCADGui
    import Arch_rc


PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")


def open(filename):
    """Opens an IFC file"""

    from PySide import QtCore  # lazy loading

    name = os.path.splitext(os.path.basename(filename))[0]
    FreeCAD.IsOpeningIFC = True
    doc = FreeCAD.newDocument()
    doc.Label = name
    FreeCAD.setActiveDocument(doc.Name)
    insert(filename, doc.Name, singledoc=None)
    del FreeCAD.IsOpeningIFC
    QtCore.QTimer.singleShot(100, unset_modified)
    return doc


def insert(
    filename,
    docname,
    strategy=None,
    shapemode=None,
    switchwb=None,
    silent=False,
    singledoc=False,
):
    """Inserts an IFC document in a FreeCAD document.
    Singledoc defines if the produced result is a locked document or not. The
    strategy is:
    - When opening IFC files, locked/unlocked depends on the preferences (default locked)
    - When inserting IFC files, always unlocked (an IFC doc object is created)"""

    from PySide import QtCore  # lazy loading

    strategy, shapemode, switchwb = get_options(strategy, shapemode, switchwb, silent)
    if strategy is None:
        print("Aborted.")
        return
    stime = time.time()
    try:
        document = FreeCAD.getDocument(docname)
    except:
        document = FreeCAD.newDocument()
    if singledoc is None:
        singledoc = PARAMS.GetBool("SingleDoc", True)
    if singledoc:
        prj_obj = ifc_tools.convert_document(document, filename, shapemode, strategy)
        QtCore.QTimer.singleShot(100, toggle_lock_on)
    else:
        prj_obj = ifc_tools.create_document_object(
            document, filename, shapemode, strategy
        )
        QtCore.QTimer.singleShot(100, toggle_lock_off)
    if PARAMS.GetBool("LoadOrphans", True):
        ifc_tools.load_orphans(prj_obj)
    if not silent and PARAMS.GetBool("LoadMaterials", False):
        ifc_materials.load_materials(prj_obj)
    if PARAMS.GetBool("LoadLayers", False):
        ifc_layers.load_layers(prj_obj)
    if PARAMS.GetBool("LoadPsets", False):
        ifc_psets.load_psets(prj_obj)
    document.recompute()
    # print a reference to the IFC file on the console
    if FreeCAD.GuiUp and PARAMS.GetBool("IfcFileToConsole", False):
        if isinstance(prj_obj, FreeCAD.DocumentObject):
            pstr = "FreeCAD.getDocument('{}').{}.Proxy.ifcfile"
            pstr = pstr.format(prj_obj.Document.Name, prj_obj.Name)
        else:
            pstr = "FreeCAD.getDocument('{}').Proxy.ifcfile"
            pstr = pstr.format(prj_obj.Name)
        pstr = "ifcfile = " + pstr
        pstr += " # warning: make sure you know what you are doing when using this!"
        FreeCADGui.doCommand(pstr)
    endtime = "%02d:%02d" % (divmod(round(time.time() - stime, 1), 60))
    fsize = round(os.path.getsize(filename) / 1048576, 2)
    print("Imported", os.path.basename(filename), "(", fsize, "Mb ) in", endtime)
    if FreeCAD.GuiUp and switchwb:
        FreeCADGui.activateWorkbench("BIMWorkbench")
    return document


def get_options(strategy=None, shapemode=None, switchwb=None, silent=False):
    """Shows a dialog to get import options

    shapemode: 0 = full shape
               1 = coin only
               2 = no representation
    strategy:  0 = only root object
               1 = only bbuilding structure,
               2 = all children
    """

    psets = PARAMS.GetBool("LoadPsets", False)
    materials = PARAMS.GetBool("LoadMaterials", False)
    layers = PARAMS.GetBool("LoadLayers", False)
    singledoc = PARAMS.GetBool("SingleDoc", False)
    if strategy is None:
        strategy = PARAMS.GetInt("ImportStrategy", 0)
    if shapemode is None:
        shapemode = PARAMS.GetInt("ShapeMode", 0)
    if switchwb is None:
        switchwb = PARAMS.GetBool("SwitchWB", True)
    if silent:
        return strategy, shapemode, switchwb
    ask = PARAMS.GetBool("AskAgain", False)
    if ask and FreeCAD.GuiUp:
        import FreeCADGui
        from PySide import QtGui

        dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogImport.ui")
        dlg.checkSwitchWB.hide()  # TODO see what to do with this...
        dlg.comboStrategy.setCurrentIndex(strategy)
        dlg.comboShapeMode.setCurrentIndex(shapemode)
        dlg.checkSwitchWB.setChecked(switchwb)
        dlg.checkAskAgain.setChecked(ask)
        dlg.checkLoadPsets.setChecked(psets)
        dlg.checkLoadMaterials.setChecked(materials)
        dlg.checkLoadLayers.setChecked(layers)
        dlg.comboSingleDoc.setCurrentIndex(1 - int(singledoc))
        result = dlg.exec_()
        if not result:
            return None, None, None
        strategy = dlg.comboStrategy.currentIndex()
        shapemode = dlg.comboShapeMode.currentIndex()
        switchwb = dlg.checkSwitchWB.isChecked()
        ask = dlg.checkAskAgain.isChecked()
        psets = dlg.checkLoadPsets.isChecked()
        materials = dlg.checkLoadMaterials.isChecked()
        layers = dlg.checkLoadLayers.isChecked()
        singledoc = dlg.comboSingleDoc.currentIndex()
        PARAMS.SetInt("ImportStrategy", strategy)
        PARAMS.SetInt("ShapeMode", shapemode)
        PARAMS.SetBool("SwitchWB", switchwb)
        PARAMS.SetBool("AskAgain", ask)
        PARAMS.SetBool("LoadPsets", psets)
        PARAMS.SetBool("LoadMaterials", materials)
        PARAMS.SetBool("LoadLayers", layers)
        PARAMS.SetBool("SingleDoc", bool(1 - singledoc))
    return strategy, shapemode, switchwb


def get_project_type(silent=False):
    """Gets the type of project to make"""

    ask = PARAMS.GetBool("ProjectAskAgain", True)
    ptype = PARAMS.GetBool("ProjectFull", False)
    if silent:
        return ptype
    if ask and FreeCAD.GuiUp:
        import FreeCADGui
        from PySide import QtGui

        dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogCreateProject.ui")
        result = dlg.exec_()
        ask = not (dlg.checkBox.isChecked())
        ptype = bool(result)
        PARAMS.SetBool("ProjectAskAgain", ask)
        PARAMS.SetBool("ProjectFull", ptype)
    return ptype


# convenience functions

def toggle_lock_on():

    ifc_status.on_toggle_lock(True, noconvert=True, setchecked=True)

def toggle_lock_off():

    ifc_status.on_toggle_lock(False, noconvert=True, setchecked=True)

def unset_modified():

    try:
        FreeCADGui.ActiveDocument.Modified = False
    except:
        pass
