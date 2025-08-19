# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

from __future__ import print_function

"""The BIM library tool"""

import os
import sys
import tempfile

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")

FILTERS = [
    "*.fcstd",
    "*.FCStd",
    "*.FCSTD",
    "*.stp",
    "*.STP",
    "*.step",
    "*.STEP",
    "*.brp",
    "*.BRP",
    "*.brep",
    "*.BREP",
    "*.ifc",
    "*.IFC",
    "*.sat",
    "*.SAT",
]
TEMPLIBPATH = os.path.join(FreeCAD.getUserAppDataDir(), "BIM", "OfflineLibrary")
THUMBNAILSPATH = os.path.join(TEMPLIBPATH, "__thumbcache__")
LIBRARYURL = "https://github.com/FreeCAD/FreeCAD-library/tree/master"
RAWURL = LIBRARYURL.replace("/tree", "/raw")
LIBINDEXFILE = "OfflineLibrary.py"
USE_API = True  # True to use github API instead of web fetching... Way faster
REFRESH_INTERVAL = (
    3600  # Min seconds between allowing a new API calls (3600 = one hour)
)


# TODO as https://github.com/yorikvanhavre/BIM_Workbench/pull/77

# All the print() statements in your code should be replaced by
# FreeCAD.Console.PrintMessage() or FreeCAD.Console.PrintWarning() or
# FreeCAD.Console.PrintError() and the text should be placed in a translate()
# function and "\n" should be added to it.
# Example FreeCAD.Console.PrintError(translate("BIM","Please save the document first")+"\n")

# It would be cool if the preview image would have a max width of the available
# column width, so if the task column is smaller than the image, it gets smaller
# to fit the space. I don't remember exactly how to do that, but it should be
# findable in QDesigner


class BIM_Library:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Library",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Library", "Objects Library"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Library", "Opens the objects library"),
        }

    def Activated(self):

        # trying to locate the parts library
        pr = FreeCAD.ParamGet("User parameter:Plugins/parts_library")
        libok = False
        self.librarypath = pr.GetString("destination", "")
        if self.librarypath:
            if os.path.exists(self.librarypath):
                libok = True
        else:
            # check if the library is at the standard addon location
            addondir = os.path.join(FreeCAD.getUserAppDataDir(), "Mod", "parts_library")
            if os.path.exists(addondir):
                # save file paths with forward slashes even on windows
                pr.SetString("destination", addondir.replace("\\", "/"))
                libok = True
        task = FreeCADGui.Control.showDialog(BIM_Library_TaskPanel(offlinemode=libok))
        task.setDocumentName(FreeCAD.ActiveDocument.Name)
        task.setAutoCloseOnDeletedDocument(True)


