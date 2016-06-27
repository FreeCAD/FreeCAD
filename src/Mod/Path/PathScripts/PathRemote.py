# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic  <shopinthewoods@gmail.com>              *
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
import Path
from PathScripts import PathUtils
import urllib2
import json

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui

__title__ = "Path Remote Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"

"""Path Remote processing object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectRemote:

    def __init__(self, obj):

        obj.addProperty("App::PropertyLinkSubList", "Base", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool", "Active", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString", "Comment", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional comment for this profile"))
        obj.addProperty("App::PropertyString", "UserLabel", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","User Assigned Label"))

        obj.addProperty("App::PropertyString", "URL", "API", QtCore.QT_TRANSLATE_NOOP("App::Property","The Base URL of the remote path service"))
        obj.addProperty("App::PropertyStringList", "proplist", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","list of remote properties"))
        obj.setEditorMode('proplist', 2)  # make this hidden

        # Tool Properties
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber", "Tool",QtCore.QT_TRANSLATE_NOOP("App::Property","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)
        obj.setEditorMode('ToolNumber', 1)  # make this read only
        obj.addProperty("App::PropertyString", "ToolDescription", "Tool", QtCore.QT_TRANSLATE_NOOP("App::Property","The description of the tool "))
        obj.setEditorMode('ToolDescription', 1)  # make this read onlyt

        # Depth Properties
        obj.addProperty("App::PropertyFloat", "ClearanceHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyFloat", "SafeHeight", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Step", QtCore.QT_TRANSLATE_NOOP("App::Property","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.01, 100.0, 0.5)
        obj.addProperty("App::PropertyFloat", "StartDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyFloat", "FinalDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyFloat", "FinishDepth", "Depth", QtCore.QT_TRANSLATE_NOOP("App::Property","Maximum material removed on final pass."))

        obj.Proxy = self

    def addbaseobject(self, obj, ss, sub=""):
        baselist = obj.Base
        if len(baselist) == 0:  # When adding the first base object, guess at heights
            try:
                bb = ss.Shape.BoundBox  # parent boundbox
                subobj = ss.Shape.getElement(sub)
                fbb = subobj.BoundBox  # feature boundbox
                obj.StartDepth = bb.ZMax
                obj.ClearanceHeight = bb.ZMax + 5.0
                obj.SafeHeight = bb.ZMax + 3.0

                if fbb.ZMax < bb.ZMax:
                    obj.FinalDepth = fbb.ZMax
                else:
                    obj.FinalDepth = bb.ZMin
            except:
                obj.StartDepth = 5.0
                obj.ClearanceHeight = 10.0
                obj.SafeHeight = 8.0

        item = (ss, sub)
        if item in baselist:
            FreeCAD.Console.PrintWarning("this object already in the list" + "\n")
        else:
            baselist.append(item)
        obj.Base = baselist
        self.execute(obj)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):

        "'''Do something when a property has changed'''"
        if prop == "URL":
            url = obj.URL + "/api/v1.0/properties"
            try:
                response = urllib2.urlopen(url)
            except:
                print "service not defined or not responding"
                print "len: " + str(len(obj.proplist))
                if len(obj.proplist) != 0:
                    for prop in obj.proplist:
                        print "removing: " + str(prop)
                        obj.removeProperty(prop)
                    pl = obj.proplist
                    pl = []
                    obj.proplist = pl
                return

            data = json.load(response)

            properties = data['properties']
            for prop in obj.proplist:
                print "removing: " + str(prop)
                obj.removeProperty(prop)

            pl = obj.proplist
            pl = []
            for prop in properties:
                obj.addProperty(
                        prop['type'],
                        prop['propertyname'],
                        "Remote",
                        prop['description'])
                pl.append(prop['propertyname'])
                print "adding: " + str(prop)
            obj.proplist = pl

        if prop == "UserLabel":
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

    def execute(self, obj):
        output = ""
        if obj.Comment != "":
            output += '(' + str(obj.Comment)+')\n'

        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.vertFeed = 100
            self.horizFeed = 100
            self.radius = 0.25
            obj.ToolNumber = 0
            obj.ToolDescription = "UNDEFINED"
        else:
            self.vertFeed = toolLoad.VertFeed.Value
            self.horizFeed = toolLoad.HorizFeed.Value
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            self.radius = tool.Diameter/2
            obj.ToolNumber = toolLoad.ToolNumber
            obj.ToolDescription = toolLoad.Name

        if obj.UserLabel == "":
            obj.Label = obj.Name + " :" + obj.ToolDescription
        else:
            obj.Label = obj.UserLabel + " :" + obj.ToolDescription

        output += "(remote gcode goes here)"

        if obj.Active:
            path = Path.Path(output)
            obj.Path = path
            obj.ViewObject.Visibility = True

        else:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            obj.ViewObject.Visibility = False


class ViewProviderRemote:
    def __init__(self, obj):
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def getIcon(self):
        return ":/icons/Path-Remote.svg"

    def onChanged(self, obj, prop):
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self, obj, prop):  # optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        taskd = TaskPanel()
        taskd.obj = vobj.Object
        FreeCADGui.Control.showDialog(taskd)
        taskd.setupUi()
        return True

    def unsetEdit(self, vobj, mode):
        # this is executed when the user cancels or terminates edit mode
        pass


class _RefreshRemotePath:
    def GetResources(self):
        return {'Pixmap': 'Path-Refresh',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Remote", "Refresh Remote Path Data"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Remote", "Refresh Remote Path Data")}

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def refresh(self):
        obj = FreeCADGui.Selection.getSelection()[0]
        values = {}

        for i in obj.PropertiesList:
            if obj.getGroupOfProperty(i) in ["Remote"]:
                values.update({i: obj.getPropertyByName(i)})

            if obj.getGroupOfProperty(i) in ["Depth"]:
                print str(i)
                values.update({i: obj.getPropertyByName(i)})

            if obj.getGroupOfProperty(i) in ["Step"]:
                values.update({i: obj.getPropertyByName(i)})

            if obj.getGroupOfProperty(i) in ["Tool"]:
                tool = PathUtils.getTool(obj, obj.ToolNumber)
                if tool:
                    tradius = tool.Diameter/2
                    tlength = tool.LengthOffset
                    ttype = tool.ToolType
                else:
                    tradius = 0.25
                    tlength = 1
                    ttype = "undefined"

                values.update({"tool_diameter": tradius})
                values.update({"tool_length": tlength})
                values.update({"tool_type": ttype})

        payload = json.dumps(values)

        url = obj.URL + "/api/v1.0/path"
        print url
        try:
            req = urllib2.Request(url)
            req.add_header('Content-Type', 'application/json')
            response = urllib2.urlopen(req, payload)
            data = json.load(response)
        except:
            print "service not defined or not responding"
            return

        path = data['path']
        output = ""
        for command in path:
            output += command['command']
        path = Path.Path(output)
        obj.Path = path

    def Activated(self):
        self.refresh()


class CommandPathRemote:

    def GetResources(self):
        return {'Pixmap': 'Path-Remote',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Remote", "Remote"),
                'Accel': "P, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Remote", "Request a Path from a remote cloud service")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):
        ztop = 10.0
        zbottom = 0.0

        FreeCAD.ActiveDocument.openTransaction(translate("Path_Remote", "Create remote path operation"))
        FreeCADGui.addModule("PathScripts.PathRemote")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Remote")')
        FreeCADGui.doCommand('PathScripts.PathRemote.ObjectRemote(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathRemote.ViewProviderRemote(obj.ViewObject)')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StartDepth = ' + str(ztop))
        FreeCADGui.doCommand('obj.SafeHeight = ' + str(ztop + 2))
        FreeCADGui.doCommand('obj.StepDown = ' + str((ztop-zbottom)/8))

        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.doCommand('obj.ViewObject.startEditing()')


class TaskPanel:
    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/RemoteEdit.ui")

    def accept(self):
        self.getFields()

        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getRemoteFields(self):
        self.getFields()
        self.obj.URL = self.form.remoteURL.text()
        print "getRemote:320"

    def getFields(self):
        if self.obj:
            if hasattr(self.obj, "StartDepth"):
                self.obj.StartDepth = float(self.form.startDepth.text())
            if hasattr(self.obj, "FinalDepth"):
                self.obj.FinalDepth = float(self.form.finalDepth.text())
            if hasattr(self.obj, "SafeHeight"):
                self.obj.SafeHeight = float(self.form.safeHeight.text())
            if hasattr(self.obj, "ClearanceHeight"):
                self.obj.ClearanceHeight = float(self.form.clearanceHeight.text())
            if hasattr(self.obj, "StepDown"):
                self.obj.StepDown = float(self.form.stepDown.value())

        self.obj.Proxy.execute(self.obj)

    def open(self):
        self.s = SelObserver()
        FreeCADGui.Selection.addObserver(self.s)

    def addBase(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()

        if not len(selection) >= 1:
            FreeCAD.Console.PrintError(translate("PathProject", "Please select at least one suitable object\n"))
            return
        for s in selection:
            if s.HasSubObjects:
                for i in s.SubElementNames:
                    self.obj.Proxy.addbaseobject(self.obj, s.Object, i)
            else:
                self.obj.Proxy.addbaseobject(self.obj, s.Object)

        self.setupUi()  # defaults may have changed.  Reload.
        self.form.baseList.clear()
        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name + "." + i[1])

    def deleteBase(self):
        dlist = self.form.baseList.selectedItems()
        for d in dlist:
            newlist = []
            for i in self.obj.Base:
                if not i[0].Name == d.text():
                    newlist.append(i)
            self.obj.Base = newlist
        self.form.baseList.takeItem(self.form.baseList.row(d))
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def itemActivated(self):
        FreeCADGui.Selection.clearSelection()
        slist = self.form.baseList.selectedItems()
        for i in slist:
            o = FreeCAD.ActiveDocument.getObject(i.text())
            FreeCADGui.Selection.addSelection(o)
        FreeCADGui.updateGui()

    def reorderBase(self):
        newlist = []
        for i in range(self.form.baseList.count()):
            s = self.form.baseList.item(i).text()
            obj = FreeCAD.ActiveDocument.getObject(s)
            newlist.append(obj)
        self.obj.Base = newlist
        self.obj.Proxy.execute(self.obj)
        FreeCAD.ActiveDocument.recompute()

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def changeURL(self):
        from urlparse import urlparse
        t = self.form.remoteURL.text()
        if t == '' and self.obj.URL != '':  # if the url was deleted, cleanup.
            self.obj.URL = ''

        if urlparse(t).scheme != '' and t != self.obj.URL:  # validate new url.
            self.obj.URL = t
        # next make sure the property fields reflect the current attached service
        for p in self.obj.proplist:
            print p

    def setupUi(self):
        self.form.startDepth.setText(str(self.obj.StartDepth))
        self.form.finalDepth.setText(str(self.obj.FinalDepth))
        self.form.safeHeight.setText(str(self.obj.SafeHeight))
        self.form.clearanceHeight.setText(str(self.obj.ClearanceHeight))
        self.form.remoteURL.setText(str(self.obj.URL))

        for i in self.obj.Base:
            self.form.baseList.addItem(i[0].Name)

        # Connect Signals and Slots
        self.form.startDepth.editingFinished.connect(self.getFields)
        self.form.finalDepth.editingFinished.connect(self.getFields)
        self.form.safeHeight.editingFinished.connect(self.getFields)
        self.form.clearanceHeight.editingFinished.connect(self.getFields)

        self.form.addBase.clicked.connect(self.addBase)
        self.form.baseList.itemSelectionChanged.connect(self.itemActivated)
        self.form.deleteBase.clicked.connect(self.deleteBase)
        self.form.reorderBase.clicked.connect(self.reorderBase)

        self.form.remoteURL.editingFinished.connect(self.changeURL)


class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Remote', CommandPathRemote())
    FreeCADGui.addCommand('Refresh_Path', _RefreshRemotePath())

FreeCAD.Console.PrintLog("Loading PathRemote... done\n")
