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

"""This is the deprecated spreadsheet module. It is not used anymore
in FreeCAD, but is still there for archiving purposes."""


import re, math, FreeCAD, FreeCADGui
from PySide import QtCore,QtGui
DEBUG = True # set to True to show debug messages

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below

class MathParser:
    "A math expression parser"
    # code adapted from http://www.nerdparadise.com/tech/python/parsemath/
    def __init__(self, string, vars={}):
        self.string = string
        self.index = 0
        self.vars = {
            'pi' : math.pi,
            'e' : math.e
            }
        for var in vars.keys():
            if self.vars.get(var) != None:
                raise RuntimeError("Cannot redefine the value of " + var)
            self.vars[var] = vars[var]

    def getValue(self):
        value = self.parseExpression()
        self.skipWhitespace()
        if self.hasNext():
            raise SyntaxError(
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
                    raise ZeroDivisionError(
                        "Division by 0 kills baby whales (occurred at index " +
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
                raise SyntaxError(
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
            raise ValueError(
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
                    raise SyntaxError(
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
                raise SyntaxError("Unexpected end found")
            else:
                raise SyntaxError(
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

        myspreadsheet = Spreadsheet()
        myspreadsheet.a1 = 54
        print(myspreadsheet.a1)
        myspreadsheet.a2 = "My text"
        myspreadsheet.b1 = "=a1*3"
        print(myspreadsheet.b1)

    The cell names are case-insensitive (a1 = A1)
    """

    def __init__(self,obj=None):
        if obj:
            obj.Proxy = self
            obj.addProperty("App::PropertyLinkList","Controllers","Base","Cell controllers of this object")
            self.Object = obj.Name
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
            if DEBUG: print("Setting key ",key," to value ",value)
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
            self._updateControllers()
        else:
            self.__dict__.__setitem__(key,value)

    def __getattr__(self, key):
        if key.lower() in self._cells:
            key = key.lower()
            if self.isFunction(self._cells[key]):
                try:
                    e = self.evaluate(key)
                except:
                    print("Spreadsheet: Error evaluating formula")
                    return None
                else:
                    return e
            else:
                return self._cells[key]
        else:
            return self.__dict__.__getitem__(key)

    def __setitem__(self, key, value):
        __setattr__(self, key, value)

    def __getitem__(self, key):
        return __getattr__(self, key)

    def __getstate__(self):
        self._cells["Type"] = self.Type
        if hasattr(self,"Object"):
            self._cells["Object"] = self.Object
        return self._cells

    def __setstate__(self,state):
        if state:
            self._cells = state
            # extracting Type
            if "Type" in self._cells.keys():
                self.Type = self._cells["Type"]
                del self._cells["Type"]
            if "Object" in self._cells.keys():
                self.Object = self._cells["Object"]
                del self._cells["Object"]
            # updating relation tables
            self.rows = []
            self.cols = []
            self._relations = {}
            for key in self._cells.keys():
                c,r = self.splitKey(key)
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

    def _updateControllers(self):
        "triggers the property controllers"
        if hasattr(self,"Object"):
            obj = FreeCAD.ActiveDocument.getObject(self.Object)
            if obj:
                import Draft
                if Draft.getType(obj) == "Spreadsheet":
                    if hasattr(obj,"Controllers"):
                        for co in obj.Controllers:
                            if Draft.getType(co) == "SpreadsheetPropertyController":
                                co.Proxy.execute(co)

    def execute(self,obj):
        pass

    def isFunction(self,key):
        "isFunction(cell): returns True if the given cell or value is a function"
        if str(key).lower() in self._cells:
            key = key.lower()
            if str(self._cells[key])[0] == "=":
                return True
        elif str(key)[0] == "=":
            return True
        else:
            return False

    def isNumeric(self,key):
        "isNumeric(cell): returns True if the given cell returns a number"
        key = key.lower()
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
        key = key.lower()
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
        key = key.lower()
        elts = re.split(r'(\W+)',self._cells[key][1:])
        result = ""
        for e in elts:
            if self.isKey(e):
                if self.isFunction(e):
                    if self.isNumeric(e):
                        result += str(self.evaluate(e))
                    else:
                        print("Spreadsheet: Error evaluating formula")
                        return
                elif self.isNumeric(e):
                    result += str(self._cells[e.lower()])
            else:
                result += e
        if DEBUG: print("Evaluating ",result)
        try:
            p = MathParser(result)
            result = p.getValue()
        except Exception as ex:
            raise #
            #msg = ex.message
            #raise Exception(msg) #would discard the type
        return result

    def recompute(self,obj):
        "Fills the controlled cells and properties"
        if obj:
            if hasattr(obj,"Controllers"):
                import Draft
                for co in obj.Controllers:
                    if Draft.getType(co) == "SpreadsheetController":
                        co.Proxy.setCells(co,obj)
                    elif Draft.getType(co) == "SpreadsheetPropertyController":
                        co.Proxy.compute(co)

    def getControlledCells(self,obj):
        "returns a list of cells managed by controllers"
        cells = []
        if hasattr(obj,"Controllers"):
            import Draft
            for co in obj.Controllers:
                if Draft.getType(co) == "SpreadsheetController":
                    cells.extend(co.Proxy.getCells(co,obj))
        return cells

    def getControllingCells(self,obj):
        "returns a list of controlling cells managed by controllers"
        cells = []
        if hasattr(obj,"Controllers"):
            import Draft
            for co in obj.Controllers:
                if Draft.getType(co) == "SpreadsheetPropertyController":
                    if co.Cell:
                        cells.append(co.Cell.lower())
        return cells


class ViewProviderSpreadsheet(object):
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/Spreadsheet.svg"

    def attach(self,vobj):
        self.Object = vobj.Object

    def setEdit(self,vobj,mode=0):
        if hasattr(self,"editor"):
            pass
        else:
            self.editor = SpreadsheetView(vobj.Object)
            addSpreadsheetView(self.editor)
        return True

    def unsetEdit(self,vobj,mode=0):
        return False

    def doubleClicked(self,vobj):
        self.setEdit(vobj)

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
        obj.addProperty("App::PropertyEnumeration","FilterType","Filter","The type of filter to apply to the scene objects")
        obj.addProperty("App::PropertyString","Filter","Filter","The filter to apply to the scene objects")
        obj.addProperty("App::PropertyEnumeration","DataType","Data","The type of data to extract from the objects")
        obj.addProperty("App::PropertyString","Data","Data","The data to extract from the objects")
        obj.addProperty("App::PropertyString","BaseCell","Base","The starting cell of this controller")
        obj.addProperty("App::PropertyEnumeration","Direction","Base","The cells direction of this controller")
        obj.FilterType = ["Object Type","Object Name"]
        obj.DataType = ["Get Property","Count"]
        obj.Direction = ["Horizontal","Vertical"]

    def execute(self,obj):
        pass

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def onChanged(self,obj,prop):
        if prop == "DataType":
            if obj.DataType == "Count":
                obj.setEditorMode('Data',1)
            else:
                obj.setEditorMode('Data',0)

    def getDataSet(self,obj):
        "returns a list of objects to be considered by this controller"
        result = []
        if hasattr(obj,"FilterType"):
            import Draft
            baseset = FreeCAD.ActiveDocument.Objects
            if obj.FilterType == "Object Type":
                for o in baseset:
                    if not ("Spreadsheet" in Draft.getType(o)):
                        t = Draft.getType(o)
                        if t == "Part":
                            t = obj.TypeId
                        if obj.Filter:
                            if obj.Filter in t:
                                result.append(o)
                        else:
                            result.append(o)
            elif obj.FilterType == "Object Name":
                for o in baseset:
                    if not ("Spreadsheet" in Draft.getType(o)):
                        if obj.Filter:
                            if obj.Filter in o.Label:
                                    result.append(o)
                        else:
                            result.append(o)
        return result

    def getCells(self,obj,spreadsheet):
        "returns a list of cells controlled by this controller"
        cells = []
        if obj.BaseCell:
            if obj.DataType == "Count":
                return obj.BaseCell
            for i in range(len(self.getDataSet(obj))):
                # get the correct cell key
                c,r = spreadsheet.Proxy.splitKey(obj.BaseCell)
                if obj.Direction == "Horizontal":
                    c = c.lower()
                    c = "abcdefghijklmnopqrstuvwxyz".index(c)
                    c += i
                    c = "abcdefghijklmnopqrstuvwxyz"[c]
                else:
                    r = int(r) + i
                cells.append(c+str(r))
        return cells

    def setCells(self,obj,spreadsheet):
        "Fills the controlled cells of the given spreadsheet"
        if obj.BaseCell:
            dataset = self.getDataSet(obj)
            if obj.DataType == "Count":
                if spreadsheet.Proxy.isKey(obj.BaseCell):
                    try:
                        setattr(spreadsheet.Proxy,obj.BaseCell,len(dataset))
                    except:
                        print("Spreadsheet: Error counting objects")
            elif obj.Data:
                for i in range(len(dataset)):
                    # get the correct cell key
                    c,r = spreadsheet.Proxy.splitKey(obj.BaseCell)
                    if obj.Direction == "Horizontal":
                        c = c.lower()
                        c = "abcdefghijklmnopqrstuvwxyz".index(c)
                        c += i
                        c = "abcdefghijklmnopqrstuvwxyz"[c]
                    else:
                        r = int(r) + i
                    cell = c+str(r)
                    if DEBUG: print("auto setting cell ",cell)
                    if spreadsheet.Proxy.isKey(cell):
                        # get the contents
                        args = obj.Data.split(".")
                        value = dataset[i]
                        for arg in args:
                            print(arg)
                            if hasattr(value,arg):
                                value = getattr(value,arg)
                        try:
                            if isinstance(value,float) or isinstance(value,int):
                                pass
                            else:
                                value = str(value)
                                value = ''.join([ c for c in value if c not in ('<','>',':')])
                            setattr(spreadsheet.Proxy,cell,value)
                            if DEBUG: print("setting cell ",cell," to value ",value)
                        except:
                            print("Spreadsheet: Error retrieving property "+obj.Data+" from object "+dataset[i].Name)


class ViewProviderSpreadsheetController:
    "A view provider for the spreadsheet cell controller"
    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/SpreadsheetController.svg"


class SpreadsheetPropertyController:
    "A spreadsheet property controller object"
    def __init__(self,obj):
        obj.Proxy = self
        self.Type = "SpreadsheetPropertyController"
        obj.addProperty("App::PropertyEnumeration","TargetType","Base","The type of item to control")
        obj.addProperty("App::PropertyLink","TargetObject","Base","The object that must be controlled")
        obj.addProperty("App::PropertyString","TargetProperty","Base","The property or constraint of the target object to control")
        obj.addProperty("App::PropertyString","Cell","Base","The cell that contains the value to apply to the property")
        obj.TargetType = ["Property","Constraint"]

    def execute(self,obj):
        pass

    def compute(self,obj):
        if obj.Cell and obj.TargetObject and obj.TargetProperty and obj.InList:
            sp = obj.InList[0]
            import Draft
            if Draft.getType(sp) == "Spreadsheet":
                try:
                    value = getattr(sp.Proxy,obj.Cell)
                except:
                    if DEBUG: print("No value for cell ",obj.Cell," in spreadsheet.")
                    return
                if obj.TargetType == "Property":
                    b = obj.TargetObject
                    props = obj.TargetProperty.split(".")
                    for p in props:
                        if hasattr(b,p):
                            if p != props[-1]:
                                b = getattr(b,p)
                        else:
                            return
                    try:
                        setattr(b,p,value)
                        FreeCAD.ActiveDocument.recompute()
                        if DEBUG: print("setting property ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)
                    except:
                        if DEBUG: print("unable to set property ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)
                else:
                    if Draft.getType(obj.TargetObject) == "Sketch":
                        if obj.TargetProperty.isdigit():
                            # try setting by constraint id
                            try:
                                c = int(obj.TargetProperty)
                                obj.TargetObject.setDatum(c,float(value))
                                FreeCAD.ActiveDocument.recompute()
                                if DEBUG: print("setting constraint ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)
                            except:
                                if DEBUG: print("unable to set constraint ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)
                        else:
                            # try setting by constraint name
                            try:
                                obj.TargetObject.setDatum(obj.TargetProperty,float(value))
                                FreeCAD.ActiveDocument.recompute()
                                if DEBUG: print("setting constraint ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)
                            except:
                                if DEBUG: print("unable to set constraint ",obj.TargetProperty, " of object ",obj.TargetObject.Name, " to ",value)


    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state

    def onChanged(self,obj,prop):
        pass


class ViewProviderSpreadsheetPropertyController:
    "A view provider for the spreadsheet property controller"
    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Spreadsheet_rc
        return ":/icons/SpreadsheetPropertyController.svg"


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
        self.label.setText(translate("Spreadsheet","Cell")+" A1 :")
        self.lineEdit = QtGui.QLineEdit(self)
        self.applyButton = QtGui.QPushButton(self)
        self.applyButton.setText(translate("Spreadsheet","Apply"))
        self.applyButton.setIcon(QtGui.QIcon(":/icons/edit_OK.svg"))
        self.applyButton.setToolTip(translate("Spreadsheet","Apply the changes to the current cell"))
        self.wipeButton = QtGui.QPushButton(self)
        self.wipeButton.setText(translate("Spreadsheet","Delete"))
        self.wipeButton.setIcon(QtGui.QIcon(":/icons/process-stop.svg"))
        self.wipeButton.setToolTip(translate("Spreadsheet","Deletes the contents of the current cell"))
        self.computeButton = QtGui.QPushButton(self)
        self.computeButton.setText(translate("Spreadsheet","Compute"))
        self.computeButton.setIcon(QtGui.QIcon(":/icons/view-refresh.svg"))
        self.computeButton.setToolTip(translate("Spreadsheet","Updates the values handled by controllers"))
        self.horizontalLayout.addWidget(self.label)
        self.horizontalLayout.addWidget(self.lineEdit)
        self.horizontalLayout.addWidget(self.applyButton)
        self.horizontalLayout.addWidget(self.wipeButton)
        self.horizontalLayout.addWidget(self.computeButton)
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
        QtCore.QObject.connect(self.applyButton, QtCore.SIGNAL("clicked()"), self.getEditLine)
        QtCore.QObject.connect(self.wipeButton, QtCore.SIGNAL("clicked()"), self.wipeCell)
        QtCore.QObject.connect(self.computeButton, QtCore.SIGNAL("clicked()"), self.recompute)

    def closeEvent(self, event):
        #if DEBUG: print("Closing spreadsheet view")
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
            controlling = self.spreadsheet.Proxy.getControllingCells(self.spreadsheet)
            for cell in self.spreadsheet.Proxy._cells.keys():
                if not cell in ["Type","Object"]:
                    c,r = self.spreadsheet.Proxy.splitKey(cell)
                    c = "abcdefghijklmnopqrstuvwxyz".index(c)
                    r = int(str(r))-1
                    content = getattr(self.spreadsheet.Proxy,cell)
                    if self.spreadsheet.Proxy.isFunction(cell):
                        self.doNotChange = True
                    if content == None:
                        content = ""
                    if DEBUG: print("Updating ",cell," to ",content)
                    if self.table.item(r,c):
                        self.table.item(r,c).setText(str(content))
                    else:
                        self.table.setItem(r,c,QtGui.QTableWidgetItem(str(content)))
                    if cell in controlled:
                        brush = QtGui.QBrush(QtGui.QColor(255, 0, 0))
                        brush.setStyle(QtCore.Qt.Dense6Pattern)
                        if self.table.item(r,c):
                            self.table.item(r,c).setBackground(brush)
                    elif cell in controlling:
                        brush = QtGui.QBrush(QtGui.QColor(0, 0, 255))
                        brush.setStyle(QtCore.Qt.Dense6Pattern)
                        if self.table.item(r,c):
                            self.table.item(r,c).setBackground(brush)
                    else:
                        brush = QtGui.QBrush()
                        if self.table.item(r,c):
                            self.table.item(r,c).setBackground(brush)

    def changeCell(self,r,c,value=None):
        "changes the contents of a cell"
        if self.doNotChange:
            if DEBUG: print("DoNotChange flag is set")
            self.doNotChange = False
        elif self.spreadsheet:
            key = "abcdefghijklmnopqrstuvwxyz"[c]+str(r+1)
            if value == None:
                value = self.table.item(r,c).text()
            if value == "":
                if DEBUG: print("Wiping "+key)
                if self.table.item(r,c):
                    self.table.item(r,c).setText("")
                if key in self.spreadsheet.Proxy._cells.keys():
                    del self.spreadsheet.Proxy._cells[key]
            else:
                if DEBUG: print("Changing "+key+" to "+value)
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
            if DEBUG: print("Active cell "+c+str(r))
            from DraftTools import translate
            self.label.setText(str(translate("Spreadsheet","Cell"))+" "+c.upper()+str(r)+" :")
            content = self.spreadsheet.Proxy.getFunction(c+str(r))
            if content == None:
                content = ""
            self.lineEdit.setText(str(content))

    def getEditLine(self):
        "called when something has been entered in the edit line"
        txt = str(self.lineEdit.text())
        if DEBUG: print("Text edited ",txt)
        r = self.table.currentRow()
        c = self.table.currentColumn()
        self.changeCell(r,c,txt)

    def wipeCell(self):
        if DEBUG: print("Wiping cell")
        self.lineEdit.setText("")
        self.getEditLine()

    def recompute(self):
        if self.spreadsheet:
            self.spreadsheet.Proxy.recompute(self.spreadsheet)
        self.update()

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
        FreeCADGui.doCommand("s = Spreadsheet.makeSpreadsheet()")
        FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")
        FreeCADGui.doCommand("FreeCADGui.ActiveDocument.setEdit(s.Name,0)")
        FreeCAD.ActiveDocument.commitTransaction()


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


class _Command_Spreadsheet_PropertyController:
    "the Spreadsheet_Controller FreeCAD command"
    def GetResources(self):
        return {'Pixmap'  : 'SpreadsheetPropertyController',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_PropertyController","Add property controller"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Spreadsheet_PropertyController","Adds a property controller to a selected spreadsheet")}

    def IsActive(self):
        if FreeCADGui.Selection.getSelection():
            return True
        else:
            return False

    def Activated(self):
        import Draft
        from DraftTools import translate
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1) and Draft.getType(sel[0]) == "Spreadsheet":
            n = FreeCADGui.Selection.getSelection()[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Spreadsheet","Add property controller")))
            FreeCADGui.doCommand("import Spreadsheet")
            FreeCADGui.doCommand("Spreadsheet.makeSpreadsheetPropertyController(FreeCAD.ActiveDocument."+n+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        elif (len(sel) == 2):
            if (Draft.getType(sel[0]) == "Spreadsheet") and (Draft.getType(sel[1]) == "SpreadsheetPropertyController"):
                s = sel[0].Name
                o = sel[1].Name
            elif (Draft.getType(sel[1]) == "Spreadsheet") and (Draft.getType(sel[0]) == "SpreadsheetPropertyController"):
                s = sel[1].Name
                o = sel[0].Name
            else:
                return
            FreeCAD.ActiveDocument.openTransaction(str(translate("Spreadsheet","Add property controller")))
            FreeCADGui.doCommand("import Spreadsheet")
            FreeCADGui.doCommand("Spreadsheet.makeSpreadsheetPropertyController(FreeCAD.ActiveDocument."+s+",FreeCAD.ActiveDocument."+o+")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


def makeSpreadsheet():
    "makeSpreadsheet(): adds a spreadsheet object to the active document"
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Spreadsheet")
    Spreadsheet(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheet(obj.ViewObject)
    return obj


def makeSpreadsheetController(spreadsheet,cell=None,direction=None):
    """makeSpreadsheetController(spreadsheet,[cell,direction]): adds a
    controller to the given spreadsheet. Call can be a starting cell such as "A5",
    and direction can be "Horizontal" or "Vertical"."""
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","CellController")
    SpreadsheetController(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheetController(obj.ViewObject)
    conts = spreadsheet.Controllers
    conts.append(obj)
    spreadsheet.Controllers = conts
    if cell:
        obj.BaseCell = cell
    if direction:
        obj.Direction = direction
    return obj


def makeSpreadsheetPropertyController(spreadsheet,object=None,prop=None,cell=None):
    """makeSpreadsheetPropertyController(spreadsheet,[object,prop,cell]): adds a
    property controller, targeting the given object if any, to the given spreadsheet.
    You can give a property (such as "Length" or "Proxy.Length") and a cell address
    (such as "B6")."""
    obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","PropertyController")
    SpreadsheetPropertyController(obj)
    if FreeCAD.GuiUp:
        ViewProviderSpreadsheetPropertyController(obj.ViewObject)
    conts = spreadsheet.Controllers
    conts.append(obj)
    spreadsheet.Controllers = conts
    if cell:
        obj.Cell = cell
    if prop:
        obj.Property = prop
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


def open(filename):
    "called when freecad opens a csv file"
    import os
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    doc.recompute()
    return doc


def insert(filename,docname):
    "called when freecad wants to import a csv file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    doc.recompute()
    return doc


def read(filename):
    "creates a spreadsheet with the contents of a csv file"
    sp = makeSpreadsheet()
    import csv
    with pyopen(filename, 'rb') as csvfile:
        csvfile = csv.reader(csvfile)
        rn = 1
        for row in csvfile:
            cn = 0
            for c in row[:26]:
                cl = "abcdefghijklmnopqrstuvwxyz"[cn]
                #print("setting ",cl+str(rn)," ",c)
                try:
                    c = int(c)
                except ValueError:
                    try:
                        c = float(c)
                    except ValueError:
                        c = str(c)
                setattr(sp.Proxy,cl+str(rn),c)
                cn += 1
            rn += 1
    print("successfully imported ",filename)


def export(exportList,filename):
    "called when freecad exports a csv file"
    import csv, Draft
    if not exportList:
        print("Spreadsheet: Nothing to export")
        return
    obj = exportList[0]
    if Draft.getType(obj) != "Spreadsheet":
        print("Spreadsheet: The selected object is not a spreadsheet")
        return
    if not obj.Proxy._cells:
        print("Spreadsheet: The selected spreadsheet contains no cell")
        return
    numcols = ("abcdefghijklmnopqrstuvwxyz".index(str(obj.Proxy.cols[-1])))+1
    numrows = int(obj.Proxy.rows[-1])
    with pyopen(filename, 'wb') as csvfile:
        csvfile = csv.writer(csvfile)
        for i in range(numrows):
            r = []
            for j in range(numcols):
                key = "abcdefghijklmnopqrstuvwxyz"[j]+str(i+1)
                if key in obj.Proxy._cells.keys():
                    r.append(str(obj.Proxy.getFunction(key)))
                else:
                    r.append("")
            csvfile.writerow(r)
    print("successfully exported ",filename)


#FreeCADGui.addCommand('Spreadsheet_Create',_Command_Spreadsheet_Create())
#FreeCADGui.addCommand('Spreadsheet_Controller',_Command_Spreadsheet_Controller())
#FreeCADGui.addCommand('Spreadsheet_PropertyController',_Command_Spreadsheet_PropertyController())
