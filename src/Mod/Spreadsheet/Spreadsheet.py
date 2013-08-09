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


class MathParser:
    "A math expression parser"
    # code borrowed from http://www.nerdparadise.com/tech/python/parsemath/
    def __init__(self, string, vars={}):
        self.string = string
        self.index = 0
        self.vars = {
            'pi' : math.pi,
            'e' : math.e
            }
        for var in vars.keys():
            if self.vars.get(var) != None:
                raise Exception("Cannot redefine the value of " + var)
            self.vars[var] = vars[var]
    
    def getValue(self):
        value = self.parseExpression()
        self.skipWhitespace()
        if self.hasNext():
            raise Exception(
                "Unexpected character found: '" +
                self.peek() +
                "' at index " +
                str(self.index))
        return value
    
    def peek(self):
        return self.string[self.index:self.index + 1]
    
    def hasNext(self):
        return self.index < len(self.string)
    
    def skipWhitespace(self):
        while self.hasNext():
            if self.peek() in ' \t\n\r':
                self.index += 1
            else:
                return
    
    def parseExpression(self):
        return self.parseAddition()
    
    def parseAddition(self):
        values = [self.parseMultiplication()]
        while True:
            self.skipWhitespace()
            char = self.peek()
            if char == '+':
                self.index += 1
                values.append(self.parseMultiplication())
            elif char == '-':
                self.index += 1
                values.append(-1 * self.parseMultiplication())
            else:
                break
        return sum(values)
    
    def parseMultiplication(self):
        values = [self.parseParenthesis()]
        while True:
            self.skipWhitespace()
            char = self.peek()
            if char == '*':
                self.index += 1
                values.append(self.parseParenthesis())
            elif char == '/':
                div_index = self.index
                self.index += 1
                denominator = self.parseParenthesis()
                if denominator == 0:
                    raise Exception(
                        "Division by 0 kills baby whales (occured at index " +
                        str(div_index) +
                        ")")
                values.append(1.0 / denominator)
            else:
                break
        value = 1.0
        for factor in values:
            value *= factor
        return value
    
    def parseParenthesis(self):
        self.skipWhitespace()
        char = self.peek()
        if char == '(':
            self.index += 1
            value = self.parseExpression()
            self.skipWhitespace()
            if self.peek() != ')':
                raise Exception(
                    "No closing parenthesis found at character "
                    + str(self.index))
            self.index += 1
            return value
        else:
            return self.parseNegative()
    
    def parseNegative(self):
        self.skipWhitespace()
        char = self.peek()
        if char == '-':
            self.index += 1
            return -1 * self.parseParenthesis()
        else:
            return self.parseValue()
    
    def parseValue(self):
        self.skipWhitespace()
        char = self.peek()
        if char in '0123456789.':
            return self.parseNumber()
        else:
            return self.parseVariable()
    
    def parseVariable(self):
        self.skipWhitespace()
        var = ''
        while self.hasNext():
            char = self.peek()
            if char.lower() in '_abcdefghijklmnopqrstuvwxyz0123456789':
                var += char
                self.index += 1
            else:
                break
        
        value = self.vars.get(var, None)
        if value == None:
            raise Exception(
                "Unrecognized variable: '" +
                var +
                "'")
        return float(value)
    
    def parseNumber(self):
        self.skipWhitespace()
        strValue = ''
        decimal_found = False
        char = ''
        
        while self.hasNext():
            char = self.peek()            
            if char == '.':
                if decimal_found:
                    raise Exception(
                        "Found an extra period in a number at character " +
                        str(self.index) +
                        ". Are you European?")
                decimal_found = True
                strValue += '.'
            elif char in '0123456789':
                strValue += char
            else:
                break
            self.index += 1
        
        if len(strValue) == 0:
            if char == '':
                raise Exception("Unexpected end found")
            else:
                raise Exception(
                    "I was expecting to find a number at character " +
                    str(self.index) +
                    " but instead I found a '" +
                    char +
                    "'. What's up with that?")
    
        return float(strValue)

