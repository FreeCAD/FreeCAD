# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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


import FreeCAD
import FreeCADGui
import Path
import Path.Tool.Bit as PathToolBit
import Path.Tool.Gui.Bit as PathToolBitGui
import Path.Tool.Gui.BitEdit as PathToolBitEdit
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathGui
import PathScripts.PathUtilsGui as PathUtilsGui
import PySide
import glob
import json
import os
import shutil
import uuid as UUID

from functools import partial


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


_UuidRole = PySide.QtCore.Qt.UserRole + 1
_PathRole = PySide.QtCore.Qt.UserRole + 2


translate = FreeCAD.Qt.translate


def checkWorkingDir():
    # users shouldn't use the example toolbits and libraries.
    # working directory should be writable
    Path.Log.track()

    workingdir = os.path.dirname(Path.Preferences.lastPathToolLibrary())
    defaultdir = os.path.dirname(Path.Preferences.pathDefaultToolsPath())

    Path.Log.debug("workingdir: {} defaultdir: {}".format(workingdir, defaultdir))

    dirOK = lambda: workingdir != defaultdir and (os.access(workingdir, os.W_OK))

    if dirOK():
        return True

    qm = PySide.QtGui.QMessageBox
    ret = qm.question(
        None,
        "",
        translate("Path_ToolBit", "Toolbit working directory not set up. Do that now?"),
        qm.Yes | qm.No,
    )

    if ret == qm.No:
        return False

    msg = translate("Path_ToolBit", "Choose a writable location for your toolbits")
    while not dirOK():
        workingdir = PySide.QtGui.QFileDialog.getExistingDirectory(
            None, msg, Path.Preferences.filePath()
        )

    if workingdir[-8:] == os.path.sep + "Library":
        workingdir = workingdir[:-8]  # trim off trailing /Library if user chose it

    Path.Preferences.setLastPathToolLibrary(
        "{}{}Library".format(workingdir, os.path.sep)
    )
    Path.Preferences.setLastPathToolBit("{}{}Bit".format(workingdir, os.path.sep))
    Path.Log.debug("setting workingdir to: {}".format(workingdir))

    # Copy only files of default Path/Tool folder to working directory (targeting the README.md help file)
    src_toolfiles = os.listdir(defaultdir)
    for file_name in src_toolfiles:
        if file_name in ["README.md"]:
            full_file_name = os.path.join(defaultdir, file_name)
            if os.path.isfile(full_file_name):
                shutil.copy(full_file_name, workingdir)

    # Determine which subdirectories are missing
    subdirlist = ["Bit", "Library", "Shape"]
    mode = 0o777
    for dir in subdirlist.copy():
        subdir = "{}{}{}".format(workingdir, os.path.sep, dir)
        if os.path.exists(subdir):
            subdirlist.remove(dir)

    # Query user for creation permission of any missing subdirectories
    if len(subdirlist) >= 1:
        needed = ", ".join([str(d) for d in subdirlist])
        qm = PySide.QtGui.QMessageBox
        ret = qm.question(
            None,
            "",
            translate(
                "Path_ToolBit",
                "Toolbit Working directory {} needs these sudirectories:\n {} \n Create them?",
            ).format(workingdir, needed),
            qm.Yes | qm.No,
        )

        if ret == qm.No:
            return False
        else:
            # Create missing subdirectories if user agrees to creation
            for dir in subdirlist:
                subdir = "{}{}{}".format(workingdir, os.path.sep, dir)
                os.mkdir(subdir, mode)
                # Query user to copy example files into subdirectories created
                if dir != "Shape":
                    qm = PySide.QtGui.QMessageBox
                    ret = qm.question(
                        None,
                        "",
                        translate(
                            "Path_ToolBit", "Copy example files to new {} directory?"
                        ).format(dir),
                        qm.Yes | qm.No,
                    )
                    if ret == qm.Yes:
                        src = "{}{}{}".format(defaultdir, os.path.sep, dir)
                        src_files = os.listdir(src)
                        for file_name in src_files:
                            full_file_name = os.path.join(src, file_name)
                            if os.path.isfile(full_file_name):
                                shutil.copy(full_file_name, subdir)

    # if no library is set, choose the first one in the Library directory
    if Path.Preferences.lastFileToolLibrary() is None:
        libFiles = [
            f
            for f in glob.glob(
                Path.Preferences.lastPathToolLibrary() + os.path.sep + "*.fctl"
            )
        ]
        Path.Preferences.setLastFileToolLibrary(libFiles[0])

    return True


