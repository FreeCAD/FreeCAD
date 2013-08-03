#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Yorik van Havre <yorik@uncreated.net>            *
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

import re, math, FreeCAD, FreeCADGui
from PyQt4 import QtCore,QtGui

class Spreadsheet(object):
    """An object representing a spreadsheet. Can be used as a
    FreeCAD object or as a standalone python object.
    Cells of the spreadsheet can be got/set as arguments, as:
    
        myspreadsheet.a1 = 54
        print(myspreadsheet.a1)
        myspreadsheet.a2 = "My text"
        myspreadsheet.b1 = "=a1*3"
        print(myspreadsheet.a3)
        
    Functions usable in formulae are limited to the contents of
    the math module."""

    def __init__(self,obj=None):
        if obj:
            obj.Proxy = self
        self._cells = {}
        self._relations = {}
        self.cols = []
        self.rows = []

    def __repr__(self):
        return "Spreadsheet object containing " + str(len(self._cells)) + " cells"

    def __setattr__(self, key, value):
        #print "setting key:",key," to value:",value
        if self.isKey(key):
            self._cells[key] = value
            if value:
                if self.isFunction(value):
                    self._updateDependencies(key,value)
            c,r = self.splitKey(key)
            if not c in self.cols:
                self.cols.append(c)
                self.cols.sort()
            if not r in self.rows:
                self.rows.append(r)
                self.rows.sort()
        else:
            self.__dict__.__setitem__(key,value)

    def __getattr__(self, key):
        if key in self._cells:
            if self.isFunction(self._cells[key]):
                #print "result = ",self.getFunction(key)
                # building a list of safe functions allowed in eval
                safe_list = ['acos', 'asin', 'atan', 'atan2', 'ceil', 
                     'cos', 'cosh', 'e', 'exp', 'fabs', 
                     'floor', 'fmod', 'frexp', 'hypot', 'ldexp', 'log', 
                     'log10', 'modf', 'pi', 'pow', 'radians', 'sin', 
                     'sinh', 'sqrt', 'tan', 'tanh']
                tools = dict((k, getattr(math, k)) for k in safe_list)
                # adding abs
                tools["abs"] = abs
                # removing all builtins from allowed functions
                tools["__builtins__"] = None
                try: 
                    e = eval(self._format(key),tools,{"self":self})
                except:
                    print "Error evaluating formula"
                    return self._cells[key]
                else:
                    return e
            else:
                return self._cells[key]
        else:
            return None
            
    def __getstate__(self):
        return self._cells

    def __setstate__(self,state):
        if state:
            self._cells = state
            # TODO rebuild rows, cols and _relations

    def _format(self,key):
        "formats all cellnames in the function a the given cell"
        elts = re.split(r'(\W+)',self._cells[key][1:])
        #print elts
        result = ''
        for e in elts:
            if self.isKey(e):
                result += "self."+e
            else:
                result += e
        return result

    def _updateDependencies(self,key,value):
        "search for ancestors in the value and updates the table"
        ancestors = []
        for v in re.findall(r"[\w']+",value):
            if self.isKey(v):
                ancestors.append(v)
        for a in ancestors:
            if a in self._relations:
                if not key in self._relations[a]:
                    self._relations[a].append(key)
            else:
                self._relations[a] = [key]

    def execute(self,obj=None):
        pass

    def isFunction(self,key):
        "isFunction(cell): returns True if the given cell or value is a function"
        if key in self._cells:
            if str(self._cells[key])[0] == "=":
                return True
        elif str(key)[0] == "=":
            return True
        else:
            return False

    def isKey(self,value):
        "isKey(val): returns True if the given value is a valid cell number"
        al = False
        nu = False
        for v in value:
            if not v.isalnum():
                return False
            elif not al:
                if v.isalpha():
                    al = True
                else:
                    return False
            else:
                if not nu:
                    # forbidden to set items at row 0
                    if v == "0": 
                        return False
                if v.isalpha():
                    if nu:
                        return False
                elif v.isdigit():
                    nu = True
        if not nu:
            return False
        return True

    def splitKey(self,key):
        "splitKey(cell): splits a key between column and row"
        c = ''
        r = ''
        for ch in key:
            if ch.isalpha(): 
                c += ch
            else:
                r += ch
        return c,r

    def getFunction(self,key):
        "getFunction(cell): returns the function contained in the given cell, instead of the value"
        if key in self._cells:
            return self._cells[key]
        else:
            return None

    def getSize(self):
        "getSize(): returns a tuple with number of columns and rows of this spreadsheet"
        return (len(self.columns),len(self.rows))

    def getCells(self,index):
        "getCells(index): returns the cells from the given column of row number"
        cells = {}
        for k in self._cells.keys():
            c,r = self.splitKey(k)
            if index in [c,r]:
                cells[k] = self._cells[k]
        return cells


