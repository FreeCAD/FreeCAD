#***************************************************************************
#*   Copyright (c) 2011 Yorik van Havre <yorik@uncreated.net>              *
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

import math

import FreeCAD
import Part
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond

__title__  = "FreeCAD Axis System"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchGrid
#  \ingroup ARCH
#  \brief Grid system for the Arch workbench
#
#  This module provides tools to build grid systems


def makeGrid(name=None):

    '''makeGrid([name]): makes a grid object'''

    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython","Grid")
    obj.Label = name if name else translate("Arch","Grid")
    ArchGrid(obj)
    if FreeCAD.GuiUp:
        ViewProviderArchGrid(obj.ViewObject)
        obj.ViewObject.Transparency = 85
    FreeCAD.ActiveDocument.recompute()
    return obj


class CommandArchGrid:

    "the Arch Grid command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Grid","Grid"),
                'Accel': "A, X",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Grid","Creates a customizable grid object")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Grid"))
        FreeCADGui.addModule("Arch")

        FreeCADGui.doCommand("Arch.makeGrid()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None


class ArchGrid:

    "The Grid object"

    def __init__(self,obj):

        obj.Proxy = self
        self.setProperties(obj)

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Rows" in pl:
            obj.addProperty("App::PropertyInteger","Rows","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The number of rows'))
        if not "Columns" in pl:
            obj.addProperty("App::PropertyInteger","Columns","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The number of columns'))
        if not "RowSize" in pl:
            obj.addProperty("App::PropertyFloatList","RowSize","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The sizes for rows'))
        if not "ColumnSize" in pl:
            obj.addProperty("App::PropertyFloatList","ColumnSize","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The sizes of columns'))
        if not "Spans" in pl:
            obj.addProperty("App::PropertyStringList","Spans","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The span ranges of cells that are merged together'))
        if not "PointsOutput" in pl:
            obj.addProperty("App::PropertyEnumeration","PointsOutput","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The type of 3D points produced by this grid object'))
            obj.PointsOutput = ["Vertices","Edges","Vertical Edges","Horizontal Edges","Faces"]
        if not "Width" in pl:
            obj.addProperty("App::PropertyLength","Width","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The total width of this grid'))
        if not "Height" in pl:
            obj.addProperty("App::PropertyLength","Height","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The total height of this grid'))
        if not "AutoWidth" in pl:
            obj.addProperty("App::PropertyLength","AutoWidth","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'Creates automatic column divisions (set to 0 to disable)'))
        if not "AutoHeight" in pl:
            obj.addProperty("App::PropertyLength","AutoHeight","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'Creates automatic row divisions (set to 0 to disable)'))
        if not "Reorient" in pl:
            obj.addProperty("App::PropertyBool","Reorient","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'When in edge midpoint mode, if this grid must reorient its children along edge normals or not'))
        if not "HiddenFaces" in pl:
            obj.addProperty("App::PropertyIntegerList","HiddenFaces","Grid",QT_TRANSLATE_NOOP("Arch_Grid",'The indices of faces to hide'))
        self.Type = "Grid"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)

    def getSizes(self,obj):

        "returns rowsizes,columnsizes,spangroups"

        if not obj.Height.Value:
            return [],[],[]
        if not obj.Width.Value:
            return [],[],[]
        if (not obj.Rows) and (not obj.AutoHeight.Value):
            return [],[],[]
        if (not obj.Columns) and (not obj.AutoWidth.Value):
            return [],[],[]
        # rescale rows
        rowsizes = []
        if obj.AutoHeight.Value:
            if obj.AutoHeight.Value > obj.Height.Value:
                FreeCAD.Console.PrintError(translate("Arch","Auto height is larger than height"))
                return [],[],[]
            rows = int(math.floor(obj.Height.Value/obj.AutoHeight.Value))
            for i in range(rows):
                rowsizes.append(obj.AutoHeight.Value)
            rowsizes.append(obj.Height.Value-rows*obj.AutoHeight.Value)
        else:
            reserved_rowsize = sum(v for v in obj.RowSize)
            if reserved_rowsize > obj.Height.Value:
                FreeCAD.Console.PrintError(translate("Arch","Total row size is larger than height"))
                return [],[],[]
            for i in range(obj.Rows):
                v = 0
                if i < len(obj.RowSize):
                    v = obj.RowSize[i]
                rowsizes.append(v)
            e = len([v for v in rowsizes if v == 0])
            default = obj.Height.Value - reserved_rowsize
            if e:
                default = default / e
            t = []
            for v in rowsizes:
                if v:
                    t.append(v)
                else:
                    t.append(default)
            rowsizes = t
        # rescale columns
        columnsizes = []
        if obj.AutoWidth.Value:
            if obj.AutoWidth.Value > obj.Width.Value:
                FreeCAD.Console.PrintError(translate("Arch","Auto width is larger than width"))
                return [],[],[]
            cols = int(math.floor(obj.Width.Value/obj.AutoWidth.Value))
            for i in range(cols):
                columnsizes.append(obj.AutoWidth.Value)
            columnsizes.append(obj.Width.Value-cols*obj.AutoWidth.Value)
        else:
            reserved_columnsize = sum(v for v in obj.ColumnSize)
            if reserved_columnsize > obj.Width.Value:
                FreeCAD.Console.PrintError(translate("Arch","Total column size is larger than width"))
                return [],[],[]
            for i in range(obj.Columns):
                v = 0
                if i < len(obj.ColumnSize):
                    v = obj.ColumnSize[i]
                columnsizes.append(v)
            e = len([v for v in columnsizes if v == 0])
            default = obj.Width.Value - reserved_columnsize
            if e:
                default = default / e
            t = []
            for v in columnsizes:
                if v:
                    t.append(v)
                else:
                    t.append(default)
            columnsizes = t
        # format span groups from [row,col,rowspan,colspan] to [faceindexes]
        spangroups = []
        for s in obj.Spans:
            nspan = []
            span = [int(i.strip()) for i in s.split(",")]
            for row in range(span[2]):
                for column in range(span[3]):
                    nspan.append((span[0]+row)*obj.Columns + (span[1]+column))
            spangroups.append(nspan)
        return rowsizes,columnsizes,spangroups

    def execute(self,obj):

        pl = obj.Placement
        rowsizes,columnsizes,spangroups = self.getSizes(obj)
        #print rowsizes,columnsizes,spangroups
        # create one face for each cell
        faces = []
        facenumber = 0
        rowoffset = 0
        for row in rowsizes:
            columnoffset = 0
            for column in columnsizes:
                v1 = FreeCAD.Vector(columnoffset,rowoffset,0)
                v2 = v1.add(FreeCAD.Vector(column,0,0))
                v3 = v2.add(FreeCAD.Vector(0,-row,0))
                v4 = v3.add(FreeCAD.Vector(-column,0,0))
                f = Part.Face(Part.makePolygon([v1,v2,v3,v4,v1]))
                if not facenumber in obj.HiddenFaces:
                    spanning = False
                    for i in range(len(spangroups)):
                        if facenumber in spangroups[i]:
                            g = spangroups[i]
                            g[g.index(facenumber)] = f
                            spangroups[i] = g
                            spanning = True
                            break
                    if not spanning:
                        faces.append(f)
                facenumber += 1
                columnoffset += column
            rowoffset -= row
        # join spangroups
        for g in spangroups:
            s = Part.makeShell(g)
            s = s.removeSplitter()
            faces.extend(s.Faces)
        if faces:
            obj.Shape = Part.makeCompound(faces)
            obj.Placement = pl

    def getPoints(self,obj):

        "returns the gridpoints"

        def remdupes(pts):
            # eliminate possible duplicates
            ret = []
            for p in pts:
                if not p in ret:
                    ret.append(p)
            return ret
        if obj.PointsOutput == "Vertices":
            return [v.Point for v in obj.Shape.Vertexes]
        elif obj.PointsOutput == "Edges":
            return remdupes([e.CenterOfMass for e in obj.Shape.Edges])
        elif obj.PointsOutput == "Vertical Edges":
            rv = obj.Placement.Rotation.multVec(FreeCAD.Vector(0,1,0))
            edges = [e for e in obj.Shape.Edges if round(rv.getAngle(e.tangentAt(e.FirstParameter)),4) in [0,3.1416]]
            return remdupes([e.CenterOfMass for e in edges])
        elif obj.PointsOutput == "Horizontal Edges":
            rv = obj.Placement.Rotation.multVec(FreeCAD.Vector(1,0,0))
            edges = [e for e in obj.Shape.Edges if round(rv.getAngle(e.tangentAt(e.FirstParameter)),4) in [0,3.1416]]
            return remdupes([e.CenterOfMass for e in edges])
        else:
            return [f.CenterOfMass for f in obj.Shape.Faces]

    def dumps(self):

        return None

    def loads(self,state):

        return None


class ViewProviderArchGrid:

    "A View Provider for the Arch Grid"

    def __init__(self,vobj):

        vobj.Proxy = self

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Grid.svg"

    def attach(self, vobj):
        self.Object = vobj.Object

    def setEdit(self, vobj, mode):
        if mode != 0:
            return None

        taskd = ArchGridTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        FreeCADGui.Control.closeDialog()
        return True

    def setupContextMenu(self, vobj, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def dumps(self):

        return None

    def loads(self,state):

        return None


class ArchGridTaskPanel:

    '''A TaskPanel for the Arch Grid'''

    def __init__(self,obj):

        # length, width, label
        self.width = 0
        self.height = 0
        self.spans = []
        self.obj = obj
        self.form = QtGui.QWidget()
        uil = FreeCADGui.UiLoader()
        layout = QtGui.QVBoxLayout(self.form)
        hbox3 = QtGui.QHBoxLayout()
        layout.addLayout(hbox3)
        self.wLabel = QtGui.QLabel(self.form)
        hbox3.addWidget(self.wLabel)
        self.widthUi = uil.createWidget("Gui::InputField")
        hbox3.addWidget(self.widthUi)
        hbox4 = QtGui.QHBoxLayout()
        layout.addLayout(hbox4)
        self.hLabel = QtGui.QLabel(self.form)
        hbox4.addWidget(self.hLabel)
        self.heightUi = uil.createWidget("Gui::InputField")
        hbox4.addWidget(self.heightUi)
        self.title = QtGui.QLabel(self.form)
        layout.addWidget(self.title)

        # grid
        self.table = QtGui.QTableWidget(self.form)
        layout.addWidget(self.table)
        style = "QTableWidget { background-color: #ffffff; gridline-color: #000000; }"
        self.table.setStyleSheet(style)
        self.table.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)

        # row/column buttons
        hbox1 = QtGui.QHBoxLayout()
        layout.addLayout(hbox1)
        self.addRowButton = QtGui.QPushButton(self.form)
        self.addRowButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        hbox1.addWidget(self.addRowButton)
        self.delRowButton = QtGui.QPushButton(self.form)
        self.delRowButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        hbox1.addWidget(self.delRowButton)
        self.addColumnButton = QtGui.QPushButton(self.form)
        self.addColumnButton.setIcon(QtGui.QIcon(":/icons/Arch_Add.svg"))
        hbox1.addWidget(self.addColumnButton)
        self.delColumnButton = QtGui.QPushButton(self.form)
        self.delColumnButton.setIcon(QtGui.QIcon(":/icons/Arch_Remove.svg"))
        hbox1.addWidget(self.delColumnButton)

        # span buttons
        hbox2 = QtGui.QHBoxLayout()
        layout.addLayout(hbox2)
        self.spanButton = QtGui.QPushButton(self.form)
        self.spanButton.setIcon(QtGui.QIcon(":/icons/SpreadsheetMergeCells.svg"))
        hbox2.addWidget(self.spanButton)
        self.spanButton.setEnabled(False)
        self.delSpanButton = QtGui.QPushButton(self.form)
        self.delSpanButton.setIcon(QtGui.QIcon(":/icons/SpreadsheetSplitCell.svg"))
        hbox2.addWidget(self.delSpanButton)
        self.delSpanButton.setEnabled(False)

        #signals
        QtCore.QObject.connect(self.widthUi,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.heightUi,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(self.table, QtCore.SIGNAL("itemSelectionChanged()"), self.checkSpan)
        QtCore.QObject.connect(self.addRowButton, QtCore.SIGNAL("clicked()"), self.addRow)
        QtCore.QObject.connect(self.delRowButton, QtCore.SIGNAL("clicked()"), self.delRow)
        QtCore.QObject.connect(self.addColumnButton, QtCore.SIGNAL("clicked()"), self.addColumn)
        QtCore.QObject.connect(self.delColumnButton, QtCore.SIGNAL("clicked()"), self.delColumn)
        QtCore.QObject.connect(self.spanButton, QtCore.SIGNAL("clicked()"), self.addSpan)
        QtCore.QObject.connect(self.delSpanButton, QtCore.SIGNAL("clicked()"), self.removeSpan)
        QtCore.QObject.connect(self.table.horizontalHeader(),QtCore.SIGNAL("sectionDoubleClicked(int)"), self.editHorizontalHeader)
        QtCore.QObject.connect(self.table.verticalHeader(),QtCore.SIGNAL("sectionDoubleClicked(int)"), self.editVerticalHeader)
        self.update()
        self.retranslateUi()

    def retranslateUi(self,widget=None):

        self.form.setWindowTitle(QtGui.QApplication.translate("Arch", "Grid", None))
        self.wLabel.setText(QtGui.QApplication.translate("Arch", "Total width", None))
        self.hLabel.setText(QtGui.QApplication.translate("Arch", "Total height", None))
        self.addRowButton.setText(QtGui.QApplication.translate("Arch", "Add row", None))
        self.delRowButton.setText(QtGui.QApplication.translate("Arch", "Del row", None))
        self.addColumnButton.setText(QtGui.QApplication.translate("Arch", "Add col", None))
        self.delColumnButton.setText(QtGui.QApplication.translate("Arch", "Del col", None))
        self.spanButton.setText(QtGui.QApplication.translate("Arch", "Create span", None))
        self.delSpanButton.setText(QtGui.QApplication.translate("Arch", "Remove span", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Rows", None)+": "+str(self.table.rowCount())+" / "+QtGui.QApplication.translate("Arch", "Columns", None)+": "+str(self.table.columnCount()))

    def update(self):

        self.table.clear()
        if self.obj.Rows:
            self.table.setRowCount(self.obj.Rows)
            vlabels = ["0" for i in range(self.obj.Rows)]
            for i,v in enumerate(self.obj.RowSize):
                if i < len(vlabels):
                    vlabels[i] = FreeCAD.Units.Quantity(v,FreeCAD.Units.Length).getUserPreferred()[0]
            self.table.setVerticalHeaderLabels(vlabels)
        if self.obj.Columns:
            self.table.setColumnCount(self.obj.Columns)
            hlabels = ["0" for i in range(self.obj.Columns)]
            for i,v in enumerate(self.obj.ColumnSize):
                if i < len(hlabels):
                    hlabels[i] = FreeCAD.Units.Quantity(v,FreeCAD.Units.Length).getUserPreferred()[0]
            self.table.setHorizontalHeaderLabels(hlabels)
        self.widthUi.setText(self.obj.Width.getUserPreferred()[0])
        self.heightUi.setText(self.obj.Height.getUserPreferred()[0])
        self.spans = []
        for s in self.obj.Spans:
            span = [int(i.strip()) for i in s.split(",")]
            if len(span) == 4:
                self.table.setSpan(span[0],span[1],span[2],span[3])
                self.spans.append(span)

    def checkSpan(self):

        if self.table.selectedRanges():
            self.spanButton.setEnabled(False)
            self.delSpanButton.setEnabled(False)
            if len(self.table.selectedRanges()) > 1:
                self.spanButton.setEnabled(True)
            for r in self.table.selectedRanges():
                if (r.rowCount() * r.columnCount()) > 1:
                    self.spanButton.setEnabled(True)
                elif (r.rowCount() * r.columnCount()) == 1:
                    if self.table.rowSpan(r.topRow(),r.leftColumn()) > 1 \
                    or self.table.columnSpan(r.topRow(),r.leftColumn()) > 1:
                        self.delSpanButton.setEnabled(True)
        else:
            self.spanButton.setEnabled(False)
            self.delSpanButton.setEnabled(False)

    def addRow(self):

        c = self.table.currentRow()
        self.table.insertRow(c+1)
        self.table.setVerticalHeaderItem(c+1,QtGui.QTableWidgetItem("0"))
        self.retranslateUi()

    def delRow(self):

        if self.table.selectedRanges():
            self.table.removeRow(self.table.currentRow())
            self.retranslateUi()

    def addColumn(self):

        c = self.table.currentColumn()
        self.table.insertColumn(c+1)
        self.table.setHorizontalHeaderItem(c+1,QtGui.QTableWidgetItem("0"))
        self.retranslateUi()

    def delColumn(self):

        if self.table.selectedRanges():
            self.table.removeColumn(self.table.currentColumn())
            self.retranslateUi()

    def addSpan(self):

        for r in self.table.selectedRanges():
            if r.rowCount() * r.columnCount() > 1:
                self.table.setSpan(r.topRow(),r.leftColumn(),r.rowCount(),r.columnCount())
                self.spans.append([r.topRow(),r.leftColumn(),r.rowCount(),r.columnCount()])
                return
        if len(self.table.selectedRanges()) > 1:
            tr = 99999
            br = 0
            lc = 99999
            rc = 0
            for r in self.table.selectedRanges():
                if r.topRow() < tr:
                    tr = r.topRow()
                if r.bottomRow() > br:
                    br = r.bottomRow()
                if r.leftColumn() < lc:
                    lc = r.leftColumn()
                if r.rightColumn() > rc:
                    rc = r.rightColumn()
            if (rc >= lc) and (br >= tr):
                self.table.setSpan(tr,lc,(br-tr)+1,(rc-lc)+1)
                self.spans.append([tr,lc,(br-tr)+1,(rc-lc)+1])

    def removeSpan(self):

        for r in self.table.selectedRanges():
            if r.rowCount() * r.columnCount() == 1:
                if self.table.rowSpan(r.topRow(),r.leftColumn()) > 1 \
                or self.table.columnSpan(r.topRow(),r.leftColumn()) > 1:
                    self.table.setSpan(r.topRow(),r.leftColumn(),1,1)
                    f = None
                    for i,s in enumerate(self.spans):
                        if (s[0] == r.topRow()) and (s[1] == r.leftColumn()):
                            f = i
                            break
                    if f is not None:
                        self.spans.pop(f)

    def editHorizontalHeader(self, index):

        val,ok = QtGui.QInputDialog.getText(None,'Edit size','New size')
        if ok:
            self.table.setHorizontalHeaderItem(index,QtGui.QTableWidgetItem(val))

    def editVerticalHeader(self, index):

        val,ok = QtGui.QInputDialog.getText(None,'Edit size','New size')
        if ok:
            self.table.setVerticalHeaderItem(index,QtGui.QTableWidgetItem(val))

    def setWidth(self,d):

        self.width = d

    def setHeight(self,d):

        self.height = d

    def accept(self):

        self.obj.Width = self.width
        self.obj.Height = self.height
        self.obj.Rows = self.table.rowCount()
        self.obj.Columns = self.table.columnCount()
        self.obj.RowSize = [FreeCAD.Units.Quantity(self.table.verticalHeaderItem(i).text()).Value for i in range(self.table.rowCount())]
        self.obj.ColumnSize = [FreeCAD.Units.Quantity(self.table.horizontalHeaderItem(i).text()).Value for i in range(self.table.columnCount())]
        self.obj.Spans = [str(s)[1:-1] for s in self.spans]
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        FreeCADGui.ActiveDocument.resetEdit()
        return True