class _TableView(PySide.QtGui.QTableView):
    """Subclass of QTableView to support rearrange and copying of ToolBits"""

    def __init__(self, parent):
        PySide.QtGui.QTableView.__init__(self, parent)
        self.setDragEnabled(False)
        self.setAcceptDrops(False)
        self.setDropIndicatorShown(False)
        self.setDragDropMode(PySide.QtGui.QAbstractItemView.DragOnly)
        self.setDefaultDropAction(PySide.QtCore.Qt.IgnoreAction)
        self.setSortingEnabled(True)
        self.setSelectionBehavior(PySide.QtGui.QAbstractItemView.SelectRows)
        self.verticalHeader().hide()

    def supportedDropActions(self):
        return [PySide.QtCore.Qt.CopyAction, PySide.QtCore.Qt.MoveAction]

    def _uuidOfRow(self, row):
        model = self.toolModel()
        return model.data(model.index(row, 0), _UuidRole)

    def _rowWithUuid(self, uuid):
        model = self.toolModel()
        for row in range(model.rowCount()):
            if self._uuidOfRow(row) == uuid:
                return row
        return None

    def _copyTool(self, uuid_, dstRow):
        model = self.toolModel()
        model.insertRow(dstRow)
        srcRow = self._rowWithUuid(uuid_)
        for col in range(model.columnCount()):
            srcItem = model.item(srcRow, col)

            model.setData(
                model.index(dstRow, col),
                srcItem.data(PySide.QtCore.Qt.EditRole),
                PySide.QtCore.Qt.EditRole,
            )
            if col == 0:
                model.setData(
                    model.index(dstRow, col), srcItem.data(_PathRole), _PathRole
                )
                # Even a clone of a tool gets its own uuid so it can be identified when
                # rearranging the order or inserting/deleting rows
                model.setData(model.index(dstRow, col), UUID.uuid4(), _UuidRole)
            else:
                model.item(dstRow, col).setEditable(False)

    def _copyTools(self, uuids, dst):
        for i, uuid in enumerate(uuids):
            self._copyTool(uuid, dst + i)

    def dropEvent(self, event):
        Path.Log.track()
        mime = event.mimeData()
        data = mime.data("application/x-qstandarditemmodeldatalist")
        stream = PySide.QtCore.QDataStream(data)
        srcRows = []
        while not stream.atEnd():
            row = stream.readInt32()
            srcRows.append(row)

        # get the uuids of all srcRows
        model = self.toolModel()
        srcUuids = [self._uuidOfRow(row) for row in set(srcRows)]
        destRow = self.rowAt(event.pos().y())

        self._copyTools(srcUuids, destRow)
        if PySide.QtCore.Qt.DropAction.MoveAction == event.proposedAction():
            for uuid in srcUuids:
                model.removeRow(self._rowWithUuid(uuid))


