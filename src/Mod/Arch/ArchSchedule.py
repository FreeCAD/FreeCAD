# -*- coding: utf8 -*-
#***************************************************************************
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
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

## @package ArchSchedule
#  \ingroup ARCH
#  \brief The Schedule object and tools
#
#  This module provides tools to build Schedule objects.
#  Schedules are objects that can count and gather information
#  about objects in the document, and fill a spreadsheet with the result

__title__ = "Arch Schedule"
__author__ = "Yorik van Havre"
__url__ = "https://www.freecad.org"


verbose = True # change this for silent recomputes



class CommandArchSchedule:

    "the Arch Schedule command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Schedule',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Schedule","Schedule"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Schedule","Creates a schedule to collect data from the model")}

    def Activated(self):
        if hasattr(self,"taskd"):
            if self.taskd:
                self.taskd.form.hide()
        self.taskd = ArchScheduleTaskPanel()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False


class _ArchScheduleDocObserver:

    "doc observer to monitor all recomputes"

    # https://forum.freecad.org/viewtopic.php?style=3&p=553377#p553377

    def __init__(self, doc, schedule):
        self.doc = doc
        self.schedule = schedule

    def slotRecomputedDocument(self, doc):
        if doc != self.doc:
            return
        try:
            self.schedule.Proxy.execute(self.schedule)
        except:
            pass


