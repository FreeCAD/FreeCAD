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
import PathScripts.PathStock as PathStock
import PathScripts.PathToolController as PathToolController
import PathScripts.PathUtil as PathUtil
import xml.etree.ElementTree as xml

from PathScripts.PathPreferences import PathPreferences
from PathScripts.PathPostProcessor import PostProcessor
from PySide import QtCore

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())

"""Path Job object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class JobTemplate:
    '''Attribute and sub element strings for template export/import.'''
    Job = 'Job'
    PostProcessor = 'post'
    PostProcessorArgs = 'post_args'
    PostProcessorOutputFile = 'output'
    GeometryTolerance = 'tol'
    Description = 'desc'
    ToolController = 'ToolController'

def isResourceClone(obj, propName, resourceName):
    '''isResourceClone(obj, propName, resourceName) ... Return True if the given property of obj is a clone of type resourceName.'''
    if hasattr(obj, propName):
        propLink =  getattr(obj, propName)
        if hasattr(propLink, 'PathResource') and resourceName == propLink.PathResource:
            return True
    return False

def createResourceClone(obj, orig, name, icon):
    clone = Draft.clone(orig)
    clone.Label = "%s-%s" % (name, orig.Label)
    clone.addProperty('App::PropertyString', 'PathResource')
    clone.PathResource = name
    if clone.ViewObject:
        PathIconViewProvider.ViewProvider(clone.ViewObject, icon)
        clone.ViewObject.Visibility = False
    return clone

class ObjectJob:

    def __init__(self, obj, base, template = None):
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

        obj.Base = createResourceClone(obj, base, 'Base', 'BaseGeometry')
        obj.Proxy = self

        self.assignTemplate(obj, template)
        if not obj.Stock:
            obj.Stock = PathStock.CreateFromBase(obj)
        if obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = False

    def onDelete(self, obj, arg2=None):
        '''Called by the view provider, there doesn't seem to be a callback on the obj itself.'''
        PathLog.track(obj.Label, arg2)
        doc = obj.Document
        for tc in obj.ToolController:
            doc.removeObject(tc.Name)
        obj.ToolController = []
        while obj.Operations.Group:
            doc.removeObject(obj.Operations.Group[0].Name)
        obj.Operations.Group = []
        doc.removeObject(obj.Operations.Name)
        obj.Operations = None
        if obj.Base:
            doc.removeObject(obj.Base.Name)
            obj.Base = None
        if obj.Stock:
            doc.removeObject(obj.Stock.Name)
            obj.Stock = None

    def fixupResourceClone(self, obj, name, icon):
        if not isResourceClone(obj, name, name):
            orig = getattr(obj, name)
            if orig:
                setattr(obj, name, createResourceClone(obj, orig, name, icon))

    def onDocumentRestored(self, obj):
        self.fixupResourceClone(obj, 'Base', 'BaseGeometry')

    def baseObject(self, obj):
        '''Return the base object, not its clone.'''
        if isResourceClone(obj, 'Base', 'Base'):
            return obj.Base.Objects[0]
        return obj.Base

    def assignTemplate(self, obj, template):
        '''assignTemplate(obj, template) ... extract the properties from the given template file and assign to receiver.
        This will also create any TCs stored in the template.'''
        tcs = []
        if template:
            tree = xml.parse(template)
            for job in tree.getroot().iter(JobTemplate.Job):
                if job.get(JobTemplate.GeometryTolerance):
                    obj.GeometryTolerance = float(job.get(JobTemplate.GeometryTolerance))
                if job.get(JobTemplate.PostProcessor):
                    obj.PostProcessor = job.get(JobTemplate.PostProcessor)
                    if job.get(JobTemplate.PostProcessorArgs):
                        obj.PostProcessorArgs = job.get(JobTemplate.PostProcessorArgs)
                    else:
                        obj.PostProcessorArgs = ''
                if job.get(JobTemplate.PostProcessorOutputFile):
                    obj.PostProcessorOutputFile = job.get(JobTemplate.PostProcessorOutputFile)
                if job.get(JobTemplate.Description):
                    obj.Description = job.get(JobTemplate.Description)
            for tc in tree.getroot().iter(JobTemplate.ToolController):
                tcs.append(PathToolController.FromTemplate(tc))
        else:
            tcs.append(PathToolController.Create(obj.Name))
        PathLog.debug("setting tool controllers (%d)" % len(tcs))
        obj.ToolController = tcs

    def templateAttrs(self, obj):
        '''templateAttrs(obj) ... answer a dictionary with all properties of the receiver that should be stored in a template file.'''
        attrs = {}
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
        PathLog.info("addToolController(%s): %s" % (tc.Label, [t.Label for t in group]))
        if tc.Name not in [str(t.Name) for t in group]:
            group.append(tc)
            self.obj.ToolController = group


    @classmethod
    def baseCandidates(cls):
        '''Answer all objects in the current document which could serve as a Base for a job.'''
        return sorted(filter(lambda obj: cls.isBaseCandidate(obj) , FreeCAD.ActiveDocument.Objects), key=lambda o: o.Label)

    @classmethod
    def isBaseCandidate(cls, obj):
        '''Answer true if the given object can be used as a Base for a job.'''
        return PathUtil.isValidBaseObject(obj) or (hasattr(obj, 'Proxy') and isinstance(obj.Proxy, ArchPanel.PanelSheet))

def Create(name, base, template = None):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    proxy = ObjectJob(obj, base, template)
    return obj