class ModelFactory(object):
    """Helper class to generate qtdata models for toolbit libraries"""

    def __init__(self, path=None):
        Path.Log.track()
        self.path = ""
        # self.currentLib = ""

    def __libraryLoad(self, path, datamodel):
        Path.Log.track(path)
        Path.Preferences.setLastFileToolLibrary(path)
        # self.currenLib = path

        with open(path) as fp:
            library = json.load(fp)

        for toolBit in library["tools"]:
            try:
                nr = toolBit["nr"]
                bit = PathToolBit.findToolBit(toolBit["path"], path)
                if bit:
                    Path.Log.track(bit)
                    tool = PathToolBit.Declaration(bit)
                    datamodel.appendRow(self._toolAdd(nr, tool, bit))
                else:
                    Path.Log.error(
                        "Could not find tool #{}: {}".format(nr, toolBit["path"])
                    )
            except Exception as e:
                msg = "Error loading tool: {} : {}".format(toolBit["path"], e)
                FreeCAD.Console.PrintError(msg)

    def _toolAdd(self, nr, tool, path):

        strShape = os.path.splitext(os.path.basename(tool["shape"]))[0]
        # strDiam = tool['parameter']['Diameter']
        tooltip = "{}".format(strShape)

        toolNr = PySide.QtGui.QStandardItem()
        toolNr.setData(nr, PySide.QtCore.Qt.EditRole)
        toolNr.setToolTip(tool["shape"])
        toolNr.setData(path, _PathRole)
        toolNr.setData(UUID.uuid4(), _UuidRole)
        toolNr.setToolTip(tooltip)

        toolName = PySide.QtGui.QStandardItem()
        toolName.setData(tool["name"], PySide.QtCore.Qt.EditRole)
        toolName.setEditable(False)
        toolName.setToolTip(tooltip)

        toolShape = PySide.QtGui.QStandardItem()
        toolShape.setData(strShape, PySide.QtCore.Qt.EditRole)
        toolShape.setEditable(False)

        return [toolNr, toolName, toolShape]

    def newTool(self, datamodel, path):
        """
        Adds a toolbit item to a model
        """
        Path.Log.track()

        try:
            nr = 0
            for row in range(datamodel.rowCount()):
                itemNr = int(datamodel.item(row, 0).data(PySide.QtCore.Qt.EditRole))
                nr = max(nr, itemNr)
            nr += 1
            tool = PathToolBit.Declaration(path)
        except Exception as e:
            Path.Log.error(e)

        datamodel.appendRow(self._toolAdd(nr, tool, path))

    def findLibraries(self, model):
        """
        Finds all the fctl files in a location
        Returns a QStandardItemModel
        """
        Path.Log.track()
        path = Path.Preferences.lastPathToolLibrary()

        if os.path.isdir(path):  # opening all tables in a directory
            libFiles = [f for f in glob.glob(path + os.path.sep + "*.fctl")]
            libFiles.sort()
            for libFile in libFiles:
                loc, fnlong = os.path.split(libFile)
                fn, ext = os.path.splitext(fnlong)
                libItem = PySide.QtGui.QStandardItem(fn)
                libItem.setToolTip(loc)
                libItem.setData(libFile, _PathRole)
                libItem.setIcon(PySide.QtGui.QPixmap(":/icons/Path_ToolTable.svg"))
                model.appendRow(libItem)

        Path.Log.debug("model rows: {}".format(model.rowCount()))
        return model

    def libraryOpen(self, model, lib=""):
        """
        opens the tools in library
        Returns a QStandardItemModel
        """
        Path.Log.track(lib)

        if lib == "":
            lib = Path.Preferences.lastFileToolLibrary()

        if lib == "" or lib is None:
            return model

        if os.path.isfile(lib):  # An individual library is wanted
            self.__libraryLoad(lib, model)

        Path.Log.debug("model rows: {}".format(model.rowCount()))
        return model