class Spreadsheet:
    """An object representing a spreadsheet. Can be used as a
    FreeCAD object or as a standalone python object.
    Cells of the spreadsheet can be got/set as arguments, as:
    
        myspreadsheet.a1 = 54
        print(myspreadsheet.a1)
        myspreadsheet.a2 = "My text"
        myspreadsheet.b1 = "=a1*3"
        print(myspreadsheet.b1)
    """

    def __init__(self,obj=None):
        if obj:
            obj.Proxy = self
            obj.addProperty("App::PropertyLinkList","Controllers","Base","Cell controllers of this object")
        self._cells = {} # this stores cell contents
        self._relations = {} # this stores relations - currently not used
        self.cols = [] # this stores filled columns
        self.rows = [] # this stores filed rows
        self.Type = "Spreadsheet"

    def __repr__(self):
        return "Spreadsheet object containing " + str(len(self._cells)) + " cells"

    def __setattr__(self, key, value):
        if self.isKey(key):
            key = key.lower()
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
        if key.lower() in self._cells:
            key = key.lower()
            if self.isFunction(self._cells[key]):
                try:
                    e = self.evaluate(key)
                except:
                    print "Error evaluating formula"
                    return None
                else:
                    return e
            else:
                return self._cells[key]
        else:
            return self.__dict__.__getitem__(key)
            
    def __getstate__(self):
        self._cells["Type"] = self.Type
        return self._cells

    def __setstate__(self,state):
        if state:
            self._cells = state
            # extracting Type
            if "Type" in self._cells.keys():
                self.Type = self._cells["Type"]
                del self._cells["Type"]
            # updating relation tables
            self.rows = []
            self.cols = []
            self._relations = {}
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

    def execute(self,obj):
        self.setControlledCells(obj)

    def isFunction(self,key):
        "isFunction(cell): returns True if the given cell or value is a function"
        if key in self._cells:
            if str(self._cells[key])[0] == "=":
                return True
        elif str(key)[0] == "=":
            return True
        else:
            return False
            
    def isNumeric(self,key):
        "isNumeric(cell): returns True if the given cell returns a number"
        if self.isFunction(key):
            res = self.evaluate(key)
        else:
            res = self._cells[key]
        if isinstance(res,float) or isinstance(res,int):
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
        
    def evaluate(self,key):
        "evaluate(key): evaluates the given formula"
        elts = re.split(r'(\W+)',self._cells[key][1:])
        result = ""
        for e in elts:
            if self.isKey(e):
                if self.isFunction(e):
                    if self.isNumeric(e):
                        result += str(self.evaluate(e))
                    else:
                        print "Error evaluating formula"
                        return
                elif self.isNumeric(e):
                    result += str(self._cells[e])
            else:
                result += e
        if DEBUG: print "Evaluating ",result
        try:
            p = MathParser(result)
            result = p.getValue()
        except Exception as (ex):
            msg = ex.message
            raise Exception(msg)
        return result
        
    def setControlledCells(self,obj):
        "Fills the cells that are controlled by a controller"
        if obj:
            if hasattr(obj,"Controllers"):
                for co in obj.Controllers:
                    co.Proxy.setCells(co,obj)
                    
    def getControlledCells(self,obj):
        "returns a list of cells managed by controllers"
        cells = []
        if hasattr(obj,"Controllers"):
            for c in obj.Controllers:
                cells.extend(c.Proxy.getCells(c,obj))
        return cells


class ViewProviderSpreadsheet(object):
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/Spreadsheet.svg"

    def attach(self,vobj):
        self.Object = vobj.Object

    def setEdit(self,vobj,mode):
        if hasattr(self,"editor"):
            pass
        else:
            self.editor = SpreadsheetView(vobj.Object)
            addSpreadsheetView(self.editor)
        return True
    
    def unsetEdit(self,vobj,mode):
        return False
        
    def claimChildren(self):
        if hasattr(self,"Object"):
            if hasattr(self.Object,"Controllers"):
                return self.Object.Controllers
                
    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class SpreadsheetController:
    "A spreadsheet cell controller object"
    def __init__(self,obj):
        obj.Proxy = self
        self.Type = "SpreadsheetController"
        obj.addProperty("App::PropertyLinkList","Objects","Base","The objects that are considered by this controller")
        obj.addProperty("App::PropertyString","Prop","Base","The property to extract from each object")
        obj.addProperty("App::PropertyString","BaseCell","Base","The starting cell of this controller")
        obj.addProperty("App::PropertyEnumeration","Direction","Base","The cells direction of this controller")
        obj.Direction = ["Horizontal","Vertical"]
        
    def execute(self,obj):
        pass
        
    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def getCells(self,obj,spreadsheet):
        "returns a list of cells controlled by this controller"
        cells = []
        if obj.Objects and obj.Prop and obj.BaseCell:
            for i in range(len(obj.Objects)):
                # get the correct cell key
                c,r = spreadsheet.Proxy.splitKey(obj.BaseCell)
                if obj.Direction == "Horizontal":
                    c = "abcdefghijklmnopqrstuvwxyz".index(c)
                    c += i
                    c = "abcdefghijklmnopqrstuvwxyz"[c]
                else:
                    r = int(r) + i
                cells.append(c+str(r))
        return cells
        
        
    def setCells(self,obj,spreadsheet):
        "Fills the controlled cells of the given spreadsheet"
        if obj.Objects and obj.Prop and obj.BaseCell:
            for i in range(len(obj.Objects)):
                # get the correct cell key
                c,r = spreadsheet.Proxy.splitKey(obj.BaseCell)
                if obj.Direction == "Horizontal":
                    c = "abcdefghijklmnopqrstuvwxyz".index(c)
                    c += i
                    c = "abcdefghijklmnopqrstuvwxyz"[c]
                else:
                    r = int(r) + i
                cell = c+str(r)
                if DEBUG: print "auto setting cell ",cell
                if spreadsheet.Proxy.isKey(cell):
                    # get the contents
                    args = obj.Prop.split(".")
                    value = obj.Objects[i]
                    for arg in args:
                        if hasattr(value,arg):
                            value = getattr(value,arg)
                    try:
                        setattr(spreadsheet.Proxy,cell,value)
                        if DEBUG: print "setting cell ",cell," to value ",value
                    except:
                        print "Error retrieving property "+obj.Prop+" from object "+obj.Objects[i].Name              


