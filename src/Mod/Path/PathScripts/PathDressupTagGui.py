# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts
import PathScripts.PathDressupTag as PathDressupTag
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils

from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore
from pivy import coin

PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
PathLog.trackModule()

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class HoldingTagMarker:
    def __init__(self, point, colors):
        self.point = point
        self.color = colors
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (point.x, point.y, point.z)
        self.sphere = coin.SoSphere()
        self.scale = coin.SoType.fromName('SoShapeScale').createInstance()
        self.scale.setPart('shape', self.sphere)
        self.scale.scaleFactor.setValue(7)
        self.material = coin.SoMaterial()
        self.sep.addChild(self.pos)
        self.sep.addChild(self.material)
        self.sep.addChild(self.scale)
        self.enabled = True
        self.selected = False

    def setSelected(self, select):
        self.selected = select
        self.sphere.radius = 1.5 if select else 1.0
        self.setEnabled(self.enabled)

    def setEnabled(self, enabled):
        self.enabled = enabled
        if enabled:
            self.material.diffuseColor = self.color[0] if not self.selected else self.color[2]
            self.material.transparency = 0.0
        else:
            self.material.diffuseColor = self.color[1] if not self.selected else self.color[2]
            self.material.transparency = 0.6

class PathDressupTagViewProvider:

    def __init__(self, vobj):
        PathLog.track()
        vobj.Proxy = self
        self.vobj = vobj
        self.panel = None

    def __getstate__(self):
        return None
    def __setstate__(self, state):
        return None

    def setupColors(self):
        def colorForColorValue(val):
            v = [((val >> n) & 0xff) / 255. for n in [24, 16, 8, 0]]
            return coin.SbColor(v[0], v[1], v[2])

        pref = PathPreferences.preferences()
        #                                                      R         G          B          A
        npc = pref.GetUnsigned('DefaultPathMarkerColor',    (( 85*256 + 255)*256 +   0)*256 + 255)
        hpc = pref.GetUnsigned('DefaultHighlightPathColor', ((255*256 + 125)*256 +   0)*256 + 255)
        dpc = pref.GetUnsigned('DefaultDisabledPathColor',  ((205*256 + 205)*256 + 205)*256 + 154)
        self.colors = [colorForColorValue(npc), colorForColorValue(dpc), colorForColorValue(hpc)]

    def attach(self, vobj):
        PathLog.track()
        self.setupColors()
        self.obj = vobj.Object
        self.tags = []
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)
        self.turnMarkerDisplayOn(False)

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, 'Group') and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                PathLog.info("Setting visibility for %s" % (self.obj.Base.Name))
                self.obj.Base.ViewObject.Visibility = False
            else:
                PathLog.info("Ignoring visibility")
            if PathLog.getLevel(PathLog.thisModule()) != PathLog.Level.DEBUG and self.obj.Debug.ViewObject:
                self.obj.Debug.ViewObject.Visibility = False

        self.obj.Proxy.changed.connect(self.onModelChanged)

    def turnMarkerDisplayOn(self, display):
        sw = coin.SO_SWITCH_ALL if display else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def claimChildren(self):
        PathLog.track()
        return [self.obj.Base, self.obj.Debug]

    def onDelete(self, arg1=None, arg2=None):
        PathLog.track()
        '''this makes sure that the base operation is added back to the project and visible'''
        if self.obj.Base.ViewObject:
            self.obj.Base.ViewObject.Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        self.obj.Debug.removeObjectsFromDocument()
        self.obj.Debug.Document.removeObject(self.obj.Debug.Name)
        self.obj.Debug = None
        return True

    def updateData(self, obj, propName):
        PathLog.track(propName)
        if 'Disabled' == propName:
            for tag in self.tags:
                self.switch.removeChild(tag.sep)
            tags = []
            for i, p in enumerate(obj.Positions):
                tag = HoldingTagMarker(p, self.colors)
                tag.setEnabled(not i in obj.Disabled)
                tags.append(tag)
                self.switch.addChild(tag.sep)
            self.tags = tags

    def onModelChanged(self):
        PathLog.track()
        self.obj.Debug.removeObjectsFromDocument()
        for solid in self.obj.Proxy.solids:
            tag = self.obj.Document.addObject('Part::Feature', 'tag')
            tag.Shape = solid
            if tag.ViewObject and self.obj.Debug.ViewObject:
                tag.ViewObject.Visibility = self.obj.Debug.ViewObject.Visibility
                tag.ViewObject.Transparency = 80
            self.obj.Debug.addObject(tag)
            tag.purgeTouched()

    def selectTag(self, index):
        PathLog.track(index)
        for i, tag in enumerate(self.tags):
            tag.setSelected(i == index)

    def tagAtPoint(self, point):
        p = FreeCAD.Vector(point[0], point[1], point[2])
        for i, tag in enumerate(self.tags):
            if PathGeom.pointsCoincide(p, tag.point, tag.sphere.radius.getValue() * 1.1):
                return i
        return -1

    # SelectionObserver interface
    def allow(self, doc, obj, sub):
        if obj == self.obj:
            return True
        return False

    def addSelection(self, doc, obj, sub, point):
        i = self.tagAtPoint(point)
        if self.panel:
            self.panel.selectTagWithId(i)
        FreeCADGui.updateGui()

def Create(baseObject, name='DressupTag'):
    '''
    Create(basePath, name = 'DressupTag') ... create tag dressup object for the given base path.
    Use this command only iff the UI is up - for batch processing see PathDressupTag.Create
    '''
    obj = PathDressupTag.Create(baseObject, name)
    vp = PathDressupTagViewProvider(obj.ViewObject)
    return obj

class CommandPathDressupTag:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP('PathDressup_Tag', 'Tag Dress-up'),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP('PathDressup_Tag', 'Creates a Tag Dress-up object from a selected path')}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == 'Job':
                        return True
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate('PathDressup_Tag', 'Please select one path object\n'))
            return
        baseObject = selection[0]

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate('PathDressup_Tag', 'Create Tag Dress-up'))
        FreeCADGui.addModule('PathScripts.PathDressupTagGui')
        FreeCADGui.doCommand("PathScripts.PathDressupTagGui.Create(App.ActiveDocument.%s)" % baseObject.Name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_Tag', CommandPathDressupTag())

PathLog.notice('Loading PathDressupTagGui... done\n')
