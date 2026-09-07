# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import os
import time
from datetime import datetime

import FreeCAD

from . import ifc_tools
from . import ifc_psets
from . import ifc_materials
from . import ifc_layers
from . import ifc_status
from . import ifc_summary
from . import ifc_types

if FreeCAD.GuiUp:
    import FreeCADGui
    import Arch_rc  # needed to load the Arch icons, noqa: F401


PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/NativeIFC")


translate = FreeCAD.Qt.translate


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

    strategy, shapemode, switchwb = get_options(filename, strategy, shapemode, switchwb, silent)
    if strategy is None:
        print("Aborted.")
        return
    stime = time.time()
    try:
        document = FreeCAD.getDocument(docname)
    except NameError:
        document = FreeCAD.newDocument()
    if singledoc is None:
        singledoc = PARAMS.GetBool("SingleDoc", True)
    if singledoc:
        prj_obj = ifc_tools.convert_document(document, filename, shapemode, strategy)
        QtCore.QTimer.singleShot(100, toggle_lock_on)
    else:
        prj_obj = ifc_tools.create_document_object(document, filename, shapemode, strategy)
        QtCore.QTimer.singleShot(100, toggle_lock_off)
    if PARAMS.GetBool("LoadOrphans", True):
        ifc_tools.load_orphans(prj_obj)
    if not silent and PARAMS.GetBool("LoadMaterials", False):
        ifc_materials.load_materials(prj_obj)
    if PARAMS.GetBool("LoadLayers", False):
        ifc_layers.load_layers(prj_obj)
    if PARAMS.GetBool("LoadPsets", False):
        ifc_psets.load_psets(prj_obj)
    if PARAMS.GetBool("LoadTypes", False):
        ifc_types.load_types(prj_obj)
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


if FreeCAD.GuiUp:
    from PySide import QtCore

    _SUMMARY_WORKERS = set()

    class _IfcSummaryWorker(QtCore.QThread):
        """Background worker generating IFC summary."""

        summary_metadata = QtCore.Signal(str, int, str, float)
        summary_update = QtCore.Signal(str, object)
        summary_failed = QtCore.Signal(str)

        def __init__(self, filename):
            super().__init__()
            self.filename = filename

        def run(self):
            try:
                ifc_summary.get_summary(
                    self.filename,
                    metadata=self.summary_metadata.emit,
                    update=self.summary_update.emit,
                    cancelled=self.isInterruptionRequested,
                )
            except InterruptedError:
                return
            except Exception as error:
                if not self.isInterruptionRequested():
                    self.summary_failed.emit(f"{type(error).__name__}: {error}")

    class _IfcSummaryReceiver(QtCore.QObject):
        """IFC summary receiver to update dialog."""

        def __init__(self, dialog):
            super().__init__(dialog)
            self.dialog = dialog

        @QtCore.Slot(str, int, str, float)
        def metadata(self, filename, file_size, created, modified):
            _summary_metadata(self.dialog, filename, file_size, created, modified)

        @QtCore.Slot(str, object)
        def update(self, name, value):
            _summary_update(self.dialog, name, value)

        @QtCore.Slot(str)
        def failed(self, message):
            _summary_failed(self.dialog, message)


def _summary_metadata(dialog, filename, file_size, created, modified):
    """IFC summary metadata on dialog."""

    dialog.summaryFile.setText(filename or "—")
    dialog.summarySize.setText(f"{file_size / (1024 * 1024):.1f} MB" if file_size else "—")
    dialog.summaryCreated.setText(created.replace("T", " ") if created else "—")
    dialog.summaryModified.setText(
        datetime.fromtimestamp(modified).strftime("%Y-%m-%d %H:%M:%S") if modified else "—"
    )


def _summary_update(dialog, name, value):
    """IFC summary fields displayed on dialog as they become available."""

    labels = {
        "schema": dialog.summarySchema,
        "projects": dialog.summaryProjects,
        "sites": dialog.summarySites,
        "buildings": dialog.summaryBuildings,
        "storeys": dialog.summaryStoreys,
        "products": dialog.summaryProducts,
        "types": dialog.summaryTypes,
        "property_sets": dialog.summaryPropertySets,
        "materials": dialog.summaryMaterials,
        "layers": dialog.summaryLayers,
    }

    label = labels.get(name)

    if label is None:
        return

    if name == "schema":
        label.setText(str(value) if value else "—")
    else:
        label.setText(f"{int(value):,}")


def _summary_failed(dialog, message):
    """Non-blocking IFC summary failure."""

    dialog.groupSummary.setTitle(translate("BIM", "IFC file content summary — Unable to read"))
    dialog.groupSummary.setToolTip(message)

    # Keep already available file metadata visible.
    # Clear only incomplete content values.
    for name in (
        "summarySchema",
        "summaryProjects",
        "summarySites",
        "summaryBuildings",
        "summaryStoreys",
        "summaryProducts",
        "summaryTypes",
        "summaryPropertySets",
        "summaryMaterials",
        "summaryLayers",
    ):
        getattr(dialog, name).setText("—")

    FreeCAD.Console.PrintError(f"BIM IFC import summary failure: {message}\n")