class ViewProviderSpreadsheetController:
    "A view provider for the spreadsheet cell controller"
    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/SpreadsheetController.svg"


class SpreadsheetView(QtGui.QWidget):
    "A spreadsheet viewer for FreeCAD"

    def __init__(self,spreadsheet=None):
        from DraftTools import translate
        QtGui.QWidget.__init__(self)
        
        self.setWindowTitle(str(translate("Spreadsheet","Spreadsheet")))
        self.setObjectName("Spreadsheet viewer")
        self.verticalLayout = QtGui.QVBoxLayout(self)
        self.doNotChange = False

        # add editor line
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.label = QtGui.QLabel(self)
        self.label.setMinimumSize(QtCore.QSize(82, 0))
        self.label.setText(str(translate("Spreadsheet","Cell"))+" A1 :")
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
        if DEBUG: print "Closing spreadsheet view"
        if self.spreadsheet:
            # before deleting this view, we remove the reference to it in the object
            if hasattr(self.spreadsheet,"ViewObject"):
                if self.spreadsheet.ViewObject:
                    if hasattr(self.spreadsheet.ViewObject.Proxy,"editor"):
                        del self.spreadsheet.ViewObject.Proxy.editor
        if FreeCADGui:
            if FreeCADGui.ActiveDocument:
                FreeCADGui.ActiveDocument.resetEdit()

    def update(self):
        "updates the cells with the contents of the spreadsheet"
        if self.spreadsheet:
            controlled = self.spreadsheet.Proxy.getControlledCells(self.spreadsheet)
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
                if cell in controlled:
                    brush = QtGui.QBrush(QtGui.QColor(255, 0, 0))
                    brush.setStyle(QtCore.Qt.Dense6Pattern)
                    self.table.item(r,c).setBackground(brush)

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
        FreeCAD.ActiveDocument.openTransaction(str(translate("Spreadsheet","Create Spreadsheet")))
        FreeCADGui.doCommand("import Spreadsheet")
        FreeCADGui.doCommand("Spreadsheet.makeSpreadsheet()")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class _Command_Spreadsheet_Controller:
    "the Spreadsheet_Controller FreeCAD command"
    def GetResources(self):
        return {'Pixmap'  : 'SpreadsheetController',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_Controller","Add controller"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_Controller","Adds a cell controller to a selected spreadsheet")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        import Draft
        if Draft.getType(FreeCADGui.Selection.getSelection()[0]) == "Spreadsheet":
            from DraftTools import translate
            n = FreeCADGui.Selection.getSelection()[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Spreadsheet","Add controller")))
            FreeCADGui.doCommand("import Spreadsheet")
            FreeCADGui.doCommand("Spreadsheet.makeSpreadsheetController(FreeCAD.ActiveDocument."+n+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


def makeSpreadsheet():
    "makeSpreadsheet(): adds a spreadsheet object to the active document"
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Spreadsheet")
    Spreadsheet(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheet(obj.ViewObject)
    return obj


def makeSpreadsheetController(spreadsheet,cell=None,objects=None,prop=None,direction=None):
    """makeSpreadsheetController(spreadsheet,[cell,objects,prop,direction]): adds a 
    controller to the given spreadsheet. Call can be a starting cell such as "A5", 
    objects can be a list of objects, prop can be a property such as "Shape.Volume",
    and direction can be "Horizontal" or "Vertical"."""
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","CellController")
    SpreadsheetController(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheetController(obj.ViewObject)
    conts = spreadsheet.Controllers
    conts.append(obj)
    spreadsheet.Controllers = conts
    if objects:
        obj.Objects = objects
    if cell:
        obj.BaseCell = cell
    if prop:
        obj.Prop = prop
    if direction:
        obj.Direction = direction
    return obj


def addSpreadsheetView(view):
    "addSpreadsheetView(view): adds the given spreadsheet view to the FreeCAD MDI area"
    if FreeCAD.GuiUp:
        import Spreadsheet_rc
        mw = FreeCADGui.getMainWindow()
        mdi = mw.findChild(QtGui.QMdiArea)
        sw = mdi.addSubWindow(view)
        sw.setWindowIcon(QtGui.QIcon(":/icons/Spreadsheet.svg"))
        sw.show()
        mdi.setActiveSubWindow(sw)


FreeCADGui.addCommand('Spreadsheet_Create',_Command_Spreadsheet_Create())
FreeCADGui.addCommand('Spreadsheet_Controller',_Command_Spreadsheet_Controller())