class ViewProviderSpreadsheet(object):
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/Spreadsheet.svg" 

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def getDisplayModes(self,vobj):
        return ["None"]
 
    def getDefaultDisplayMode(self):
        return "None"
 
    def setDisplayMode(self,mode):
        return mode

    def setEdit(self,vobj,mode):
        if hasattr(self,"editor"):
            pass
        else:
            #FreeCADGui.Control.showDialog(SpreadsheetTaskPanel())
            self.editor = SpreadsheetView(vobj.Object)
            addSpreadsheetView(self.editor)
        return True
    
    def unsetEdit(self,vobj,mode):
        #FreeCADGui.Control.closeDialog()
        return False


class SpreadsheetView(QtGui.QWidget):
    "A spreadsheet viewer for FreeCAD"

    def __init__(self,spreadsheet=None):
        from DraftTools import translate
        QtGui.QWidget.__init__(self)
        self.setWindowIcon(QtGui.QIcon(":/icons/Spreadsheet.svg"))
        self.setWindowTitle(str(translate("Spreadsheet","Spreadsheet")))
        self.setObjectName("Spreadsheet viewer")
        self.verticalLayout = QtGui.QVBoxLayout(self)
        
        # add editor line
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.label = QtGui.QLabel(self)
        self.label.setMinimumSize(QtCore.QSize(82, 0))
        self.label.setText(str(translate("Spreadsheet","Cell"))+":")
        self.horizontalLayout.addWidget(self.label)
        self.lineEdit = QtGui.QLineEdit(self)
        self.horizontalLayout.addWidget(self.lineEdit)
        self.verticalLayout.addLayout(self.horizontalLayout)
        
        # add table
        self.table = QtGui.QTableWidget(30,26,self)
        for i in range(26):
            ch = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]
            self.table.setHorizontalHeaderItem(i, QtGui.QTableWidgetItem(ch))
        self.verticalLayout.addWidget(self.table)
        self.spreadsheet = spreadsheet
        self.update()

        QtCore.QObject.connect(self.table, QtCore.SIGNAL("cellChanged(int,int)"), self.changeCell)
        QtCore.QObject.connect(self.table, QtCore.SIGNAL("currentCellChanged(int,int,int,int)"), self.activeCell)

    def __del__(self):
        if self.spreadsheet:
            if hasattr(self.spreadsheet,"ViewObject"):
                if self.spreadsheet.ViewObject:
                    if hasattr(self.spreadsheet.ViewObject,"editor"):
                        del self.spreadsheet.ViewObject.editor

    def update(self):
        "fills the cells with the contents of the spreadsheet"
        if self.spreadsheet:
            for cell in self.spreadsheet.Proxy._cells.keys():
                c,r = self.spreadsheet.Proxy.splitKey(cell)
                c = "abcdefghijklmnopqrstuvwxyz".index(c)
                r = r-1
                self.table.item(r,c).setText(getattr(self.spreadsheet.Proxy,cell))

    def changeCell(self,r,c):
        "changes the contens of a cell"
        key = "abcdefghijklmnopqrstuvwxyz"[c]+str(r+1)
        value = self.table.item(r,c).text()
        print "Changing "+key+" to "+value
        if self.spreadsheet:
            setattr(self.spreadsheet.Proxy,key,value)
            
    def activeCell(self,r,c,or,oc):
        "sets the contents of the active cell to the header"
        print "setting cell ",r,c

    def clear(self):
        "clears the spreadsheet"
        for r in range(self.table.columnCount):
            for c in range(self.table.rowCount):
                self.table.item(r,c).setText("")


class _CommandSpreadsheet:
    "the Spreadsheet FreeCAD command"
    def GetResources(self):
        return {'Pixmap'  : 'Spreadsheet',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_Create","Spreadsheet"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_Create","Adds a spreadsheet object to the active document")}

    def Activated(self):
        from DraftTools import translate
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Spreadsheet")))
        FreeCADGui.doCommand("import Spreadsheet")
        FreeCADGui.doCommand("Spreadsheet.makeSpreadsheet()")
        FreeCAD.ActiveDocument.commitTransaction()


def makeSpreadsheet():
    "makeSpreadsheet(): adds a spreadsheet object to the active document"
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Spreadsheet")
    Spreadsheet(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheet(obj.ViewObject)
    return obj


def addSpreadsheetView(view):
    "addSpreadsheetView(view): adds the given spreadsheet view to the FreeCAD MDI area"
    if FreeCAD.GuiUp:
        import Spreadsheet_rc
        mdi = FreeCADGui.getMainWindow().findChild(QtGui.QMdiArea)
        mdi.addSubWindow(view)
        
FreeCADGui.addCommand('Spreadsheet_Create',_CommandSpreadsheet())