def _summary_finished(dialog):
    """Mark summary as ready."""

    if not getattr(dialog, "summary_worker", None):
        return

    dialog.groupSummary.setTitle(translate("BIM", "IFC file content summary"))


def get_options(filename=None, strategy=None, shapemode=None, switchwb=None, silent=False):
    """Show a dialog to get IFC import options.

    Parameters
    ----------
    filename : str, optional
        IFC file path used to display file content summary.
    strategy : int, optional
        Import strategy.
            0 = only root object
            1 = only building structure
            2 = all children
    shapemode : int, optional
        Shape loading mode.
            0 = full shape
            1 = coin only
            2 = no representation
    switchwb : bool, optional
        Whether to switch to BIM workbench after import.
    silent : bool, optional
        Skip the dialog.

    Returns
    -------
    tuple
        Import strategy, shape mode, and workbench switch setting.
    """

    psets = PARAMS.GetBool("LoadPsets", False)
    types = PARAMS.GetBool("LoadTypes", False)
    materials = PARAMS.GetBool("LoadMaterials", False)
    layers = PARAMS.GetBool("LoadLayers", False)
    singledoc = PARAMS.GetBool("SingleDoc", False)
    if strategy is None:
        strategy = PARAMS.GetInt("ImportStrategy", 0)
    if shapemode is None:
        shapemode = PARAMS.GetInt("ShapeMode", 1)
    if switchwb is None:
        switchwb = PARAMS.GetBool("SwitchWB", True)
    if silent:
        return strategy, shapemode, switchwb
    ask = PARAMS.GetBool("AskAgain", True)
    if ask and FreeCAD.GuiUp:
        import FreeCADGui
        from PySide import QtCore, QtGui

        dlg = FreeCADGui.PySideUic.loadUi(":/ui/dialogImport.ui")
        dlg.checkSwitchWB.hide()  # TODO see what to do with this...
        dlg.comboStrategy.setCurrentIndex(strategy)
        dlg.comboShapeMode.setCurrentIndex(shapemode)
        dlg.checkSwitchWB.setChecked(switchwb)
        dlg.checkAskAgain.setChecked(ask)
        dlg.checkLoadPsets.setChecked(psets)
        dlg.checkLoadTypes.setChecked(types)
        dlg.checkLoadMaterials.setChecked(materials)
        dlg.checkLoadLayers.setChecked(layers)
        dlg.comboSingleDoc.setCurrentIndex(1 - int(singledoc))

        if filename:
            dlg.groupSummary.setTitle(
                translate("BIM", "IFC file content summary — Reading IFC file…")
            )

            summary_worker = _IfcSummaryWorker(filename)
            summary_receiver = _IfcSummaryReceiver(dlg)

            summary_worker.summary_metadata.connect(summary_receiver.metadata)
            summary_worker.summary_update.connect(summary_receiver.update)
            summary_worker.summary_failed.connect(summary_receiver.failed)

            summary_worker.finished.connect(lambda: _summary_finished(dlg))

            def _cleanup_worker():
                _SUMMARY_WORKERS.discard(summary_worker)

                if getattr(dlg, "summary_worker", None) is summary_worker:
                    dlg.summary_worker = None

                summary_worker.deleteLater()

            summary_worker.finished.connect(_cleanup_worker)

            _SUMMARY_WORKERS.add(summary_worker)

            # Keep explicit references for dialog lifetime.
            dlg.summary_worker = summary_worker
            dlg.summary_receiver = summary_receiver

            summary_worker.start()

        else:
            dlg.groupSummary.setTitle(
                translate("BIM", "IFC file content summary — No IFC file selected")
            )

        def stop_summary_worker():
            worker = getattr(dlg, "summary_worker", None)

            if worker is not None and worker.isRunning():
                worker.requestInterruption()
                dlg.summary_worker = None

        QtGui.QApplication.setOverrideCursor(QtCore.Qt.ArrowCursor)
        result = dlg.exec_()
        stop_summary_worker()
        QtGui.QApplication.restoreOverrideCursor()

        if not result:
            return None, None, None
        strategy = dlg.comboStrategy.currentIndex()
        shapemode = dlg.comboShapeMode.currentIndex()
        switchwb = dlg.checkSwitchWB.isChecked()
        ask = dlg.checkAskAgain.isChecked()
        psets = dlg.checkLoadPsets.isChecked()
        types = dlg.checkLoadTypes.isChecked()
        materials = dlg.checkLoadMaterials.isChecked()
        layers = dlg.checkLoadLayers.isChecked()
        singledoc = dlg.comboSingleDoc.currentIndex()
        PARAMS.SetInt("ImportStrategy", strategy)
        PARAMS.SetInt("ShapeMode", shapemode)
        PARAMS.SetBool("SwitchWB", switchwb)
        PARAMS.SetBool("AskAgain", ask)
        PARAMS.SetBool("LoadPsets", psets)
        PARAMS.SetBool("LoadTypes", types)
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
    except AttributeError:
        pass