class ToolBitSelector(object):
    """Controller for displaying a library and creating ToolControllers"""

    def __init__(self):
        checkWorkingDir()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitSelector.ui")
        self.factory = ModelFactory()
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.setupUI()
        self.title = self.form.windowTitle()

    def columnNames(self):
        return ["#", "Tool"]

    def currentLibrary(self, shortNameOnly):
        libfile = Path.Preferences.lastFileToolLibrary()
        if libfile is None or libfile == "":
            return ""
        elif shortNameOnly:
            return os.path.splitext(os.path.basename(libfile))[0]
        return libfile

    def loadData(self):
        Path.Log.track()
        self.toolModel.clear()
        self.toolModel.setHorizontalHeaderLabels(self.columnNames())
        self.form.lblLibrary.setText(self.currentLibrary(True))
        self.form.lblLibrary.setToolTip(self.currentLibrary(False))
        self.factory.libraryOpen(self.toolModel)
        self.toolModel.takeColumn(3)
        self.toolModel.takeColumn(2)

    def setupUI(self):
        Path.Log.track()
        self.loadData()
        self.form.tools.setModel(self.toolModel)
        self.form.tools.selectionModel().selectionChanged.connect(self.enableButtons)
        self.form.tools.doubleClicked.connect(
            partial(self.selectedOrAllToolControllers)
        )
        self.form.libraryEditorOpen.clicked.connect(self.libraryEditorOpen)
        self.form.addToolController.clicked.connect(self.selectedOrAllToolControllers)

    def enableButtons(self):
        selected = len(self.form.tools.selectedIndexes()) >= 1
        if selected:
            jobs = (
                len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"])
                >= 1
            )
        self.form.addToolController.setEnabled(selected and jobs)

    def libraryEditorOpen(self):
        library = ToolBitLibrary()
        library.open()
        self.loadData()

    def selectedOrAllTools(self):
        """
        Iterate the selection and add individual tools
        If a group is selected, iterate and add children
        """

        itemsToProcess = []
        for index in self.form.tools.selectedIndexes():
            item = index.model().itemFromIndex(index)

            if item.hasChildren():
                for i in range(item.rowCount() - 1):
                    if item.child(i).column() == 0:
                        itemsToProcess.append(item.child(i))

            elif item.column() == 0:
                itemsToProcess.append(item)

        tools = []
        for item in itemsToProcess:
            toolNr = int(item.data(PySide.QtCore.Qt.EditRole))
            toolPath = item.data(_PathRole)
            tools.append((toolNr, PathToolBit.Factory.CreateFrom(toolPath)))
        return tools

    def selectedOrAllToolControllers(self, index=None):
        """
        if no jobs, don't do anything, otherwise all TCs for all
        selected toolbits
        """
        jobs = PathUtilsGui.PathUtils.GetJobs()
        if len(jobs) == 0:
            return
        elif len(jobs) == 1:
            job = jobs[0]
        else:
            userinput = PathUtilsGui.PathUtilsUserInput()
            job = userinput.chooseJob(jobs)

        if job is None:  # user may have canceled
            return

        tools = self.selectedOrAllTools()

        for tool in tools:
            tc = PathToolControllerGui.Create(
                "TC: {}".format(tool[1].Label), tool[1], tool[0]
            )
            job.Proxy.addToolController(tc)
            FreeCAD.ActiveDocument.recompute()

    def open(self, path=None):
        """load library stored in path and bring up ui"""
        docs = FreeCADGui.getMainWindow().findChildren(PySide.QtGui.QDockWidget)
        for doc in docs:
            if doc.objectName() == "ToolSelector":
                if doc.isVisible():
                    doc.deleteLater()
                    return
                else:
                    doc.setVisible(True)
                    return

        mw = FreeCADGui.getMainWindow()
        mw.addDockWidget(
            PySide.QtCore.Qt.RightDockWidgetArea,
            self.form,
            PySide.QtCore.Qt.Orientation.Vertical,
        )


