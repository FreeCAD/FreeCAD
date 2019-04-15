# -*- coding: utf8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Yorik van Havre <yorik@uncreated.net>            *
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

import sys
import FreeCAD, time
if FreeCAD.GuiUp:
    import FreeCADGui, Arch_rc, os
    from PySide import QtCore, QtGui
    from DraftTools import translate
    from PySide.QtCore import QT_TRANSLATE_NOOP
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond
    
## @package ArchSchedule
#  \ingroup ARCH
#  \brief The Schedule object and tools
#
#  This module provides tools to build Schedule objects.
#  Schedules are objects that can count and gather information
#  about objects in the document, and fill a spreadsheet with the result

__title__ = "Arch Schedule"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"


verbose = True # change this for silent recomputes


class _CommandArchSchedule:

    "the Arch Schedule command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Schedule',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Schedule","Schedule"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Schedule","Creates a schedule to collect data from the model")}

    def Activated(self):
        taskd = _ArchScheduleTaskPanel()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _ArchSchedule:

    "the Arch Schedule object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyStringList","Description","Arch",QT_TRANSLATE_NOOP("App::Property","The description column"))
        obj.addProperty("App::PropertyStringList","Value",      "Arch",QT_TRANSLATE_NOOP("App::Property","The values column"))
        obj.addProperty("App::PropertyStringList","Unit",       "Arch",QT_TRANSLATE_NOOP("App::Property","The units column"))
        obj.addProperty("App::PropertyStringList","Objects",    "Arch",QT_TRANSLATE_NOOP("App::Property","The objects column"))
        obj.addProperty("App::PropertyStringList","Filter",     "Arch",QT_TRANSLATE_NOOP("App::Property","The filter column"))
        obj.addProperty("App::PropertyLink",      "Result",     "Arch",QT_TRANSLATE_NOOP("App::Property","The spreadsheet to print the results to"))
        obj.Proxy = self
        self.Type = "Schedule"

    def execute(self,obj):
        # fills columns A, B and C of the spreadsheet
        if not obj.Description:
            return
        for p in [obj.Value,obj.Unit,obj.Objects,obj.Filter]:
            if len(obj.Description) != len(p):
                return
        if not hasattr(obj,"Result"):
            # silently fail on old schedule objects
            return
        if not obj.Result:
            FreeCAD.Console.PrintError(translate("Arch","No spreadsheet attached to this schedule")+"\n")
            return
        obj.Result.clearAll()
        obj.Result.set("A1","Description")
        obj.Result.set("B1","Value")
        obj.Result.set("C1","Unit")
        obj.Result.setStyle('A1:C1', 'bold', 'add')
        for i in range(len(obj.Description)):
            if not obj.Description[i]:
                # blank line
                continue
            # write description
            if sys.version_info.major >= 3:
                # use unicode for python3
                obj.Result.set("A"+str(i+2), obj.Description[i])
            else:
                obj.Result.set("A"+str(i+2), obj.Description[i].encode("utf8"))
            if verbose:
                l= "OPERATION: "+obj.Description[i]
                print (l)
                print (len(l)*"=")
            # get list of objects
            objs = obj.Objects[i]
            val = obj.Value[i]
            if val:
                import Draft,Arch
                if objs:
                    objs = objs.split(";")
                    objs = [FreeCAD.ActiveDocument.getObject(o) for o in objs]
                    objs = [o for o in objs if o != None]
                else:
                    objs = FreeCAD.ActiveDocument.Objects
                if len(objs) == 1:
                    # remove object itself if the object is a group
                    if objs[0].isDerivedFrom("App::DocumentObjectGroup"):
                        objs = objs[0].Group
                objs = Draft.getGroupContents(objs,walls=True,addgroups=True)
                objs = Arch.pruneIncluded(objs,strict=True)
                # remove the schedule object and its result from the list
                objs = [o for o in objs if not o == obj]
                objs = [o for o in objs if not o == obj.Result]
                if obj.Filter[i]:
                    # apply filters
                    nobjs = []
                    for o in objs:
                        ok = True
                        for f in obj.Filter[i].split(";"):
                            args = [a.strip() for a in f.strip().split(":")]
                            if args[0].upper() == "NAME":
                                if not(args[1].upper() in o.Name.upper()):
                                    ok = False
                            elif args[0].upper() == "!NAME":
                                if (args[1].upper() in o.Name.upper()):
                                    ok = False
                            elif args[0].upper() == "LABEL":
                                if not(args[1].upper() in o.Label.upper()):
                                    ok = False
                            elif args[0].upper() == "!LABEL":
                                if args[1].upper() in o.Label.upper():
                                    ok = False
                            elif args[0].upper() == "TYPE":
                                if Draft.getType(o).upper() != args[1].upper():
                                    ok = False
                            elif args[0].upper() == "!TYPE":
                                if Draft.getType(o).upper() == args[1].upper():
                                    ok = False
                            elif args[0].upper() == "IFCTYPE":
                                if hasattr(o,"IfcType"):
                                    if o.IfcType.upper() != args[1].upper():
                                        ok = False
                                else:
                                    ok = False
                            elif args[0].upper() == "!IFCTYPE":
                                if hasattr(o,"IfcType"):
                                    if o.IfcType.upper() == args[1].upper():
                                        ok = False
                        if ok:
                            nobjs.append(o)
                    objs = nobjs
                # perform operation
                if val.upper() == "COUNT":
                    val = len(objs)
                    if verbose:
                        print (val, ",".join([o.Label for o in objs]))
                    obj.Result.set("B"+str(i+2),str(val))
                else:
                    vals = val.split(".")
                    sumval = 0
                    for o in objs:
                        if verbose:
                            l = o.Name+" ("+o.Label+"):"
                            print (l+(40-len(l))*" ",)
                        try:
                            d = o
                            for v in vals[1:]:
                                d = getattr(d,v)
                            if verbose:
                                print (d)
                            if hasattr(d,"Value"):
                                d = d.Value
                        except:
                            FreeCAD.Console.PrintWarning(translate("Arch","Unable to retrieve value from object")+": "+o.Name+"."+".".join(vals)+"\n")
                        else:
                            if not sumval:
                                sumval = d
                            else:
                                sumval += d
                    val = sumval
                    # get unit
                    if obj.Unit[i]:
                        ustr = obj.Unit[i]
                        if sys.version_info.major < 3:
                            ustr = ustr.encode("utf8")
                        unit = ustr.replace("²","^2")
                        unit = unit.replace("³","^3")
                        if "2" in unit:
                            tp = FreeCAD.Units.Area
                        elif "3" in unit:
                            tp = FreeCAD.Units.Volume
                        elif "deg" in unit:
                            tp = FreeCAD.Units.Angle
                        else:
                            tp = FreeCAD.Units.Length
                        q = FreeCAD.Units.Quantity(val,tp)
                        obj.Result.set("B"+str(i+2),str(q.getValueAs(unit).Value))
                        obj.Result.set("C"+str(i+2),ustr)
                    else:
                        obj.Result.set("B"+str(i+2),str(val))
                    if verbose:
                        print ("TOTAL:"+34*" "+str(val))
        obj.Result.recompute()

    def __getstate__(self):
        return self.Type

    def __setstate__(self,state):
        if state:
            self.Type = state