class _ArchSchedule:

    "the Arch Schedule object"

    def __init__(self,obj):

        self.setProperties(obj)
        obj.Proxy = self
        self.Type = "Schedule"

    def onDocumentRestored(self,obj):

        self.setProperties(obj)
        if hasattr(obj, "Result"):
            self.update_properties_0v21(obj)

    def update_properties_0v21(self,obj):
        sp = obj.Result
        if sp is not None:
            self.setSchedulePropertySpreadsheet(sp, obj)
        obj.removeProperty("Result")
        from draftutils.messages import _wrn
        _wrn("v0.21, " + obj.Label + ", " + translate("Arch", "removed property 'Result', and added property 'AutoUpdate'"))
        if sp is not None:
            _wrn("v0.21, " + sp.Label + ", " + translate("Arch", "added property 'Schedule'"))

    def setProperties(self,obj):

        if not "Description" in obj.PropertiesList:
            obj.addProperty("App::PropertyStringList","Description",       "Arch",QT_TRANSLATE_NOOP("App::Property","The description column"))
        if not "Value" in obj.PropertiesList:
            obj.addProperty("App::PropertyStringList","Value",             "Arch",QT_TRANSLATE_NOOP("App::Property","The values column"))
        if not "Unit" in obj.PropertiesList:
            obj.addProperty("App::PropertyStringList","Unit",              "Arch",QT_TRANSLATE_NOOP("App::Property","The units column"))
        if not "Objects" in obj.PropertiesList:
            obj.addProperty("App::PropertyStringList","Objects",           "Arch",QT_TRANSLATE_NOOP("App::Property","The objects column"))
        if not "Filter" in obj.PropertiesList:
            obj.addProperty("App::PropertyStringList","Filter",            "Arch",QT_TRANSLATE_NOOP("App::Property","The filter column"))
        if not "CreateSpreadsheet" in obj.PropertiesList:
            obj.addProperty("App::PropertyBool",      "CreateSpreadsheet", "Arch",QT_TRANSLATE_NOOP("App::Property","If True, a spreadsheet containing the results is recreated when needed"))
        if not "DetailedResults" in obj.PropertiesList:
            obj.addProperty("App::PropertyBool",      "DetailedResults",   "Arch",QT_TRANSLATE_NOOP("App::Property","If True, additional lines with each individual object are added to the results"))
        if not "AutoUpdate" in obj.PropertiesList:
            obj.addProperty("App::PropertyBool",      "AutoUpdate",        "Arch",QT_TRANSLATE_NOOP("App::Property","If True, the schedule and the associated spreadsheet are updated whenever the document is recomputed"))
            obj.AutoUpdate = True

        # To add the doc observer:
        self.onChanged(obj,"AutoUpdate")

    def setSchedulePropertySpreadsheet(self, sp, obj):
        if not hasattr(sp, "Schedule"):
            sp.addProperty(
                "App::PropertyLink",
                "Schedule",
                "Arch",
                QT_TRANSLATE_NOOP("App::Property", "The Arch Schedule that uses this spreadsheet"))
        sp.Schedule = obj

    def getSpreadSheet(self, obj, force=False):

        """Get the spreadsheet and store it in self.spreadsheet.

        If force is True the spreadsheet is created if required.
        """
        try: # Required as self.spreadsheet may get deleted.
            if getattr(self, "spreadsheet", None) is not None \
                    and getattr(self.spreadsheet, "Schedule", None) == obj:
                return self.spreadsheet
        except:
            pass
        else:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.TypeId == "Spreadsheet::Sheet" \
                        and getattr(o, "Schedule", None) == obj:
                    self.spreadsheet = o
                    return self.spreadsheet
            if force:
                self.spreadsheet = FreeCAD.ActiveDocument.addObject("Spreadsheet::Sheet", "Result")
                self.setSchedulePropertySpreadsheet(self.spreadsheet, obj)
                return self.spreadsheet
            else:
                return None

    def onChanged(self,obj,prop):

        if prop == "CreateSpreadsheet":
            if obj.CreateSpreadsheet:
                self.getSpreadSheet(obj, force=True)
            else:
                sp = self.getSpreadSheet(obj)
                if sp is not None:
                    FreeCAD.ActiveDocument.removeObject(sp.Name)
                    self.spreadsheet = None
        elif prop == "AutoUpdate":
            if obj.AutoUpdate:
                if getattr(self, "docObserver", None) is None:
                    self.docObserver = _ArchScheduleDocObserver(FreeCAD.ActiveDocument, obj)
                    FreeCAD.addDocumentObserver(self.docObserver)
            elif getattr(self, "docObserver", None) is not None:
                FreeCAD.removeDocumentObserver(self.docObserver)
                self.docObserver = None

    def setSpreadsheetData(self,obj,force=False):

        """Fills a spreadsheet with the stored data"""

        if not hasattr(self,"data"):
            self.execute(obj)
        if not hasattr(self,"data"):
            return
        if not self.data:
            return
        if not (obj.CreateSpreadsheet or force):
            return
        sp = self.getSpreadSheet(obj, force=True)
        sp.clearAll()
        # clearAll removes the custom property, we need to re-add it:
        self.setSchedulePropertySpreadsheet(sp, obj)
        # set headers
        sp.set("A1","Description")
        sp.set("B1","Value")
        sp.set("C1","Unit")
        sp.setStyle('A1:C1', 'bold', 'add')
        # write contents
        for k,v in self.data.items():
            sp.set(k,v)
        # recompute
        sp.recompute()
        sp.purgeTouched()   # Remove the confusing blue checkmark from the spreadsheet.
        for o in sp.InList: # Also recompute TechDraw views.
            o.TypeId == "TechDraw::DrawViewSpreadsheet"
            o.recompute()

    def execute(self,obj):

        # verify the data

        if not obj.Description:
            # empty description column
            return
        for p in [obj.Value,obj.Unit,obj.Objects,obj.Filter]:
            # different number of items in each column
            if len(obj.Description) != len(p):
                return

        self.data = {} # store all results in self.data, so it lives even without spreadsheet
        li = 1 # row index - starts at 2 to leave 2 blank rows for the title

        for i in range(len(obj.Description)):
            li += 1
            if not obj.Description[i]:
                # blank line
                continue
            # write description
            self.data["A"+str(li)] = obj.Description[i]
            if verbose:
                l= "OPERATION: "+obj.Description[i]
                print("")
                print (l)
                print (len(l)*"=")

            # build set of valid objects

            objs = obj.Objects[i]
            val = obj.Value[i]
            if val:
                import Draft,Arch
                if objs:
                    objs = objs.split(";")
                    objs = [FreeCAD.ActiveDocument.getObject(o) for o in objs]
                    objs = [o for o in objs if o is not None]
                else:
                    objs = FreeCAD.ActiveDocument.Objects
                if len(objs) == 1:
                    # remove object itself if the object is a group
                    if objs[0].isDerivedFrom("App::DocumentObjectGroup"):
                        objs = objs[0].Group
                objs = Draft.get_group_contents(objs)
                objs = Arch.pruneIncluded(objs,strict=True)
                # Remove all schedules and spreadsheets:
                objs = [o for o in objs if Draft.get_type(o) not in ["Schedule", "Spreadsheet::Sheet"]]
                if obj.Filter[i]:
                    # apply filters
                    nobjs = []
                    for o in objs:
                        props = [p.upper() for p in o.PropertiesList]
                        ok = True
                        for f in obj.Filter[i].split(";"):
                            args = [a.strip() for a in f.strip().split(":")]
                            if args[0][0] == "!":
                                inv = True
                                prop = args[0][1:].upper()
                            else:
                                inv = False
                                prop = args[0].upper()
                            fval = args[1].upper()
                            if prop == "TYPE":
                                prop = "IFCTYPE"
                            if inv:
                                if prop in props:
                                    csprop = o.PropertiesList[props.index(prop)]
                                    if fval in getattr(o,csprop).upper():
                                        ok = False
                            else:
                                if not (prop in props):
                                    ok = False
                                else:
                                    csprop = o.PropertiesList[props.index(prop)]
                                    if not (fval in getattr(o,csprop).upper()):
                                        ok = False
                        if ok:
                            nobjs.append(o)
                    objs = nobjs

                # perform operation: count or retrieve property

                if val.upper() == "COUNT":
                    val = len(objs)
                    if verbose:
                        print (val, ",".join([o.Label for o in objs]))
                    self.data["B"+str(li)] = str(val)
                    if obj.DetailedResults:
                        # additional blank line...
                        li += 1
                        self.data["A"+str(li)] = " "
                else:
                    vals = val.split(".")
                    if vals[0][0].islower():
                        # old-style: first member is not a property
                        vals = vals[1:]
                    sumval = 0

                    # get unit
                    tp = None
                    unit = None
                    q = None
                    if obj.Unit[i]:
                        unit = obj.Unit[i]
                        unit = unit.replace("2","^2")
                        unit = unit.replace("3","^3")
                        unit = unit.replace("²","^2")
                        unit = unit.replace("³","^3")
                        if "2" in unit:
                            tp = FreeCAD.Units.Area
                        elif "3" in unit:
                            tp = FreeCAD.Units.Volume
                        elif "deg" in unit:
                            tp = FreeCAD.Units.Angle
                        else:
                            tp = FreeCAD.Units.Length

                    # format value
                    dv = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2)
                    fs = "{:."+str(dv)+"f}" # format string
                    for o in objs:
                        if verbose:
                            l = o.Name+" ("+o.Label+"):"
                            print (l+(40-len(l))*" ",end="")
                        try:
                            d = o
                            for v in vals:
                                d = getattr(d,v)
                            if hasattr(d,"Value"):
                                d = d.Value
                        except Exception:
                            FreeCAD.Console.PrintWarning(translate("Arch","Unable to retrieve value from object")+": "+o.Name+"."+".".join(vals)+"\n")
                        else:
                            if verbose:
                                if tp and unit:
                                    v = fs.format(FreeCAD.Units.Quantity(d,tp).getValueAs(unit).Value)
                                    print(v,unit)
                                else:
                                    print(fs.format(d))
                            if obj.DetailedResults:
                                li += 1
                                self.data["A"+str(li)] = o.Name+" ("+o.Label+")"
                                if tp and unit:
                                    q = FreeCAD.Units.Quantity(d,tp)
                                    self.data["B"+str(li)] = str(q.getValueAs(unit).Value)
                                    self.data["C"+str(li)] = unit
                                else:
                                    self.data["B"+str(li)] = str(d)

                            if not sumval:
                                sumval = d
                            else:
                                sumval += d
                    val = sumval
                    if tp:
                        q = FreeCAD.Units.Quantity(val,tp)

                    # write data
                    if obj.DetailedResults:
                        li += 1
                        self.data["A"+str(li)] = "TOTAL"
                    if q and unit:
                        self.data["B"+str(li)] = str(q.getValueAs(unit).Value)
                        self.data["C"+str(li)] = unit
                    else:
                        self.data["B"+str(li)] = str(val)
                    if verbose:
                        if tp and unit:
                            v = fs.format(FreeCAD.Units.Quantity(val,tp).getValueAs(unit).Value)
                            print("TOTAL:"+34*" "+v+" "+unit)
                        else:
                            v = fs.format(val)
                            print("TOTAL:"+34*" "+v)
        self.setSpreadsheetData(obj)

    def dumps(self):

        return self.Type

    def loads(self,state):

        if state:
            self.Type = state


