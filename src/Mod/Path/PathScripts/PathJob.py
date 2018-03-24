# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import ArchPanel
import Draft
import FreeCAD
import PathScripts.PathIconViewProvider as PathIconViewProvider
import PathScripts.PathLog as PathLog
import PathScripts.PathSetupSheet as PathSetupSheet
import PathScripts.PathStock as PathStock
import PathScripts.PathToolController as PathToolController
import PathScripts.PathUtil as PathUtil
import json

from PathScripts.PathPreferences import PathPreferences
from PathScripts.PathPostProcessor import PostProcessor
from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

"""Path Job object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class JobTemplate:
    '''Attribute and sub element strings for template export/import.'''
    Description = 'Desc'
    GeometryTolerance = 'Tolerance'
    Job = 'Job'
    PostProcessor = 'Post'
    PostProcessorArgs = 'PostArgs'
    PostProcessorOutputFile = 'Output'
    SetupSheet = 'SetupSheet'
    Stock = 'Stock'
    ToolController = 'ToolController'
    Version = 'Version'

def isArchPanelSheet(obj):
    return hasattr(obj, 'Proxy') and isinstance(obj.Proxy, ArchPanel.PanelSheet)

def isResourceClone(obj, propName, resourceName=None):
    '''isResourceClone(obj, propName, resourceName) ... Return True if the given property of obj is a clone of type resourceName.'''
    if hasattr(obj, propName):
        propLink =  getattr(obj, propName)
        if hasattr(propLink, 'PathResource') and ((resourceName and resourceName == propLink.PathResource) or (resourceName is None and propName == propLink.PathResource)):
            return True
    return False

def createResourceClone(obj, orig, name, icon):
    if isArchPanelSheet(orig):
        # can't clone panel sheets - they have to be panel sheets
        return orig

    clone = Draft.clone(orig)
    clone.Label = "%s-%s" % (name, orig.Label)
    clone.addProperty('App::PropertyString', 'PathResource')
    clone.PathResource = name
    if clone.ViewObject:
        PathIconViewProvider.ViewProvider(clone.ViewObject, icon)
        clone.ViewObject.Visibility = False
    obj.Document.recompute() # necessary to create the clone shape
    return clone

class ObjectJob:

    def __init__(self, obj, base, templateFile = None):
        self.obj = obj
        obj.addProperty("App::PropertyFile", "PostProcessorOutputFile", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","The NC output file for this project"))
        obj.addProperty("App::PropertyEnumeration", "PostProcessor", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property","Select the Post Processor"))
        obj.addProperty("App::PropertyString", "PostProcessorArgs", "Output", QtCore.QT_TRANSLATE_NOOP("App::Property", "Arguments for the Post Processor (specific to the script)"))

        obj.addProperty("App::PropertyString", "Description", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","An optional description for this job"))
        obj.addProperty("App::PropertyDistance", "GeometryTolerance", "Geometry", QtCore.QT_TRANSLATE_NOOP("App::Property", "For computing Paths; smaller increases accuracy, but slows down computation"))

        obj.addProperty("App::PropertyLink", "Base", "Base", QtCore.QT_TRANSLATE_NOOP("PathJob", "The base object for all operations"))
        obj.addProperty("App::PropertyLink", "Stock", "Base", QtCore.QT_TRANSLATE_NOOP("PathJob", "Solid object to be used as stock."))
        obj.addProperty("App::PropertyLink", "Operations", "Base", QtCore.QT_TRANSLATE_NOOP("PathJob", "Compound path of all operations in the order they are processed."))
        obj.addProperty("App::PropertyLinkList", "ToolController", "Base", QtCore.QT_TRANSLATE_NOOP("PathJob", "Collection of tool controllers available for this job."))

        obj.PostProcessorOutputFile = PathPreferences.defaultOutputFile()
        #obj.setEditorMode("PostProcessorOutputFile", 0)  # set to default mode
        obj.PostProcessor = postProcessors = PathPreferences.allEnabledPostProcessors()
        defaultPostProcessor = PathPreferences.defaultPostProcessor()
        # Check to see if default post processor hasn't been 'lost' (This can happen when Macro dir has changed)
        if defaultPostProcessor in postProcessors:
            obj.PostProcessor = defaultPostProcessor
        else:
            obj.PostProcessor = postProcessors[0]
        obj.PostProcessorArgs = PathPreferences.defaultPostProcessorArgs()
        obj.GeometryTolerance = PathPreferences.defaultGeometryTolerance()

        ops = FreeCAD.ActiveDocument.addObject("Path::FeatureCompoundPython", "Operations")
        obj.Operations = ops
        obj.setEditorMode('Operations', 2) # hide
        obj.setEditorMode('Placement', 2)

        self.setupSetupSheet(obj)

        obj.Base = createResourceClone(obj, base, 'Base', 'BaseGeometry')
        obj.Proxy = self

        self.setFromTemplateFile(obj, templateFile)
        if not obj.Stock:
            stockTemplate = PathPreferences.defaultStockTemplate()
            if stockTemplate:
                obj.Stock = PathStock.CreateFromTemplate(obj, json.loads(stockTemplate))
            if not obj.Stock:
                obj.Stock = PathStock.CreateFromBase(obj)
        if obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = False

    def setupSetupSheet(self, obj):
        if not hasattr(obj, 'SetupSheet'):
            obj.addProperty('App::PropertyLink', 'SetupSheet', 'Base', QtCore.QT_TRANSLATE_NOOP('PathJob', 'SetupSheet holding the settings for this job'))
            obj.SetupSheet = PathSetupSheet.Create()
            if obj.SetupSheet.ViewObject:
                PathIconViewProvider.ViewProvider(obj.SetupSheet.ViewObject, 'SetupSheet')
        self.setupSheet = obj.SetupSheet.Proxy

    def onDelete(self, obj, arg2=None):
        '''Called by the view provider, there doesn't seem to be a callback on the obj itself.'''
        PathLog.track(obj.Label, arg2)
        doc = obj.Document
        # the first to tear down are the ops, they depend on other resources
        PathLog.debug('taking down ops: %s' % [o.Name for o in self.allOperations()])
        while obj.Operations.Group:
            op = obj.Operations.Group[0]
            if not op.ViewObject or not hasattr(op.ViewObject.Proxy, 'onDelete') or op.ViewObject.Proxy.onDelete(op.ViewObject, ()):
                PathUtil.clearExpressionEngine(op)
                doc.removeObject(op.Name)
        obj.Operations.Group = []
        doc.removeObject(obj.Operations.Name)
        obj.Operations = None
        # stock could depend on Base
        if obj.Stock:
            PathLog.debug('taking down stock')
            PathUtil.clearExpressionEngine(obj.Stock)
            doc.removeObject(obj.Stock.Name)
            obj.Stock = None
        # base doesn't depend on anything inside job
        if obj.Base:
            PathLog.debug('taking down base')
            if isResourceClone(obj, 'Base'):
                PathUtil.clearExpressionEngine(obj.Base)
                doc.removeObject(obj.Base.Name)
            obj.Base = None
        # Tool controllers don't depend on anything
        PathLog.debug('taking down tool controller')
        for tc in obj.ToolController:
            PathUtil.clearExpressionEngine(tc)
            doc.removeObject(tc.Name)
        obj.ToolController = []
        # SetupSheet
        PathUtil.clearExpressionEngine(obj.SetupSheet)
        doc.removeObject(obj.SetupSheet.Name)
        obj.SetupSheet = None
        return True

    def fixupResourceClone(self, obj, name, icon):
        if not isResourceClone(obj, name, name) and not isArchPanelSheet(obj):
            orig = getattr(obj, name)
            if orig:
                setattr(obj, name, createResourceClone(obj, orig, name, icon))

    def onDocumentRestored(self, obj):
        self.fixupResourceClone(obj, 'Base', 'BaseGeometry')
        self.setupSetupSheet(obj)

    def onChanged(self, obj, prop):
        if prop == "PostProcessor" and obj.PostProcessor:
            processor = PostProcessor.load(obj.PostProcessor)
            self.tooltip = processor.tooltip
            self.tooltipArgs = processor.tooltipArgs

    def baseObject(self, obj):
        '''Return the base object, not its clone.'''
        if isResourceClone(obj, 'Base', 'Base'):
            return obj.Base.Objects[0]
        return obj.Base

    def setFromTemplateFile(self, obj, template):
        '''setFromTemplateFile(obj, template) ... extract the properties from the given template file and assign to receiver.
        This will also create any TCs stored in the template.'''
        tcs = []
        if template:
            with open(PathUtil.toUnicode(template), 'rb') as fp:
                attrs = json.load(fp)

            if attrs.get(JobTemplate.Version) and 1 == int(attrs[JobTemplate.Version]):
                attrs = self.setupSheet.decodeTemplateAttributes(attrs)
                if attrs.get(JobTemplate.SetupSheet):
                    self.setupSheet.setFromTemplate(attrs[JobTemplate.SetupSheet])

                if attrs.get(JobTemplate.GeometryTolerance):
                    obj.GeometryTolerance = float(attrs.get(JobTemplate.GeometryTolerance))
                if attrs.get(JobTemplate.PostProcessor):
                    obj.PostProcessor = attrs.get(JobTemplate.PostProcessor)
                    if attrs.get(JobTemplate.PostProcessorArgs):
                        obj.PostProcessorArgs = attrs.get(JobTemplate.PostProcessorArgs)
                    else:
                        obj.PostProcessorArgs = ''
                if attrs.get(JobTemplate.PostProcessorOutputFile):
                    obj.PostProcessorOutputFile = attrs.get(JobTemplate.PostProcessorOutputFile)
                if attrs.get(JobTemplate.Description):
                    obj.Description = attrs.get(JobTemplate.Description)

                if attrs.get(JobTemplate.ToolController):
                    for tc in attrs.get(JobTemplate.ToolController):
                        tcs.append(PathToolController.FromTemplate(tc))
                if attrs.get(JobTemplate.Stock):
                    obj.Stock = PathStock.CreateFromTemplate(obj, attrs.get(JobTemplate.Stock))

                PathLog.debug("setting tool controllers (%d)" % len(tcs))
                obj.ToolController = tcs
            else:
                PathLog.error(translate('PathJob', "Unsupported PathJob template version %s") % attrs.get(JobTemplate.Version))
        if not tcs:
            self.addToolController(PathToolController.Create())

    def templateAttrs(self, obj):
        '''templateAttrs(obj) ... answer a dictionary with all properties of the receiver that should be stored in a template file.'''
        attrs = {}
        attrs[JobTemplate.Version] = 1
        if obj.PostProcessor:
            attrs[JobTemplate.PostProcessor]           = obj.PostProcessor
            attrs[JobTemplate.PostProcessorArgs]       = obj.PostProcessorArgs
        if obj.PostProcessorOutputFile:
            attrs[JobTemplate.PostProcessorOutputFile] = obj.PostProcessorOutputFile
        attrs[JobTemplate.GeometryTolerance]           = str(obj.GeometryTolerance.Value)
        if obj.Description:
            attrs[JobTemplate.Description]             = obj.Description
        return attrs

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, 'Proxy') and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def execute(self, obj):
        obj.Path = obj.Operations.Path

    def addOperation(self, op):
        group = self.obj.Operations.Group
        if op not in group:
            group.append(op)
            self.obj.Operations.Group = group

    def addToolController(self, tc):
        group = self.obj.ToolController
        PathLog.debug("addToolController(%s): %s" % (tc.Label, [t.Label for t in group]))
        if tc.Name not in [str(t.Name) for t in group]:
            tc.setExpression('VertRapid',  "%s.%s" % (self.setupSheet.expressionReference(), PathSetupSheet.Template.VertRapid))
            tc.setExpression('HorizRapid', "%s.%s" % (self.setupSheet.expressionReference(), PathSetupSheet.Template.HorizRapid))
            group.append(tc)
            self.obj.ToolController = group

    def allOperations(self):
        ops = []
        def collectBaseOps(op):
            if hasattr(op, 'TypeId'):
                if op.TypeId == 'Path::FeaturePython':
                    ops.append(op)
                    if hasattr(op, 'Base'):
                        collectBaseOps(op.Base)
                if op.TypeId == 'Path::FeatureCompoundPython':
                    ops.append(op)
                    for sub in op.Group:
                        collectBaseOps(sub)
        for op in self.obj.Operations.Group:
            collectBaseOps(op)
        return ops

    @classmethod
    def baseCandidates(cls):
        '''Answer all objects in the current document which could serve as a Base for a job.'''
        return sorted(filter(lambda obj: cls.isBaseCandidate(obj) , FreeCAD.ActiveDocument.Objects), key=lambda o: o.Label)

    @classmethod
    def isBaseCandidate(cls, obj):
        '''Answer true if the given object can be used as a Base for a job.'''
        return PathUtil.isValidBaseObject(obj) or isArchPanelSheet(obj)

def Instances():
    '''Instances() ... Return all Jobs in the current active document.'''
    if FreeCAD.ActiveDocument:
        return [job for job in FreeCAD.ActiveDocument.Objects if hasattr(job, 'Proxy') and isinstance(job.Proxy, ObjectJob)]
    return []

def Create(name, base, templateFile = None):
    '''Create(name, base, templateFile=None) ... creates a new job and all it's resources.
    If a template file is specified the new job is initialized with the values from the template.'''
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectJob(obj, base, templateFile)
    return obj