class _ViewProviderArchSchedule:

    "A View Provider for Schedules"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Schedule.svg"

    def attach(self, vobj):
        self.Object = vobj.Object

    def setEdit(self,vobj,mode):
        taskd = _ArchScheduleTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def doubleClicked(self,vobj):
        taskd = _ArchScheduleTaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self,vobj,mode):
        FreeCADGui.Control.closeDialog()
        return

    def claimChildren(self):
        if hasattr(self,"Object"):
            return [self.Object.Result]

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode


class _ArchScheduleTaskPanel:

    '''The editmode TaskPanel for Schedules'''

    def __init__(self,obj=None):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchSchedule.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Arch_Schedule.svg"))
        QtCore.QObject.connect(self.form.buttonAdd, QtCore.SIGNAL("clicked()"), self.add)
        QtCore.QObject.connect(self.form.buttonDel, QtCore.SIGNAL("clicked()"), self.remove)
        QtCore.QObject.connect(self.form.buttonClear, QtCore.SIGNAL("clicked()"), self.clear)
        QtCore.QObject.connect(self.form.buttonImport, QtCore.SIGNAL("clicked()"), self.importCSV)
        QtCore.QObject.connect(self.form.buttonExport, QtCore.SIGNAL("clicked()"), self.exportCSV)
        QtCore.QObject.connect(self.form.buttonSelect, QtCore.SIGNAL("clicked()"), self.select)
        self.form.list.clearContents()

        if self.obj:
            if not obj.Description:
                return
            for p in [obj.Value,obj.Unit,obj.Objects,obj.Filter]:
                if len(obj.Description) != len(p):
                    return
            self.form.list.setRowCount(len(obj.Description))
            for i in range(5):
                for j in range(len(obj.Description)):
                    item = QtGui.QTableWidgetItem([obj.Description,obj.Value,obj.Unit,obj.Objects,obj.Filter][i][j])
                    self.form.list.setItem(j,i,item)

    def add(self):
        self.form.list.insertRow(self.form.list.currentRow()+1)

    def remove(self):
        if self.form.list.currentRow() >= 0:
            self.form.list.removeRow(self.form.list.currentRow())

    def clear(self):
        self.form.list.clearContents()
        self.form.list.setRowCount(0)

    def importCSV(self):
        filename = QtGui.QFileDialog.getOpenFileName(QtGui.QApplication.activeWindow(), translate("Arch","Import CSV File"), None, "CSV file (*.csv)");
        if filename:
            self.form.list.clearContents()
            import csv
            with open(filename[0], 'rb') as csvfile:
                r = 0
                for row in csv.reader(csvfile):
                    self.form.list.insertRow(r)
                    for i in range(5):
                        if len(row) > i:
                            t = row[i]
                            t = t.replace("²","^2")
                            t = t.replace("³","^3")
                            self.form.list.setItem(r,i,QtGui.QTableWidgetItem(t))
                    r += 1

    def exportCSV(self):
        if self.obj:
            if self.obj.Result:
                filename = QtGui.QFileDialog.getSaveFileName(QtGui.QApplication.activeWindow(), translate("Arch","Export CSV File"), None, "CSV file (*.csv)");
                if filename:
                    # the following line crashes, couldn't fnid out why
                    # self.obj.Result.exportFile(str(filename[0].encode("utf8")))
                    import csv
                    if not("Up-to-date" in self.obj.State):
                        self.obj.Proxy.execute(self.obj)
                    numrows = len(self.obj.Description)+1
                    with open(filename[0].encode("utf8"), 'wb') as csvfile:
                        csvfile = csv.writer(csvfile,delimiter="\t")
                        for i in range(numrows):
                            r = []
                            for j in ["A","B","C"]:
                                r.append(self.obj.Result.getContents(j+str(i+1)))
                            csvfile.writerow(r)
                    print("successfully exported ",filename[0])

    def select(self):
        if self.form.list.currentRow() >= 0:
            sel = ""
            for o in FreeCADGui.Selection.getSelection():
                if o != self.obj:
                    if sel:
                        sel += ";"
                    sel += o.Name
            if sel:
                self.form.list.setItem(self.form.list.currentRow(),3,QtGui.QTableWidgetItem(sel))

    def accept(self):
        if not self.obj:
            import Spreadsheet
            self.obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Schedule")
            self.obj.Label = translate("Arch","Schedule")
            _ArchSchedule(self.obj)
            sp = FreeCAD.ActiveDocument.addObject("Spreadsheet::Sheet","Result")
            self.obj.Result = sp
            if FreeCAD.GuiUp:
                _ViewProviderArchSchedule(self.obj.ViewObject)
        lists = [ [], [], [], [], [] ]
        for i in range(self.form.list.rowCount()):
            for j in range(5):
                cell = self.form.list.item(i,j)
                if cell:
                    lists[j].append(cell.text())
                else:
                    lists[j].append("")
        self.obj.Description = lists[0]
        self.obj.Value = lists[1]
        self.obj.Unit = lists[2]
        self.obj.Objects = lists[3]
        self.obj.Filter = lists[4]
        FreeCAD.ActiveDocument.recompute()
        return True


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Schedule',_CommandArchSchedule())
