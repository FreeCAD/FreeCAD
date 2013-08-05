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
DEBUG = True # set to True to show debug messages

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
        self._cells = {} # this stores cell contents
        self._relations = {} # this stores relations - currently not used
        self.cols = [] # this stores filled columns
        self.rows = [] # this stores filed rows
        self.Type = "Spreadsheet"

    def __repr__(self):
        return "Spreadsheet object containing " + str(len(self._cells)) + " cells"

    def __setattr__(self, key, value):
        if self.isKey(key):
            if DEBUG: print "Setting key ",key," to value ",value
            if (value == "") or (value == None):
                # remove cell
                if key in self._cells.keys():
                    del self._cells[key]
            else:
                # add cell
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
                    if DEBUG: print "Error evaluating formula"
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
            # updating relation tables
            self.rows = []
            self.cols = []
            self._relations = []
            for key in self._cells.keys():
                r,c = self.splitKey(key)
                if not r in self.rows:
                    self.rows.append(r)
                    self.rows.sort()
                if not c in self.cols:
                    self.cols.append(c)
                    self.cols.sort()
                if self.isFunction(key):
                    self._updateDependencies(key)

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

    def _updateDependencies(self,key,value=None):
        "search for ancestors in the value and updates the table"
        ancestors = []
        if not value:
            value = self._cells[key]
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
        allowMoreThanOneLetter = False
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
                    if not allowMoreThanOneLetter:
                        return False
                    elif nu:
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

    def setEdit(self,vobj,mode):
        if hasattr(self,"editor"):
            pass
        else:
            self.editor = SpreadsheetView(vobj.Object)
            addSpreadsheetView(self.editor)
        return True
    
    def unsetEdit(self,vobj,mode):
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
        self.doNotChange = False
        self.WindowParameter = "Spreadsheet"
        
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
        self.table.setCurrentCell(0,0)
        self.spreadsheet = spreadsheet
        self.update()

        QtCore.QObject.connect(self.table, QtCore.SIGNAL("cellChanged(int,int)"), self.changeCell)
        QtCore.QObject.connect(self.table, QtCore.SIGNAL("currentCellChanged(int,int,int,int)"), self.setEditLine)
        QtCore.QObject.connect(self.lineEdit, QtCore.SIGNAL("returnPressed()"), self.getEditLine)
        QtCore.QObject.connect(self, QtCore.SIGNAL("destroyed()"), self.destroy)

    def destroy(self):
        if DEBUG: print "Closing"
        if self.spreadsheet:
            # before deleting this view, we remove the reference to it in the object
            if hasattr(self.spreadsheet,"ViewObject"):
                if self.spreadsheet.ViewObject:
                    if hasattr(self.spreadsheet.ViewObject.Proxy,"editor"):
                        del self.spreadsheet.ViewObject.Proxy.editor
        FreeCADGui.ActiveDocument.resetEdit()

    def update(self):
        "updates the cells with the contents of the spreadsheet"
        if self.spreadsheet:
            for cell in self.spreadsheet.Proxy._cells.keys():
                c,r = self.spreadsheet.Proxy.splitKey(cell)
                c = "abcdefghijklmnopqrstuvwxyz".index(c)
                r = int(str(r))-1
                content = getattr(self.spreadsheet.Proxy,cell)
                if self.spreadsheet.Proxy.isFunction(cell):
                    self.doNotChange = True
                if content == None:
                    content = ""
                if DEBUG: print "Updating ",cell," to ",content
                if self.table.item(r,c):
                    self.table.item(r,c).setText(str(content))
                else:
                    self.table.setItem(r,c,QtGui.QTableWidgetItem(str(content)))

    def changeCell(self,r,c,value=None):
        "changes the contens of a cell"
        if self.doNotChange:
            if DEBUG: print "DoNotChange flag is set"
            self.doNotChange = False
        elif self.spreadsheet:
            key = "abcdefghijklmnopqrstuvwxyz"[c]+str(r+1)
            if not value:
                value = self.table.item(r,c).text()
            if DEBUG: print "Changing "+key+" to "+value
            # store the entry as best as possible
            try:
                v = int(value)
            except:
                try:
                    v = float(value)
                except:
                    try:
                        v = v = str(value)
                    except:
                        v = value
            setattr(self.spreadsheet.Proxy,key,v)
            self.update()
            # TODO do not update the whole spreadsheet when only one cell has changed:
            # use the _relations table and recursively update only cells based on this one
            self.setEditLine(r,c)

    def setEditLine(self,r,c,orr=None,orc=None):
        "copies the contents of the active cell to the edit line"
        if self.spreadsheet:
            c = "abcdefghijklmnopqrstuvwxyz"[c]
            r = r+1
            if DEBUG: print "Active cell "+c+str(r)
            from DraftTools import translate
            self.label.setText(str(translate("Spreadsheet","Cell"))+" "+c.upper()+str(r)+" :")
            content = self.spreadsheet.Proxy.getFunction(c+str(r))
            if content == None:
                content = ""
            self.lineEdit.setText(str(content))
        
    def getEditLine(self):
        "called when something has been entered in the edit line"
        txt = str(self.lineEdit.text())
        if DEBUG: print "Text edited ",txt
        if txt:
            r = self.table.currentRow()
            c = self.table.currentColumn()
            self.changeCell(r,c,txt)


class _Command_Spreadsheet_Create:
    "the Spreadsheet_Create FreeCAD command"
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
        FreeCAD.ActiveDocument.recompute()


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
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtGui.QMdiArea)
        sw = mdi.addSubWindow(view)
        #mw.setCentralWidget(view) # this causes a crash
        sw.show()
        mdi.setActiveSubWindow(sw)
        
FreeCADGui.addCommand('Spreadsheet_Create',_Command_Spreadsheet_Create())