class ToolBitLibrary(object):
    """ToolBitLibrary is the controller for
    displaying/selecting/creating/editing a collection of ToolBits."""

    def __init__(self):
        Path.Log.track()
        checkWorkingDir()
        self.factory = ModelFactory()
        self.temptool = None
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.listModel = PySide.QtGui.QStandardItemModel()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitLibraryEdit.ui")
        self.toolTableView = _TableView(self.form.toolTableGroup)
        self.form.toolTableGroup.layout().replaceWidget(
            self.form.toolTable, self.toolTableView
        )
        self.form.toolTable.hide()
        self.setupUI()
        self.title = self.form.windowTitle()

    def toolBitNew(self):
        Path.Log.track()

        # select the shape file
        shapefile = PathToolBitGui.GetToolShapeFile()
        if shapefile is None:  # user canceled
            return

        # select the bit file location and filename
        filename = PathToolBitGui.GetNewToolFile()
        if filename is None:
            return

        # Parse out the name of the file and write the structure
        loc, fil = os.path.split(filename)
        fname = os.path.splitext(fil)[0]
        fullpath = "{}{}{}.fctb".format(loc, os.path.sep, fname)
        Path.Log.debug("fullpath: {}".format(fullpath))

        self.temptool = PathToolBit.ToolBitFactory().Create(name=fname)
        self.temptool.BitShape = shapefile
        self.temptool.Proxy.unloadBitBody(self.temptool)
        self.temptool.Label = fname
        self.temptool.Proxy.saveToFile(self.temptool, fullpath)
        self.temptool.Document.removeObject(self.temptool.Name)
        self.temptool = None

        # add it to the model
        self.factory.newTool(self.toolModel, fullpath)

    def toolBitExisting(self):

        filenames = PathToolBitGui.GetToolFiles()

        if len(filenames) == 0:
            return

        for f in filenames:

            loc, fil = os.path.split(f)
            fname = os.path.splitext(fil)[0]
            fullpath = "{}{}{}.fctb".format(loc, os.path.sep, fname)

            self.factory.newTool(self.toolModel, fullpath)

    def toolDelete(self):
        Path.Log.track()
        selectedRows = set(
            [index.row() for index in self.toolTableView.selectedIndexes()]
        )
        for row in sorted(list(selectedRows), key=lambda r: -r):
            self.toolModel.removeRows(row, 1)

    def toolSelect(self, selected, deselected):
        sel = len(self.toolTableView.selectedIndexes()) > 0
        self.form.toolDelete.setEnabled(sel)

    def tableSelected(self, index):
        """loads the tools for the selected tool table"""
        Path.Log.track()
        item = index.model().itemFromIndex(index)
        libpath = item.data(_PathRole)
        self.loadData(libpath)
        self.path = libpath

    def open(self):
        Path.Log.track()
        return self.form.exec_()

    def libraryPath(self):
        Path.Log.track()
        path = PySide.QtGui.QFileDialog.getExistingDirectory(
            self.form, "Tool Library Path", Path.Preferences.lastPathToolLibrary()
        )
        if len(path) == 0:
            return

        Path.Preferences.setLastPathToolLibrary(path)
        self.loadData()

    def cleanupDocument(self):
        # This feels like a hack.  Remove the toolbit object
        # remove the editor from the dialog
        # re-enable all the controls
        self.temptool.Proxy.unloadBitBody(self.temptool)
        self.temptool.Document.removeObject(self.temptool.Name)
        self.temptool = None
        widget = self.form.toolTableGroup.children()[-1]
        widget.setParent(None)
        self.editor = None
        self.lockoff()

    def accept(self):
        self.editor.accept()
        self.temptool.Proxy.saveToFile(self.temptool, self.temptool.File)
        self.librarySave()
        self.loadData()
        self.cleanupDocument()

    def reject(self):
        self.cleanupDocument()

    def lockon(self):
        self.toolTableView.setEnabled(False)
        self.form.toolCreate.setEnabled(False)
        self.form.toolDelete.setEnabled(False)
        self.form.toolAdd.setEnabled(False)
        self.form.TableList.setEnabled(False)
        self.form.libraryOpen.setEnabled(False)
        self.form.libraryExport.setEnabled(False)
        self.form.addToolTable.setEnabled(False)
        self.form.librarySave.setEnabled(False)

    def lockoff(self):
        self.toolTableView.setEnabled(True)
        self.form.toolCreate.setEnabled(True)
        self.form.toolDelete.setEnabled(True)
        self.form.toolAdd.setEnabled(True)
        self.form.toolTable.setEnabled(True)
        self.form.TableList.setEnabled(True)
        self.form.libraryOpen.setEnabled(True)
        self.form.libraryExport.setEnabled(True)
        self.form.addToolTable.setEnabled(True)
        self.form.librarySave.setEnabled(True)

    def toolEdit(self, selected):
        Path.Log.track()
        item = self.toolModel.item(selected.row(), 0)

        if self.temptool is not None:
            self.temptool.Document.removeObject(self.temptool.Name)

        if selected.column() == 0:  # editing Nr
            pass
        else:
            tbpath = item.data(_PathRole)
            self.temptool = PathToolBit.ToolBitFactory().CreateFrom(tbpath, "temptool")
            self.editor = PathToolBitEdit.ToolBitEditor(
                self.temptool, self.form.toolTableGroup, loadBitBody=False
            )

            QBtn = (
                PySide.QtGui.QDialogButtonBox.Ok | PySide.QtGui.QDialogButtonBox.Cancel
            )
            buttonBox = PySide.QtGui.QDialogButtonBox(QBtn)
            buttonBox.accepted.connect(self.accept)
            buttonBox.rejected.connect(self.reject)

            layout = self.editor.form.layout()
            layout.addWidget(buttonBox)
            self.lockon()
            self.editor.setupUI()

    def toolEditDone(self, success=True):
        FreeCAD.ActiveDocument.removeObject("temptool")
        print("all done")

    def libraryNew(self):
        TooltableTypeJSON = translate("Path_ToolBit", "Tooltable JSON (*.fctl)")

        filename = PySide.QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("Path_ToolBit", "Save toolbit library"),
            Path.Preferences.lastPathToolLibrary(),
            "{}".format(TooltableTypeJSON),
        )

        if not (filename and filename[0]):
            self.loadData()

        path = (
            filename[0]
            if filename[0].endswith(".fctl")
            else "{}.fctl".format(filename[0])
        )
        library = {}
        tools = []
        library["version"] = 1
        library["tools"] = tools
        with open(path, "w") as fp:
            json.dump(library, fp, sort_keys=True, indent=2)

        self.loadData()

    def librarySave(self):
        library = {}
        tools = []
        library["version"] = 1
        library["tools"] = tools
        for row in range(self.toolModel.rowCount()):
            toolNr = self.toolModel.data(
                self.toolModel.index(row, 0), PySide.QtCore.Qt.EditRole
            )
            toolPath = self.toolModel.data(self.toolModel.index(row, 0), _PathRole)
            if Path.Preferences.toolsStoreAbsolutePaths():
                bitPath = toolPath
            else:
                # bitPath = PathToolBit.findRelativePathTool(toolPath)
                # Extract the name of the shape file
                __, filShp = os.path.split(
                    toolPath
                )  #  __ is an ignored placeholder acknowledged by LGTM
                bitPath = str(filShp)
            tools.append({"nr": toolNr, "path": bitPath})

        if self.path is not None:
            with open(self.path, "w") as fp:
                json.dump(library, fp, sort_keys=True, indent=2)

    def libraryOk(self):
        self.librarySave()
        self.form.close()

    def libPaths(self):
        lib = Path.Preferences.lastFileToolLibrary()
        loc = Path.Preferences.lastPathToolLibrary()

        Path.Log.track("lib: {} loc: {}".format(lib, loc))
        return lib, loc

    def columnNames(self):
        return ["Nr", "Tool", "Shape"]

    def loadData(self, path=None):
        Path.Log.track(path)
        self.toolTableView.setUpdatesEnabled(False)
        self.form.TableList.setUpdatesEnabled(False)

        if path is None:
            path, loc = self.libPaths()

            self.toolModel.clear()
            self.listModel.clear()
            self.factory.libraryOpen(self.toolModel, lib=path)
            self.factory.findLibraries(self.listModel)

        else:
            self.toolModel.clear()
            self.factory.libraryOpen(self.toolModel, lib=path)

        self.path = path
        self.form.setWindowTitle("{}".format(Path.Preferences.lastPathToolLibrary()))
        self.toolModel.setHorizontalHeaderLabels(self.columnNames())
        self.listModel.setHorizontalHeaderLabels(["Library"])

        # Select the current library in the list of tables
        curIndex = None
        for i in range(self.listModel.rowCount()):
            item = self.listModel.item(i)
            if item.data(_PathRole) == path:
                curIndex = self.listModel.indexFromItem(item)

        if curIndex:
            sm = self.form.TableList.selectionModel()
            sm.select(curIndex, PySide.QtCore.QItemSelectionModel.Select)

        self.toolTableView.setUpdatesEnabled(True)
        self.form.TableList.setUpdatesEnabled(True)

    def setupUI(self):
        Path.Log.track()
        self.form.TableList.setModel(self.listModel)
        self.toolTableView.setModel(self.toolModel)

        self.loadData()

        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)
        self.toolTableView.doubleClicked.connect(self.toolEdit)

        self.form.TableList.clicked.connect(self.tableSelected)

        self.form.toolAdd.clicked.connect(self.toolBitExisting)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolCreate.clicked.connect(self.toolBitNew)

        self.form.addToolTable.clicked.connect(self.libraryNew)

        self.form.libraryOpen.clicked.connect(self.libraryPath)
        self.form.librarySave.clicked.connect(self.libraryOk)
        self.form.libraryExport.clicked.connect(self.librarySaveAs)

        self.toolSelect([], [])

    def librarySaveAs(self, path):

        TooltableTypeJSON = translate("Path_ToolBit", "Tooltable JSON (*.fctl)")
        TooltableTypeLinuxCNC = translate("Path_ToolBit", "LinuxCNC tooltable (*.tbl)")
        TooltableTypeCamotics = translate("Path_ToolBit", "Camotics tooltable (*.json)")

        filename = PySide.QtGui.QFileDialog.getSaveFileName(
            self.form,
            translate("Path_ToolBit", "Save toolbit library"),
            Path.Preferences.lastPathToolLibrary(),
            "{};;{};;{}".format(
                TooltableTypeJSON, TooltableTypeLinuxCNC, TooltableTypeCamotics
            ),
        )
        if filename and filename[0]:
            if filename[1] == TooltableTypeLinuxCNC:
                path = (
                    filename[0]
                    if filename[0].endswith(".tbl")
                    else "{}.tbl".format(filename[0])
                )
                self.libararySaveLinuxCNC(path)
            elif filename[1] == TooltableTypeCamotics:
                path = (
                    filename[0]
                    if filename[0].endswith(".json")
                    else "{}.json".format(filename[0])
                )
                self.libararySaveCamotics(path)
            else:
                path = (
                    filename[0]
                    if filename[0].endswith(".fctl")
                    else "{}.fctl".format(filename[0])
                )
                self.path = path
                self.librarySave()
                self.updateToolbar()

    def libararySaveLinuxCNC(self, path):
        # linuxcnc line template
        LIN = "T{} P{} X{} Y{} Z{} A{} B{} C{} U{} V{} W{} D{} I{} J{} Q{}; {}"
        with open(path, "w") as fp:
            fp.write(";\n")

            for row in range(self.toolModel.rowCount()):
                toolNr = self.toolModel.data(
                    self.toolModel.index(row, 0), PySide.QtCore.Qt.EditRole
                )
                toolPath = self.toolModel.data(self.toolModel.index(row, 0), _PathRole)

                bit = PathToolBit.Factory.CreateFrom(toolPath)
                if bit:
                    Path.Log.track(bit)

                    pocket = bit.Pocket if hasattr(bit, "Pocket") else "0"
                    xoffset = bit.Xoffset if hasattr(bit, "Xoffset") else "0"
                    yoffset = bit.Yoffset if hasattr(bit, "Yoffset") else "0"
                    zoffset = bit.Zoffset if hasattr(bit, "Zoffset") else "0"
                    aoffset = bit.Aoffset if hasattr(bit, "Aoffset") else "0"
                    boffset = bit.Boffset if hasattr(bit, "Boffset") else "0"
                    coffset = bit.Coffset if hasattr(bit, "Coffset") else "0"
                    uoffset = bit.Uoffset if hasattr(bit, "Uoffset") else "0"
                    voffset = bit.Voffset if hasattr(bit, "Voffset") else "0"
                    woffset = bit.Woffset if hasattr(bit, "Woffset") else "0"

                    diameter = (
                        bit.Diameter.getUserPreferred()[0].split()[0]
                        if hasattr(bit, "Diameter")
                        else "0"
                    )
                    frontangle = bit.FrontAngle if hasattr(bit, "FrontAngle") else "0"
                    backangle = bit.BackAngle if hasattr(bit, "BackAngle") else "0"
                    orientation = (
                        bit.Orientation if hasattr(bit, "Orientation") else "0"
                    )
                    remark = bit.Label

                    fp.write(
                        LIN.format(
                            toolNr,
                            pocket,
                            xoffset,
                            yoffset,
                            zoffset,
                            aoffset,
                            boffset,
                            coffset,
                            uoffset,
                            voffset,
                            woffset,
                            diameter,
                            frontangle,
                            backangle,
                            orientation,
                            remark,
                        )
                        + "\n"
                    )

                    FreeCAD.ActiveDocument.removeObject(bit.Name)

                else:
                    Path.Log.error("Could not find tool #{} ".format(toolNr))

    def libararySaveCamotics(self, path):

        SHAPEMAP = {
            "ballend": "Ballnose",
            "endmill": "Cylindrical",
            "v-bit": "Conical",
            "chamfer": "Snubnose",
        }

        tooltemplate = {
            "units": "metric",
            "shape": "cylindrical",
            "length": 10,
            "diameter": 3.125,
            "description": "",
        }
        toollist = {}

        unitstring = (
            "imperial" if FreeCAD.Units.getSchema() in [2, 3, 5, 7] else "metric"
        )

        for row in range(self.toolModel.rowCount()):
            toolNr = self.toolModel.data(
                self.toolModel.index(row, 0), PySide.QtCore.Qt.EditRole
            )

            toolPath = self.toolModel.data(self.toolModel.index(row, 0), _PathRole)
            Path.Log.debug(toolPath)
            try:
                bit = PathToolBit.Factory.CreateFrom(toolPath)
            except FileNotFoundError as e:
                FreeCAD.Console.PrintError(e)
                continue
            except Exception as e:
                raise e

            if not bit:
                continue

            Path.Log.track(bit)

            toolitem = tooltemplate.copy()

            toolitem["diameter"] = (
                float(bit.Diameter.getUserPreferred()[0].split()[0])
                if hasattr(bit, "Diameter")
                else 2
            )
            toolitem["description"] = bit.Label
            toolitem["length"] = (
                float(bit.Length.getUserPreferred()[0].split()[0])
                if hasattr(bit, "Length")
                else 10
            )

            if hasattr(bit, "Camotics"):
                toolitem["shape"] = bit.Camotics
            else:
                toolitem["shape"] = SHAPEMAP.get(bit.ShapeName, "Cylindrical")

            toolitem["units"] = unitstring
            FreeCAD.ActiveDocument.removeObject(bit.Name)

            toollist[toolNr] = toolitem

        if len(toollist) > 0:
            with open(path, "w") as fp:
                fp.write(json.dumps(toollist, indent=2))