class BIM_Library_TaskPanel:

    def __init__(self, offlinemode=False):

        from PySide import QtGui

        self.mainDocName = FreeCAD.Gui.ActiveDocument.Document.Name
        self.previewDocName = "Viewer"

        self.linked = False

        self.librarypath = FreeCAD.ParamGet(
            "User parameter:Plugins/parts_library"
        ).GetString("destination", "")
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogLibrary.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/BIM_Library.svg"))

        # setting up a flat (no directories) file model for search
        self.filemodel = QtGui.QStandardItemModel()
        self.filemodel.setColumnCount(1)

        # setting up a directory model that shows only fcstd, step and brep
        self.dirmodel = LibraryModel()
        self.dirmodel.setRootPath(self.librarypath)
        self.dirmodel.setNameFilters(self.getFilters())
        self.dirmodel.setNameFilterDisables(False)
        self.form.tree.setModel(self.dirmodel)
        self.form.buttonInsert.clicked.connect(self.insert)
        self.form.buttonLink.clicked.connect(self.link)

        self.modelmode = 1  # 0 = File search, 1 = Dir mode

        # Don't show columns for size, file type, and last modified
        self.form.tree.setHeaderHidden(True)
        self.form.tree.hideColumn(1)
        self.form.tree.hideColumn(2)
        self.form.tree.hideColumn(3)
        self.form.tree.setRootIndex(self.dirmodel.index(self.librarypath))
        self.form.searchBox.textChanged.connect(self.onSearch)

        # external search
        sites = {
            "BimObject": [
                "bimobject.png",
                "https://www.bimobject.com/en/product?filetype=8&freetext=",
            ],
            "NBS Library": [
                "nbslibrary.png",
                "https://www.nationalbimlibrary.com/en/search/?facet=Xo-P0w&searchTerm=",
            ],
            "BIMTool": [
                "bimtool.png",
                "https://www.bimtool.com/Catalog.aspx?criterio=",
            ],
            "3DFindIt": ["3dfindit.svg", "https://www.3dfindit.com/textsearch?q="],
            "GrabCAD": [
                "grabcad.svg",
                "https://grabcad.com/library?softwares=step-slash-iges&query=",
            ],
        }
        for k, v in sites.items():
            self.form.comboSearch.addItem(QtGui.QIcon(":/icons/"+v[0]), k, v[1])
        self.form.comboSearch.currentIndexChanged.connect(self.onExternalSearch)

        # retrieve preferences
        self.form.checkOnline.toggled.connect(self.onCheckOnline)
        self.form.checkOnline.setChecked(
            PARAMS.GetBool("LibraryOnline", not offlinemode)
        )
        self.form.checkFCStdOnly.toggled.connect(self.onCheckFCStdOnly)
        self.form.checkFCStdOnly.setChecked(PARAMS.GetBool("LibraryFCStdOnly", False))
        self.form.checkWebSearch.toggled.connect(self.onCheckWebSearch)
        self.form.checkWebSearch.setChecked(PARAMS.GetBool("LibraryWebSearch", False))
        self.form.check3DPreview.toggled.connect(self.onCheck3DPreview)
        self.form.check3DPreview.setChecked(PARAMS.GetBool("3DPreview", False))

        # collapsables
        if PARAMS.GetBool("LibraryPreview", False):
            self.form.framePreview.show()
            self.form.buttonPreview.setText(translate("BIM", "Preview") + " ▼")
        else:
            self.form.framePreview.hide()
            self.form.buttonPreview.setText(translate("BIM", "Preview") + " ▸")
        self.form.buttonPreview.clicked.connect(self.onButtonPreview)
        self.form.frameOptions.hide()
        self.form.buttonOptions.setText(translate("BIM", "Options") + " ▸")
        self.form.buttonOptions.clicked.connect(self.onButtonOptions)

        # saving functionality, is disabled for now
        self.form.buttonSave.hide()
        self.form.checkThumbnail.hide()
        # self.form.buttonSave.clicked.connect(self.addtolibrary)
        # self.form.checkThumbnail.toggled.connect(self.onCheckThumbnail)
        # self.form.checkThumbnail.setChecked(PARAMS.GetBool("SaveThumbnails",False))
        # self.fcstdCB = QtGui.QCheckBox('FCStd')
        # self.fcstdCB.setCheckState(QtCore.Qt.Checked)
        # self.fcstdCB.setEnabled(False)
        # self.fcstdCB.hide()
        # self.stepCB = QtGui.QCheckBox('STEP')
        # self.stepCB.setCheckState(QtCore.Qt.Checked)
        # self.stepCB.hide()
        # self.stlCB = QtGui.QCheckBox('STL')
        # self.stlCB.setCheckState(QtCore.Qt.Checked)
        # self.stlCB.hide()

        # update the tree
        self.onCheckOnline()

    def onItemSelected(self, selected, deselected):
        """Generates and displays needed previews"""

        from PySide import QtGui

        if not selected:
            return
        index = selected[0].indexes()[0]
        if self.modelmode == 1:
            path = self.dirmodel.filePath(index)
        else:
            path = self.filemodel.itemFromIndex(index).toolTip()
        if path.startswith(":github"):
            path = RAWURL + "/" + path[7:]
        thumb = self.getThumbnail(path)
        if thumb:
            px = QtGui.QPixmap(thumb)
        else:
            px = QtGui.QPixmap()
        self.form.framePreview.setPixmap(px)

        if False:
            # TO BE REFACTORED

            import Part
            import zipfile

            self.previewOn = PARAMS.GetBool("3DPreview", False)
            try:
                self.path = self.dirmodel.filePath(index)
            except:
                self.path = self.previousIndex
                print(self.path)
            self.isFile = os.path.isfile(self.path)
            # if the 3D preview checkbox is on ticked, show the preview
            if self.previewOn == True or self.linked == True:
                if self.isFile == True:
                    # close a non linked preview document
                    if self.linked == False:
                        try:
                            FreeCAD.closeDocument(self.previewDocName)
                        except:
                            pass
                    # create different kinds of previews based on file type
                    if (
                        self.path.lower().endswith(".stp")
                        or self.path.lower().endswith(".step")
                        or self.path.lower().endswith(".brp")
                        or self.path.lower().endswith(".brep")
                    ):
                        self.previewDocName = "Viewer"
                        FreeCAD.newDocument(self.previewDocName)
                        FreeCAD.setActiveDocument(self.previewDocName)
                        Part.show(Part.read(self.path))
                        FreeCADGui.SendMsgToActiveView("ViewFit")
                    elif self.path.lower().endswith(".fcstd"):
                        openedDoc = FreeCAD.openDocument(self.path)
                        FreeCADGui.SendMsgToActiveView("ViewFit")
                        self.previewDocName = FreeCAD.ActiveDocument.Name
                        thumbnailSave = PARAMS.GetBool("SaveThumbnails", False)
                        if thumbnailSave == True:
                            FreeCAD.ActiveDocument.save()
            if self.linked == False:
                self.previousIndex = self.path

            # create a 2D image preview
            if self.path.lower().endswith(".fcstd"):
                zfile = zipfile.ZipFile(self.path)
                files = zfile.namelist()
                # check for meta-file if it's really a FreeCAD document
                if files[0] == "Document.xml":
                    image = "thumbnails/Thumbnail.png"
                    if image in files:
                        image = zfile.read(image)
                        thumbfile = tempfile.mkstemp(suffix=".png")[1]
                        thumb = open(thumbfile, "wb")
                        thumb.write(image)
                        thumb.close()
                        im = QtGui.QPixmap(thumbfile)
                        self.form.framePreview.setPixmap(im)
                        return self.previewDocName, self.previousIndex, self.linked
            self.form.framePreview.clear()
            return self.previewDocName, self.previousIndex, self.linked

    def link(self, index):

        # check if the main document is open
        try:
            # check if the working document is saved
            if FreeCAD.getDocument(self.mainDocName).FileName == "":
                FreeCAD.Console.PrintWarning(translate("BIM","Save the working file before linking.")+"\n")
            else:
                self.previewOn = PARAMS.GetBool("3DPreview", False)
                self.linked = True
                if self.previewOn != True:
                    BIM_Library_TaskPanel.clicked(self, index, previewDocName="Viewer")
                self.librarypath = ""
                # save the file prior to linking
                BIM_Library_TaskPanel.addtolibrary(self)
                # link a document if it has been previously saved
                if self.fileDialog[0] != "":
                    FreeCADGui.Selection.clearSelection()
                    # link only root objects
                    for obj in FreeCAD.ActiveDocument.RootObjects:
                        FreeCADGui.Selection.addSelection(obj)
                    objects = FreeCADGui.Selection.getSelection()
                    # tries to create a link for each object in the selection
                    for obj in objects:
                        try:
                            link = (
                                FreeCAD.getDocument(self.mainDocName)
                                .addObject("App::Link", "Link")
                                .setLink(obj)
                            )
                            # FreeCAD.getDocument(self.mainDocName).getObject('Link').Label=FreeCAD.ActiveDocument.ActiveObject.Label
                            FreeCAD.getDocument(self.mainDocName).getObject(
                                link
                            ).Label = FreeCAD.ActiveDocument.ActiveObject.Label
                        except:
                            pass
                    FreeCAD.setActiveDocument(self.mainDocName)
                    self.librarypath = FreeCAD.ParamGet(
                        "User parameter:Plugins/parts_library"
                    ).GetString("destination", "")
                    self.linked = False
                    return self.linked
        except:
            FreeCAD.Console.PrintWarning(
                translate("BIM","It is not possible to link because the main document is closed.")+"\n")

    def addtolibrary(self):
        # DISABLED

        import os
        import Mesh
        import Part

        self.fileDialog = QtGui.QFileDialog.getSaveFileName(
            None, "Save As", self.librarypath
        )
        #print(self.fileDialog[0])
        # check if file saving has been canceled and save .fcstd, .step and .stl copies
        if self.fileDialog[0] != "":
            # remove the file extension from the file path
            fileName = os.path.splitext(self.fileDialog[0])[0]
            FCfilename = fileName + ".fcstd"
            FreeCAD.ActiveDocument.saveAs(FCfilename)
            if self.stepCB.isChecked() or self.stlCB.isChecked():
                toexport = []
                objs = FreeCAD.ActiveDocument.Objects
                for obj in objs:
                    if obj.ViewObject.Visibility == True:
                        toexport.append(obj)
                if self.stepCB.isChecked() and self.linked == False:
                    STEPfilename = fileName + ".step"
                    Part.export(toexport, STEPfilename)
                if self.stlCB.isChecked() and self.linked == False:
                    STLfilename = fileName + ".stl"
                    Mesh.export(toexport, STLfilename)
        return self.fileDialog[0]

    def onSearch(self, text):

        if text:
            self.setSearchModel(text)
        else:
            self.setFileModel()

    def setSearchModel(self, text):

        from PySide import QtGui

        def add_line(f, dp):
            if self.isAllowed(f) and (text.lower() in f.lower()):
                it = QtGui.QStandardItem(f)
                it.setToolTip(os.path.join(dp, f))
                self.filemodel.appendRow(it)
                if f.lower().endswith(".fcstd"):
                    it.setIcon(QtGui.QIcon(":icons/freecad-doc.png"))
                elif f.lower().endswith(".ifc"):
                    it.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
                else:
                    it.setIcon(QtGui.QIcon(":/icons/Part_document.svg"))

        self.form.tree.setModel(self.filemodel)
        self.filemodel.clear()
        if self.form.checkOnline.isChecked():
            res = self.getOfflineLib(structured=True)
            for i in range(len(res[0])):
                add_line(res[0][i], res[2][i])
        else:
            res = os.walk(self.librarypath)
            for dp, dn, fn in res:
                for f in fn:
                    if not os.path.isdir(os.path.join(dp, f)):
                        add_line(f, dp)
        self.modelmode = 0

    def getFilters(self):

        if self.form.checkFCStdOnly.isChecked():
            return FILTERS
        else:
            return FILTERS[:3]

    def isAllowed(self, filename):

        e = os.path.splitext(filename)[1]
        if e in [f[1:] for f in FILTERS]:
            if e in [f[1:] for f in self.getFilters()]:
                return True
            else:
                return False
        else:
            return True

    def setFileModel(self):

        # self.form.tree.clear()
        self.form.tree.setModel(self.dirmodel)
        self.dirmodel.setRootPath(self.librarypath)
        self.dirmodel.setNameFilters(self.getFilters())
        self.dirmodel.setNameFilterDisables(False)
        self.form.tree.setRootIndex(self.dirmodel.index(self.librarypath))
        self.modelmode = 1
        self.form.tree.setHeaderHidden(True)
        self.form.tree.hideColumn(1)
        self.form.tree.hideColumn(2)
        self.form.tree.hideColumn(3)
        self.form.tree.selectionModel().selectionChanged.connect(self.onItemSelected)

    def setOnlineModel(self):

        from PySide import QtGui

        def addItems(root, d, path):
            for k, v in d.items():
                if self.isAllowed(k):
                    it = QtGui.QStandardItem(k)
                    root.appendRow(it)
                    it.setToolTip(path + "/" + k)
                    if isinstance(v, dict):
                        it.setIcon(
                            QtGui.QIcon.fromTheme(
                                "folder", QtGui.QIcon(":/icons/Group.svg")
                            )
                        )
                        addItems(it, v, path + "/" + k)
                        it.setToolTip("")
                    elif k.lower().endswith(".fcstd"):
                        it.setIcon(QtGui.QIcon(":icons/freecad-doc.png"))
                    elif k.lower().endswith(".ifc"):
                        it.setIcon(QtGui.QIcon(":/icons/IFC.svg"))
                    else:
                        it.setIcon(QtGui.QIcon(":/icons/Part_document.svg"))
        self.form.tree.setModel(self.filemodel)
        self.filemodel.clear()
        d = self.getOfflineLib()
        addItems(self.filemodel, d, ":github")
        self.modelmode = 0
        self.form.tree.selectionModel().selectionChanged.connect(self.onItemSelected)

    def getOfflineLib(self, structured=False):

        def addDir(d, root):
            fn = []
            dn = []
            dp = []
            for k, v in d.items():
                if isinstance(v, dict) and v:
                    fn2, dn2, dp2 = addDir(v, root + "/" + k)
                    fn.extend(fn2)
                    dn.extend(dn2)
                    dp.extend(dp2)
                elif v:
                    fn.append(k)
                    dn.append(root)
                    dp.append(root)
            return fn, dn, dp

        templibfile = os.path.join(TEMPLIBPATH, LIBINDEXFILE)
        if not os.path.exists(templibfile):
            FreeCAD.Console.PrintError(
                translate("BIM", "No structure in cache. Refresh required.") + "\n"
            )
            return {}
        import sys

        sys.path.append(TEMPLIBPATH)
        import OfflineLibrary

        d = OfflineLibrary.library
        if structured:
            return addDir(d, ":github")
        else:
            return d

    def urlencode(self, text):

        #print(text, type(text))
        if sys.version_info.major < 3:
            import urllib

            return urllib.quote_plus(text)
        else:
            import urllib.parse

            return urllib.parse.quote_plus(text)

    def openUrl(self, url):

        from PySide import QtGui

        s = PARAMS.GetBool("LibraryWebSearch", False)
        if s:
            import WebGui

            WebGui.openBrowser(url)
        else:
            QtGui.QDesktopServices.openUrl(url)

    def needsFullSpace(self):

        return True

    def getStandardButtons(self):

        from PySide import QtGui

        return QtGui.QDialogButtonBox.Close

    def reject(self):

        if hasattr(self, "box") and self.box:
            self.box.off()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()

    def insert(self, index=None):

        # check if the main document is open
        try:
            FreeCAD.setActiveDocument(self.mainDocName)
        except:
            FreeCAD.Console.PrintError(
                translate(
                    "BIM",
                    "It is not possible to insert this object because the document has been closed.",
                )
                + "\n"
            )
            return
        if self.previewDocName in FreeCAD.listDocuments().keys():
            FreeCAD.closeDocument(self.previewDocName)
        if not index:
            index = self.form.tree.selectedIndexes()
            if not index:
                return
            index = index[0]
        if self.modelmode == 1:
            path = self.dirmodel.filePath(index)
        else:
            path = self.filemodel.itemFromIndex(index).toolTip()
        if path.startswith(":github"):
            path = self.download(RAWURL + "/" + path[7:])
        before = FreeCAD.ActiveDocument.Objects
        self.name = os.path.splitext(os.path.basename(path))[0]
        ext = os.path.splitext(path.lower())[1]
        if ext in [".stp", ".step", ".brp", ".brep"]:
            self.place(path)
        elif ext == ".fcstd":
            FreeCADGui.ActiveDocument.mergeProject(path)
            from draftutils import todo

            todo.ToDo.delay(self.reject, None)
        elif ext == ".ifc":
            from importers import importIFC

            importIFC.ZOOMOUT = False
            importIFC.insert(path, FreeCAD.ActiveDocument.Name)
            from draftutils import todo

            todo.ToDo.delay(self.reject, None)
        elif ext in [".sat", ".sab"]:
            try:
                # InventorLoader addon
                import importerIL
            except ImportError:
                try:
                    # CADExchanger addon
                    import CadExchangerIO
                except ImportError:
                    FreeCAD.Console.PrintError(
                        translate(
                            "BIM",
                            "Error: Unable to import SAT files - InventorLoader or CadExchanger addon must be installed",
                        )
                        + "\n"
                    )
                else:
                    path = CadExchangerIO.insert(
                        path, FreeCAD.ActiveDocument.Name, returnpath=True
                    )
                    self.place(path)
            else:
                path = importerIL.insert(path, FreeCAD.ActiveDocument.Name)
        FreeCADGui.Selection.clearSelection()
        for o in FreeCAD.ActiveDocument.Objects:
            if not o in before:
                FreeCADGui.Selection.addSelection(o)
        FreeCADGui.SendMsgToActiveView("ViewSelection")

    def download(self, url):

        import urllib.request

        filepath = os.path.join(TEMPLIBPATH, url.split("/")[-1])
        url = url.replace(" ", "%20")
        if not os.path.exists(filepath):
            from PySide import QtCore, QtGui

            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            u = urllib.request.urlopen(url)
            if not u:
                FreeCAD.Console.PrintError(
                    translate("BIM", "Error: Unable to download") + " " + url + "\n"
                )
            b = u.read()
            f = open(filepath, "wb")
            f.write(b)
            f.close()
            QtGui.QApplication.restoreOverrideCursor()
        return filepath

    def place(self, path):

        import Part
        import WorkingPlane

        self.shape = Part.read(path)
        if hasattr(FreeCADGui, "Snapper"):
            try:
                import DraftTrackers
            except Exception:
                import draftguitools.gui_trackers as DraftTrackers
            self.box = DraftTrackers.ghostTracker(
                self.shape, dotted=True, scolor=(0.0, 0.0, 1.0), swidth=1.0
            )
            self.delta = self.shape.BoundBox.Center
            self.box.move(self.delta)
            self.box.on()
            WorkingPlane.get_working_plane()
            self.origin = self.makeOriginWidget()
            FreeCADGui.Snapper.getPoint(
                movecallback=self.mouseMove,
                callback=self.mouseClick,
                extradlg=self.origin,
            )
        else:
            Part.show(self.shape)

    def makeOriginWidget(self):

        from PySide import QtGui

        w = QtGui.QWidget()
        w.setWindowTitle(translate("BIM", "Insertion point"))
        w.setWindowIcon(
            QtGui.QIcon(
                os.path.join(os.path.dirname(__file__), "icons", "BIM_Library.svg")
            )
        )
        l = QtGui.QVBoxLayout()
        w.setLayout(l)
        c = QtGui.QComboBox()
        c.ObjectName = "comboOrigin"
        w.comboOrigin = c
        c.addItems(
            [
                translate("BIM", "Origin"),
                translate("BIM", "Top left"),
                translate("BIM", "Top center"),
                translate("BIM", "Top right"),
                translate("BIM", "Middle left"),
                translate("BIM", "Middle center"),
                translate("BIM", "Middle right"),
                translate("BIM", "Bottom left"),
                translate("BIM", "Bottom center"),
                translate("BIM", "Bottom right"),
            ]
        )
        c.setCurrentIndex(PARAMS.GetInt("LibraryDefaultInsert", 0))
        c.currentIndexChanged.connect(self.storeInsert)
        l.addWidget(c)
        return w

    def storeInsert(self, index):

        PARAMS.SetInt("LibraryDefaultInsert", index)

    def mouseMove(self, point, info):

        self.box.move(point.add(self.getDelta()))

    def mouseClick(self, point, info):

        if point:
            import Arch

            self.box.off()
            self.shape.translate(point.add(self.getDelta()))
            obj = Arch.makeEquipment()
            obj.Shape = self.shape
            obj.Label = self.name
        self.reject()

    def getDelta(self):

        d = FreeCAD.Vector(
            -self.shape.BoundBox.Center.x, -self.shape.BoundBox.Center.y, 0
        )
        idx = self.origin.comboOrigin.currentIndex()
        if idx <= 0:
            return FreeCAD.Vector()
        elif idx == 1:
            return d.add(
                FreeCAD.Vector(
                    self.shape.BoundBox.XLength / 2, -self.shape.BoundBox.YLength / 2, 0
                )
            )
        elif idx == 2:
            return d.add(FreeCAD.Vector(0, -self.shape.BoundBox.YLength / 2, 0))
        elif idx == 3:
            return d.add(
                FreeCAD.Vector(
                    -self.shape.BoundBox.XLength / 2,
                    -self.shape.BoundBox.YLength / 2,
                    0,
                )
            )
        elif idx == 4:
            return d.add(FreeCAD.Vector(self.shape.BoundBox.XLength / 2, 0, 0))
        elif idx == 5:
            return d
        elif idx == 6:
            return d.add(FreeCAD.Vector(-self.shape.BoundBox.XLength / 2, 0, 0))
        elif idx == 7:
            return d.add(
                FreeCAD.Vector(
                    self.shape.BoundBox.XLength / 2, self.shape.BoundBox.YLength / 2, 0
                )
            )
        elif idx == 8:
            return d.add(FreeCAD.Vector(0, self.shape.BoundBox.YLength / 2, 0))
        elif idx == 9:
            return d.add(
                FreeCAD.Vector(
                    -self.shape.BoundBox.XLength / 2, self.shape.BoundBox.YLength / 2, 0
                )
            )

    def getOnlineContentsAPI(self, url):
        """same as getOnlineContents but uses github API (faster)"""

        import json
        import requests

        result = {}
        count = 0
        r = requests.get(
            "https://api.github.com/repos/FreeCAD/FreeCAD-library/git/trees/master?recursive=1"
        )
        if r.ok:
            j = json.loads(r.content)
            if j["truncated"]:
                print(
                    "WARNING: The fetched content exceeds maximum GitHub allowance and is truncated"
                )
            t = j["tree"]
            for f in t:
                path = f["path"].split("/")
                if f["type"] == "tree":
                    name = None
                else:
                    name = path[-1]
                    path = path[:-1]
                host = result
                for fp in path:
                    if fp in host:
                        host = host[fp]
                    else:
                        host[fp] = {}
                        host = host[fp]
                if name:
                    for ft in self.getFilters():
                        if name.endswith(ft[1:]):
                            break
                    else:
                        continue
                    host[name] = name
                    count += 1
        else:
            FreeCAD.Console.PrintError(
                translate("BIM", "Could not fetch library contents") + "\n"
            )
        # print("result:",result)
        if not result:
            FreeCAD.Console.PrintError(
                translate("BIM", "No results fetched from online library") + "\n"
            )
        else:
            FreeCAD.Console.PrintLog("BIM Library: Reloaded " + str(count) + " files\n")
        return result

    def onCheckOnline(self, state=None):
        """if the Online checkbox is clicked"""

        import datetime

        if state == None:
            state = self.form.checkOnline.isChecked()
        # save state
        PARAMS.SetBool("LibraryOnline", state)
        if state:
            # online
            if USE_API:
                needrefresh = True
                timestamp = datetime.datetime.now()
                if os.path.exists(os.path.join(TEMPLIBPATH, LIBINDEXFILE)):
                    stored = PARAMS.GetUnsigned("LibraryTimeStamp", 0)
                    if stored:
                        stored = datetime.datetime.fromtimestamp(stored)
                        if (timestamp - stored).total_seconds() < REFRESH_INTERVAL:
                            needrefresh = False
                if needrefresh:
                    PARAMS.SetUnsigned("LibraryTimeStamp", int(timestamp.timestamp()))
                    self.onRefresh()
                else:
                    FreeCAD.Console.PrintLog("BIM Library: Using cached library\n")
            self.setOnlineModel()
            self.form.buttonLink.setEnabled(False)
        else:
            # offline
            self.setFileModel()
            self.form.buttonLink.setEnabled(True)

    def onRefresh(self):
        """refreshes the tree"""

        from PySide import QtCore, QtGui

        def writeOfflineLib():
            if USE_API:
                rootfiles = self.getOnlineContentsAPI(LIBRARYURL)
            if rootfiles:
                templibfile = os.path.join(TEMPLIBPATH, LIBINDEXFILE)
                os.makedirs(TEMPLIBPATH, exist_ok=True)
                tf = open(templibfile, "w", encoding="utf8")
                tf.write("library=" + str(rootfiles) + "\n")
                tf.close()
                self.setOnlineModel()

        reply = PARAMS.GetBool("LibraryWarning", False)
        if not reply:
            reply = QtGui.QMessageBox.information(
                None, "", translate("BIM", "Warning, this can take several minutes!")
            )
        if reply:
            PARAMS.SetBool("LibraryWarning", True)
            self.form.setEnabled(False)
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
            self.form.repaint()
            QtGui.QApplication.processEvents()
            QtCore.QTimer.singleShot(1, writeOfflineLib)
            self.form.setEnabled(True)
            QtGui.QApplication.restoreOverrideCursor()
        else:
            self.setOnlineModel()

    def onCheckFCStdOnly(self, state):
        """if the FCStd only checkbox is clicked"""

        # save state
        PARAMS.SetBool("LibraryFCStdOnly", state)
        self.dirmodel.setNameFilters(self.getFilters())
        self.onCheckOnline(self.form.checkOnline.isChecked())

    def onCheckWebSearch(self, state):
        """if the web search checkbox is clicked"""

        # save state
        PARAMS.SetBool("LibraryWebSearch", state)

    def onCheck3DPreview(self, state):
        """if the 3D preview checkbox is clicked"""

        # save state
        PARAMS.SetBool("3DPreview", state)
        self.previewOn = PARAMS.GetBool("3DPreview", False)
        try:
            FreeCAD.closeDocument(self.previewDocName)
        except:
            pass
        if self.previewOn == True:
            self.previewDocName = "Viewer"
            self.doc = FreeCAD.newDocument(self.previewDocName)
            FreeCADGui.ActiveDocument.ActiveView.viewIsometric()
            return self.previewDocName

    def onCheckThumbnail(self, state):
        """if the thumbnail checkbox is clicked"""

        # save state
        PARAMS.SetBool("SaveThumbnails", state)

    def onButtonOptions(self):
        """hides/shows the options"""

        if self.form.frameOptions.isVisible():
            self.form.frameOptions.hide()
            self.form.buttonOptions.setText(translate("BIM", "Options") + " ▸")
        else:
            self.form.frameOptions.show()
            self.form.buttonOptions.setText(translate("BIM", "Options") + " ▼")

    def onButtonPreview(self):
        """hides/shows the preview"""

        if self.form.framePreview.isVisible():
            self.form.framePreview.hide()
            self.form.buttonPreview.setText(translate("BIM", "Preview") + " ▸")
            PARAMS.SetBool("LibraryPreview", False)
        else:
            self.form.framePreview.show()
            self.form.buttonPreview.setText(translate("BIM", "Preview") + " ▼")
            PARAMS.SetBool("LibraryPreview", True)

    def getThumbnail(self, filepath):
        """returns a thumbnail image path for a given file path"""

        import urllib.request
        import urllib.parse
        import zipfile
        import io

        if not filepath.lower().endswith(".fcstd"):
            return None
        iconname = self.getHashname(filepath)
        iconfile = os.path.join(THUMBNAILSPATH, iconname)
        if os.path.exists(iconfile):
            return iconfile
        else:
            if self.form.checkOnline.isChecked():
                # download file
                u = urllib.request.urlopen(urllib.parse.quote(filepath, safe=":/."))
                fdata = u.read()
                u.close()
                f = io.BytesIO(fdata)
            else:
                f = filepath
            zfile = zipfile.ZipFile(f)
            if "thumbnails/Thumbnail.png" in zfile.namelist():
                data = zfile.read("thumbnails/Thumbnail.png")
                os.makedirs(os.path.dirname(iconfile), exist_ok=True)
                thumb = open(iconfile, "wb")
                thumb.write(data)
                thumb.close()
                return iconfile
            else:
                return None

    def getHashname(self, filepath):
        """creates a png filename for a given file path"""

        import hashlib

        filepath = self.cleanPath(filepath)
        return hashlib.md5(filepath.encode()).hexdigest() + ".png"

    def cleanPath(self, filepath):
        """cleans a file path into subfolder/subfolder/file form"""

        import urllib.request
        import urllib.parse

        if filepath.startswith(self.librarypath):
            # strip local part od the path
            filepath = filepath[len(self.librarypath) :]
        if filepath.startswith(RAWURL):
            filepath = filepath[len(RAWURL) :]
        filepath = filepath.replace("\\", "/")
        if filepath.startswith("/"):
            filepath = filepath[1:]
        filepath = urllib.parse.quote(filepath)
        return filepath

    def onExternalSearch(self, index):
        """searches on external websites"""

        if index > 0:
            baseurl = self.form.comboSearch.itemData(index)
            term = self.form.searchBox.text()
            if term:
                self.openUrl(baseurl + self.urlencode(term))


if FreeCAD.GuiUp:

    from PySide import QtCore, QtGui

    class LibraryModel(QtGui.QFileSystemModel):
        "a custom QFileSystemModel that displays FreeCAD file icons"

        def __init__(self):

            QtGui.QFileSystemModel.__init__(self)

        def data(self, index, role):

            if index.column() == 0 and role == QtCore.Qt.DecorationRole:
                if index.data().lower().endswith(".fcstd"):
                    return QtGui.QIcon(":icons/freecad-doc.png")
                elif index.data().lower().endswith(".ifc"):
                    return QtGui.QIcon(
                        os.path.join(os.path.dirname(__file__), "icons", "IFC.svg")
                    )
                elif index.data().lower() == "private":
                    return QtGui.QIcon.fromTheme("folder-lock")
            return super(LibraryModel, self).data(index, role)


FreeCADGui.addCommand("BIM_Library", BIM_Library())