class _ViewProviderArchSchedule:

    "A View Provider for Schedules"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        if self.Object.AutoUpdate is False:
            import TechDrawGui
            return ":/icons/TechDraw_TreePageUnsync.svg"
        import Arch_rc
        return ":/icons/Arch_Schedule.svg"

    def isShow(self):
        return True

    def attach(self, vobj):
        self.Object = vobj.Object

    def setEdit(self, vobj, mode=0):
        if mode != 0:
            return None

        self.taskd = ArchScheduleTaskPanel(vobj.Object)
        return True

    def unsetEdit(self, vobj, mode):
        if mode != 0:
            return None

        return True

    def doubleClicked(self, vobj):
        self.edit()

    def setupContextMenu(self, vobj, menu):
        actionEdit = QtGui.QAction(translate("Arch", "Edit"),
                                   menu)
        QtCore.QObject.connect(actionEdit,
                               QtCore.SIGNAL("triggered()"),
                               self.edit)
        menu.addAction(actionEdit)

        if self.Object.CreateSpreadsheet is True:
            msg = translate("Arch", "Remove spreadsheet")
        else:
            msg = translate("Arch", "Attach spreadsheet")
        actionToggleSpreadsheet = QtGui.QAction(QtGui.QIcon(":/icons/Arch_Schedule.svg"),
                                                msg,
                                                menu)
        QtCore.QObject.connect(actionToggleSpreadsheet,
                               QtCore.SIGNAL("triggered()"),
                               self.toggleSpreadsheet)
        menu.addAction(actionToggleSpreadsheet)

    def edit(self):
        FreeCADGui.ActiveDocument.setEdit(self.Object, 0)

    def toggleSpreadsheet(self):
        self.Object.CreateSpreadsheet = not self.Object.CreateSpreadsheet

    def claimChildren(self):
        if hasattr(self,"Object"):
            return [self.Object.Proxy.getSpreadSheet(self.Object)]

    def dumps(self):
        return None

    def loads(self,state):
        return None

    def getDisplayModes(self,vobj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def setDisplayMode(self,mode):
        return mode


class ArchScheduleTaskPanel:

    '''The editmode TaskPanel for Schedules'''

    def __init__(self,obj=None):

        """Sets the panel up"""

        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/ArchSchedule.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Arch_Schedule.svg"))

        # set icons
        self.form.buttonAdd.setIcon(QtGui.QIcon(":/icons/list-add.svg"))
        self.form.buttonDel.setIcon(QtGui.QIcon(":/icons/list-remove.svg"))
        self.form.buttonClear.setIcon(QtGui.QIcon(":/icons/delete.svg"))
        self.form.buttonImport.setIcon(QtGui.QIcon(":/icons/document-open.svg"))
        self.form.buttonExport.setIcon(QtGui.QIcon(":/icons/document-save.svg"))
        self.form.buttonSelect.setIcon(QtGui.QIcon(":/icons/edit-select-all.svg"))

        # restore widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.form.list.setColumnWidth(0,p.GetInt("ScheduleColumnWidth0",100))
        self.form.list.setColumnWidth(1,p.GetInt("ScheduleColumnWidth1",100))
        self.form.list.setColumnWidth(2,p.GetInt("ScheduleColumnWidth2",50))
        self.form.list.setColumnWidth(3,p.GetInt("ScheduleColumnWidth3",100))
        w = p.GetInt("ScheduleDialogWidth",300)
        h = p.GetInt("ScheduleDialogHeight",200)
        self.form.resize(w,h)

        # set delegate - Not using custom delegates for now...
        #self.form.list.setItemDelegate(ScheduleDelegate())
        #self.form.list.setEditTriggers(QtGui.QAbstractItemView.DoubleClicked)

        # connect slots
        QtCore.QObject.connect(self.form.buttonAdd, QtCore.SIGNAL("clicked()"), self.add)
        QtCore.QObject.connect(self.form.buttonDel, QtCore.SIGNAL("clicked()"), self.remove)
        QtCore.QObject.connect(self.form.buttonClear, QtCore.SIGNAL("clicked()"), self.clear)
        QtCore.QObject.connect(self.form.buttonImport, QtCore.SIGNAL("clicked()"), self.importCSV)
        QtCore.QObject.connect(self.form.buttonExport, QtCore.SIGNAL("clicked()"), self.export)
        QtCore.QObject.connect(self.form.buttonSelect, QtCore.SIGNAL("clicked()"), self.select)
        QtCore.QObject.connect(self.form.buttonBox, QtCore.SIGNAL("accepted()"), self.accept)
        QtCore.QObject.connect(self.form.buttonBox, QtCore.SIGNAL("rejected()"), self.reject)
        QtCore.QObject.connect(self.form, QtCore.SIGNAL("rejected()"), self.reject)
        self.form.list.clearContents()

        if self.obj:
            for p in [obj.Value,obj.Unit,obj.Objects,obj.Filter]:
                if len(obj.Description) != len(p):
                    return
            self.form.list.setRowCount(len(obj.Description))
            for i in range(5):
                for j in range(len(obj.Description)):
                    item = QtGui.QTableWidgetItem([obj.Description,obj.Value,obj.Unit,obj.Objects,obj.Filter][i][j])
                    self.form.list.setItem(j,i,item)
            self.form.lineEditName.setText(self.obj.Label)
            self.form.checkDetailed.setChecked(self.obj.DetailedResults)
            self.form.checkSpreadsheet.setChecked(self.obj.CreateSpreadsheet)

        # center over FreeCAD window
        mw = FreeCADGui.getMainWindow()
        self.form.move(mw.frameGeometry().topLeft() + mw.rect().center() - self.form.rect().center())

        # maintain above FreeCAD window
        self.form.setWindowFlags(self.form.windowFlags() | QtCore.Qt.WindowStaysOnTopHint)

        self.form.show()

    def add(self):

        """Adds a new row below the last one"""

        self.form.list.insertRow(self.form.list.currentRow()+1)

    def remove(self):

        """Removes the current row"""

        if self.form.list.currentRow() >= 0:
            self.form.list.removeRow(self.form.list.currentRow())

    def clear(self):

        """Clears the list"""

        self.form.list.clearContents()
        self.form.list.setRowCount(0)

    def importCSV(self):

        """Imports a CSV file"""

        filename = QtGui.QFileDialog.getOpenFileName(QtGui.QApplication.activeWindow(), translate("Arch","Import CSV File"), None, "CSV file (*.csv)");
        if filename:
            filename = filename[0]
            self.form.list.clearContents()
            import csv
            with open(filename,'r') as csvfile:
                r = 0
                for row in csv.reader(csvfile):
                    self.form.list.insertRow(r)
                    for i in range(5):
                        if len(row) > i:
                            t = row[i]
                            #t = t.replace("²","^2")
                            #t = t.replace("³","^3")
                            self.form.list.setItem(r,i,QtGui.QTableWidgetItem(t))
                    r += 1

    def export(self):

        """Exports the results as MD or CSV"""

        # commit latest changes
        self.writeValues()

        # tests
        if not("Up-to-date" in self.obj.State):
            self.obj.Proxy.execute(self.obj)
        if not hasattr(self.obj.Proxy,"data"):
            return
        if not self.obj.Proxy.data:
            return

        filename = QtGui.QFileDialog.getSaveFileName(QtGui.QApplication.activeWindow(),
                                                     translate("Arch","Export CSV File"),
                                                     None,
                                                     "Comma-separated values (*.csv);;TAB-separated values (*.tsv);;Markdown (*.md)");
        if filename:
            filt = filename[1]
            filename = filename[0]
            # add missing extension
            if (not filename.lower().endswith(".csv")) and (not filename.lower().endswith(".tsv")) and (not filename.lower().endswith(".md")):
                if "csv" in filt:
                    filename += ".csv"
                elif "tsv" in filt:
                    filename += ".tsv"
                else:
                    filename += ".md"
            if filename.lower().endswith(".csv"):
                self.exportCSV(filename,delimiter=",")
            elif filename.lower().endswith(".tsv"):
                self.exportCSV(filename,delimiter="\t")
            elif filename.lower().endswith(".md"):
                self.exportMD(filename)
            else:
                FreeCAD.Console.PrintError(translate("Arch","Unable to recognize that file type")+":"+filename+"\n")

    def getRows(self):

        """get the rows that contain data"""

        rows = []
        if hasattr(self.obj.Proxy,"data") and self.obj.Proxy.data:
            for key in self.obj.Proxy.data.keys():
                n = key[1:]
                if not n in rows:
                    rows.append(n)
        rows.sort(key=int)
        return rows

    def exportCSV(self,filename,delimiter="\t"):

        """Exports the results as a CSV/TSV file"""

        import csv
        with open(filename, 'w') as csvfile:
            csvfile = csv.writer(csvfile,delimiter=delimiter)
            csvfile.writerow([translate("Arch","Description"),translate("Arch","Value"),translate("Arch","Unit")])
            if self.obj.DetailedResults:
                csvfile.writerow(["","",""])
            for i in self.getRows():
                r = []
                for j in ["A","B","C"]:
                    if j+i in self.obj.Proxy.data:
                        r.append(str(self.obj.Proxy.data[j+i]))
                    else:
                        r.append("")
                csvfile.writerow(r)
        print("successfully exported ",filename)

    def exportMD(self,filename):

        """Exports the results as a Markdown file"""

        with open(filename, 'w') as mdfile:
            mdfile.write("| "+translate("Arch","Description")+" | "+translate("Arch","Value")+" | "+translate("Arch","Unit")+" |\n")
            mdfile.write("| --- | --- | --- |\n")
            if self.obj.DetailedResults:
                mdfile.write("| | | |\n")
            for i in self.getRows():
                r = []
                for j in ["A","B","C"]:
                    if j+i in self.obj.Proxy.data:
                        r.append(str(self.obj.Proxy.data[j+i]))
                    else:
                        r.append("")
                mdfile.write("| "+" | ".join(r)+" |\n")
        print("successfully exported ",filename)

    def select(self):

        """Adds selected objects to current row"""

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

        """Saves the changes and closes the dialog"""

        # store widths
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        p.SetInt("ScheduleColumnWidth0",self.form.list.columnWidth(0))
        p.SetInt("ScheduleColumnWidth1",self.form.list.columnWidth(1))
        p.SetInt("ScheduleColumnWidth2",self.form.list.columnWidth(2))
        p.SetInt("ScheduleColumnWidth3",self.form.list.columnWidth(3))
        p.SetInt("ScheduleDialogWidth",self.form.width())
        p.SetInt("ScheduleDialogHeight",self.form.height())

        # commit values
        self.writeValues()
        self.form.hide()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def reject(self):

        """Close dialog without saving"""

        self.form.hide()
        FreeCADGui.ActiveDocument.resetEdit()
        return True

    def writeValues(self):

        """commits values and recalculate"""

        if not self.obj:
            self.obj = FreeCAD.ActiveDocument.addObject("App::FeaturePython","Schedule")
            self.obj.Label = translate("Arch","Schedule")
            _ArchSchedule(self.obj)
            if FreeCAD.GuiUp:
                _ViewProviderArchSchedule(self.obj.ViewObject)
            if hasattr(self.obj,"CreateSpreadsheet") and self.obj.CreateSpreadsheet:
                self.obj.Proxy.getSpreadSheet(self.obj, force=True)
        lists = [ [], [], [], [], [] ]
        for i in range(self.form.list.rowCount()):
            for j in range(5):
                cell = self.form.list.item(i,j)
                if cell:
                    lists[j].append(cell.text())
                else:
                    lists[j].append("")
        FreeCAD.ActiveDocument.openTransaction("Edited Schedule")
        self.obj.Description = lists[0]
        self.obj.Value = lists[1]
        self.obj.Unit = lists[2]
        self.obj.Objects = lists[3]
        self.obj.Filter = lists[4]
        self.obj.Label = self.form.lineEditName.text()
        self.obj.DetailedResults = self.form.checkDetailed.isChecked()
        self.obj.CreateSpreadsheet = self.form.checkSpreadsheet.isChecked()
        self.obj.AutoUpdate = self.form.checkAutoUpdate.isChecked()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Schedule',CommandArchSchedule())
