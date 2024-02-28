# -*- coding: utf-8 -*-
# ***************************************************************************
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

from Path.Post.Processor import PostProcessor
from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Base.SetupSheet as PathSetupSheet
import Path.Base.Util as PathUtil
import Path.Main.Stock as PathStock
import Path.Tool.Controller as PathToolController
import json
import time


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class JobTemplate:
    """Attribute and sub element strings for template export/import."""

    Description = "Desc"
    GeometryTolerance = "Tolerance"
    Job = "Job"
    PostProcessor = "Post"
    PostProcessorArgs = "PostArgs"
    PostProcessorOutputFile = "Output"
    Fixtures = "Fixtures"
    OrderOutputBy = "OrderOutputBy"
    SplitOutput = "SplitOutput"
    SetupSheet = "SetupSheet"
    Stock = "Stock"
    # TCs are grouped under Tools in a job, the template refers to them directly though
    ToolController = "ToolController"
    Version = "Version"


def isResourceClone(obj, propLink, resourceName):
    if hasattr(propLink, "PathResource") and (
        resourceName is None or resourceName == propLink.PathResource
    ):
        return True
    return False


def createResourceClone(obj, orig, name, icon):
    clone = Draft.clone(orig)
    clone.Label = "%s-%s" % (name, orig.Label)
    clone.addProperty("App::PropertyString", "PathResource")
    clone.PathResource = name
    if clone.ViewObject:
        import Path.Base.Gui.IconViewProvider

        Path.Base.Gui.IconViewProvider.Attach(clone.ViewObject, icon)
        clone.ViewObject.Visibility = False
        clone.ViewObject.Transparency = 80
    obj.Document.recompute()  # necessary to create the clone shape
    return clone


def createModelResourceClone(obj, orig):
    return createResourceClone(obj, orig, "Model", "BaseGeometry")


class NotificationClass(QtCore.QObject):
    updateTC = QtCore.Signal(object, object)


Notification = NotificationClass()


class ObjectJob:
    def __init__(self, obj, models, templateFile=None):
        self.obj = obj
        self.tooltip = None
        self.tooltipArgs = None
        obj.Proxy = self

        obj.addProperty(
            "App::PropertyFile",
            "PostProcessorOutputFile",
            "Output",
            QT_TRANSLATE_NOOP("App::Property", "The G-code output file for this project"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "PostProcessor",
            "Output",
            QT_TRANSLATE_NOOP("App::Property", "Select the Post Processor"),
        )
        obj.addProperty(
            "App::PropertyString",
            "PostProcessorArgs",
            "Output",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Arguments for the Post Processor (specific to the script)",
            ),
        )
        obj.addProperty(
            "App::PropertyString",
            "LastPostProcessDate",
            "Output",
            QT_TRANSLATE_NOOP("App::Property", "Last Time the Job was post processed"),
        )
        obj.setEditorMode("LastPostProcessDate", 2)  # Hide
        obj.addProperty(
            "App::PropertyString",
            "LastPostProcessOutput",
            "Output",
            QT_TRANSLATE_NOOP("App::Property", "Last Time the Job was post processed"),
        )
        obj.setEditorMode("LastPostProcessOutput", 2)  # Hide

        obj.addProperty(
            "App::PropertyString",
            "Description",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "An optional description for this job"),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Job Cycle Time Estimation"),
        )
        obj.setEditorMode("CycleTime", 1)  # read-only
        obj.addProperty(
            "App::PropertyDistance",
            "GeometryTolerance",
            "Geometry",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "For computing Paths; smaller increases accuracy, but slows down computation",
            ),
        )

        obj.addProperty(
            "App::PropertyLink",
            "Stock",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Solid object to be used as stock."),
        )
        obj.addProperty(
            "App::PropertyLink",
            "Operations",
            "Base",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Compound path of all operations in the order they are processed.",
            ),
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "JobType",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "Select the Type of Job"),
        )
        obj.setEditorMode("JobType", 2)  # Hide

        obj.addProperty(
            "App::PropertyBool",
            "SplitOutput",
            "Output",
            QT_TRANSLATE_NOOP(
                "App::Property", "Split output into multiple G-code files"
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "OrderOutputBy",
            "WCS",
            QT_TRANSLATE_NOOP(
                "App::Property", "If multiple WCS, order the output this way"
            ),
        )
        obj.addProperty(
            "App::PropertyStringList",
            "Fixtures",
            "WCS",
            QT_TRANSLATE_NOOP(
                "App::Property", "The Work Coordinate Systems for the Job"
            ),
        )

        obj.Fixtures = ["G54"]

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        obj.PostProcessorOutputFile = Path.Preferences.defaultOutputFile()
        obj.PostProcessor = postProcessors = Path.Preferences.allEnabledPostProcessors()
        defaultPostProcessor = Path.Preferences.defaultPostProcessor()
        # Check to see if default post processor hasn't been 'lost' (This can happen when Macro dir has changed)
        if defaultPostProcessor in postProcessors:
            obj.PostProcessor = defaultPostProcessor
        else:
            obj.PostProcessor = postProcessors[0]
        obj.PostProcessorArgs = Path.Preferences.defaultPostProcessorArgs()
        obj.GeometryTolerance = Path.Preferences.defaultGeometryTolerance()

        self.setupOperations(obj)
        self.setupSetupSheet(obj)
        self.setupBaseModel(obj, models)
        self.setupToolTable(obj)
        self.setFromTemplateFile(obj, templateFile)
        self.setupStock(obj)

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """propertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "OrderOutputBy": [
                (translate("CAM_Job", "Fixture"), "Fixture"),
                (translate("CAM_Job", "Tool"), "Tool"),
                (translate("CAM_Job", "Operation"), "Operation"),
            ],
            "JobType": [
                (translate("CAM_Job", "2D"), "2D"),
                (translate("CAM_Job", "2.5D"), "2.5D"),
                (translate("CAM_Job", "Lathe"), "Lathe"),
                (translate("CAM_Job", "Multiaxis"), "Multiaxis"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def setupOperations(self, obj):
        """setupOperations(obj)... setup the Operations group for the Job object."""
        # ops = FreeCAD.ActiveDocument.addObject(
        #     "Path::FeatureCompoundPython", "Operations"
        # )
        ops = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", "Operations")
        if ops.ViewObject:
            # ops.ViewObject.Proxy = 0
            ops.ViewObject.Visibility = True

        obj.Operations = ops
        obj.setEditorMode("Operations", 2)  # hide
        obj.setEditorMode("Placement", 2)

    def setupSetupSheet(self, obj):
        if not getattr(obj, "SetupSheet", None):
            if not hasattr(obj, "SetupSheet"):
                obj.addProperty(
                    "App::PropertyLink",
                    "SetupSheet",
                    "Base",
                    QT_TRANSLATE_NOOP(
                        "App::Property", "SetupSheet holding the settings for this job"
                    ),
                )
            obj.SetupSheet = PathSetupSheet.Create()
            if obj.SetupSheet.ViewObject:
                import Path.Base.Gui.IconViewProvider

                Path.Base.Gui.IconViewProvider.Attach(
                    obj.SetupSheet.ViewObject, "SetupSheet"
                )
            obj.SetupSheet.Label = "SetupSheet"
        self.setupSheet = obj.SetupSheet.Proxy

    def setupBaseModel(self, obj, models=None):
        Path.Log.track(obj.Label, models)
        addModels = False

        if not hasattr(obj, "Model"):
            obj.addProperty(
                "App::PropertyLink",
                "Model",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The base objects for all operations"
                ),
            )
            addModels = True
        elif obj.Model is None:
            addModels = True

        if addModels:
            model = FreeCAD.ActiveDocument.addObject(
                "App::DocumentObjectGroup", "Model"
            )
            if model.ViewObject:
                model.ViewObject.Visibility = False
            if models:
                model.addObjects(
                    [createModelResourceClone(obj, base) for base in models]
                )
            obj.Model = model
            obj.Model.Label = "Model"

        if hasattr(obj, "Base"):
            Path.Log.info(
                "Converting Job.Base to new Job.Model for {}".format(obj.Label)
            )
            obj.Model.addObject(obj.Base)
            obj.Base = None
            obj.removeProperty("Base")

    def setupToolTable(self, obj):
        addTable = False
        if not hasattr(obj, "Tools"):
            obj.addProperty(
                "App::PropertyLink",
                "Tools",
                "Base",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Collection of all tool controllers for the job"
                ),
            )
            addTable = True
        elif obj.Tools is None:
            addTable = True

        if addTable:
            toolTable = FreeCAD.ActiveDocument.addObject(
                "App::DocumentObjectGroup", "Tools"
            )
            toolTable.Label = "Tools"
            if toolTable.ViewObject:
                toolTable.ViewObject.Visibility = False
            if hasattr(obj, "ToolController"):
                toolTable.addObjects(obj.ToolController)
                obj.removeProperty("ToolController")
            obj.Tools = toolTable

    def setupStock(self, obj):
        """setupStock(obj)... setup the Stock for the Job object."""
        if not obj.Stock:
            stockTemplate = Path.Preferences.defaultStockTemplate()
            if stockTemplate:
                obj.Stock = PathStock.CreateFromTemplate(obj, json.loads(stockTemplate))
            if not obj.Stock:
                obj.Stock = PathStock.CreateFromBase(obj)
        if obj.Stock.ViewObject:
            obj.Stock.ViewObject.Visibility = False

    def removeBase(self, obj, base, removeFromModel):
        if isResourceClone(obj, base, None):
            PathUtil.clearExpressionEngine(base)
            if removeFromModel:
                obj.Model.removeObject(base)
            obj.Document.removeObject(base.Name)

    def modelBoundBox(self, obj):
        return PathStock.shapeBoundBox(obj.Model.Group)

    def onDelete(self, obj, arg2=None):
        """Called by the view provider, there doesn't seem to be a callback on the obj itself."""
        Path.Log.track(obj.Label, arg2)
        doc = obj.Document

        if getattr(obj, "Operations", None):
            # the first to tear down are the ops, they depend on other resources
            Path.Log.debug(
                "taking down ops: %s" % [o.Name for o in self.allOperations()]
            )
            while obj.Operations.Group:
                op = obj.Operations.Group[0]
                if (
                    not op.ViewObject
                    or not hasattr(op.ViewObject.Proxy, "onDelete")
                    or op.ViewObject.Proxy.onDelete(op.ViewObject, ())
                ):
                    PathUtil.clearExpressionEngine(op)
                    doc.removeObject(op.Name)
            obj.Operations.Group = []
            doc.removeObject(obj.Operations.Name)
            obj.Operations = None

        # stock could depend on Model, so delete it first
        if getattr(obj, "Stock", None):
            Path.Log.debug("taking down stock")
            PathUtil.clearExpressionEngine(obj.Stock)
            doc.removeObject(obj.Stock.Name)
            obj.Stock = None

        # base doesn't depend on anything inside job
        if getattr(obj, "Model", None):
            for base in obj.Model.Group:
                Path.Log.debug("taking down base %s" % base.Label)
                self.removeBase(obj, base, False)
            obj.Model.Group = []
            doc.removeObject(obj.Model.Name)
            obj.Model = None

        # Tool controllers might refer to either legacy tool or toolbit
        if getattr(obj, "Tools", None):
            Path.Log.debug("taking down tool controller")
            for tc in obj.Tools.Group:
                if hasattr(tc.Tool, "Proxy"):
                    PathUtil.clearExpressionEngine(tc.Tool)
                    doc.removeObject(tc.Tool.Name)
                PathUtil.clearExpressionEngine(tc)
                tc.Proxy.onDelete(tc)
                doc.removeObject(tc.Name)
            obj.Tools.Group = []
            doc.removeObject(obj.Tools.Name)
            obj.Tools = None

        # SetupSheet
        if getattr(obj, "SetupSheet", None):
            PathUtil.clearExpressionEngine(obj.SetupSheet)
            doc.removeObject(obj.SetupSheet.Name)
            obj.SetupSheet = None

        return True

    def fixupOperations(self, obj):
        if getattr(obj.Operations, "ViewObject", None):
            try:
                obj.Operations.ViewObject.DisplayMode
            except Exception:
                name = obj.Operations.Name
                label = obj.Operations.Label
                ops = FreeCAD.ActiveDocument.addObject(
                    "Path::FeatureCompoundPython", "Operations"
                )
                ops.ViewObject.Proxy = 0
                ops.Group = obj.Operations.Group
                obj.Operations.Group = []
                obj.Operations = ops
                FreeCAD.ActiveDocument.removeObject(name)
                if label == "Unnamed":
                    ops.Label = "Operations"
                else:
                    ops.Label = label

    def onDocumentRestored(self, obj):
        self.setupBaseModel(obj)
        self.fixupOperations(obj)
        self.setupSetupSheet(obj)
        self.setupToolTable(obj)
        self.integrityCheck(obj)

        obj.setEditorMode("Operations", 2)  # hide
        obj.setEditorMode("Placement", 2)

        if hasattr(obj, "Path"):
            obj.Path = Path.Path()

        if not hasattr(obj, "CycleTime"):
            obj.addProperty(
                "App::PropertyString",
                "CycleTime",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Operations Cycle Time Estimation"),
            )
            obj.setEditorMode("CycleTime", 1)  # read-only

        if not hasattr(obj, "Fixtures"):
            obj.addProperty(
                "App::PropertyStringList",
                "Fixtures",
                "WCS",
                QT_TRANSLATE_NOOP(
                    "App::Property", "The Work Coordinate Systems for the Job"
                ),
            )
            obj.Fixtures = ["G54"]

        if not hasattr(obj, "OrderOutputBy"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "OrderOutputBy",
                "WCS",
                QT_TRANSLATE_NOOP(
                    "App::Property", "If multiple WCS, order the output this way"
                ),
            )
            obj.OrderOutputBy = ["Fixture", "Tool", "Operation"]

        if not hasattr(obj, "SplitOutput"):
            obj.addProperty(
                "App::PropertyBool",
                "SplitOutput",
                "Output",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Split output into multiple G-code files"
                ),
            )
            obj.SplitOutput = False

        if not hasattr(obj, "JobType"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "JobType",
                "Base",
                QT_TRANSLATE_NOOP("App::Property", "Select the type of Job"),
            )
            obj.setEditorMode("JobType", 2)  # Hide

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def onChanged(self, obj, prop):
        if prop == "PostProcessor" and obj.PostProcessor:
            processor = PostProcessor.load(obj.PostProcessor)
            self.tooltip = processor.tooltip
            self.tooltipArgs = processor.tooltipArgs

    def baseObject(self, obj, base):
        """Return the base object, not its clone."""
        if isResourceClone(obj, base, "Model") or isResourceClone(obj, base, "Base"):
            return base.Objects[0]
        return base

    def baseObjects(self, obj):
        """Return the base objects, not their clones."""
        return [self.baseObject(obj, base) for base in obj.Model.Group]

    def resourceClone(self, obj, base):
        """resourceClone(obj, base) ... Return the resource clone for base if it exists."""
        if isResourceClone(obj, base, None):
            return base
        for b in obj.Model.Group:
            if base == b.Objects[0]:
                return b
        return None

    def setFromTemplateFile(self, obj, template):
        """setFromTemplateFile(obj, template) ... extract the properties from the given template file and assign to receiver.
        This will also create any TCs stored in the template."""
        tcs = []
        if template:
            with open(str(template), "rb") as fp:
                attrs = json.load(fp)

            if attrs.get(JobTemplate.Version) and 1 == int(attrs[JobTemplate.Version]):
                attrs = self.setupSheet.decodeTemplateAttributes(attrs)
                if attrs.get(JobTemplate.SetupSheet):
                    self.setupSheet.setFromTemplate(attrs[JobTemplate.SetupSheet])

                if attrs.get(JobTemplate.GeometryTolerance):
                    obj.GeometryTolerance = float(
                        attrs.get(JobTemplate.GeometryTolerance)
                    )
                if attrs.get(JobTemplate.PostProcessor):
                    obj.PostProcessor = attrs.get(JobTemplate.PostProcessor)
                    if attrs.get(JobTemplate.PostProcessorArgs):
                        obj.PostProcessorArgs = attrs.get(JobTemplate.PostProcessorArgs)
                    else:
                        obj.PostProcessorArgs = ""
                if attrs.get(JobTemplate.PostProcessorOutputFile):
                    obj.PostProcessorOutputFile = attrs.get(
                        JobTemplate.PostProcessorOutputFile
                    )
                if attrs.get(JobTemplate.Description):
                    obj.Description = attrs.get(JobTemplate.Description)

                if attrs.get(JobTemplate.ToolController):
                    for tc in attrs.get(JobTemplate.ToolController):
                        ctrl = PathToolController.FromTemplate(tc)
                        if ctrl:
                            tcs.append(ctrl)
                        else:
                            Path.Log.debug(f"skipping TC {tc['name']}")
                if attrs.get(JobTemplate.Stock):
                    obj.Stock = PathStock.CreateFromTemplate(
                        obj, attrs.get(JobTemplate.Stock)
                    )

                if attrs.get(JobTemplate.Fixtures):
                    obj.Fixtures = [
                        x for y in attrs.get(JobTemplate.Fixtures) for x in y
                    ]

                if attrs.get(JobTemplate.OrderOutputBy):
                    obj.OrderOutputBy = attrs.get(JobTemplate.OrderOutputBy)

                if attrs.get(JobTemplate.SplitOutput):
                    obj.SplitOutput = attrs.get(JobTemplate.SplitOutput)

                Path.Log.debug("setting tool controllers (%d)" % len(tcs))
                if tcs:
                    obj.Tools.Group = tcs
            else:
                Path.Log.error(
                    "Unsupported PathJob template version {}".format(
                        attrs.get(JobTemplate.Version)
                    )
                )

        if not tcs:
            self.addToolController(PathToolController.Create())

    def templateAttrs(self, obj):
        """templateAttrs(obj) ... answer a dictionary with all properties of the receiver that should be stored in a template file."""
        attrs = {}
        attrs[JobTemplate.Version] = 1
        if obj.PostProcessor:
            attrs[JobTemplate.PostProcessor] = obj.PostProcessor
            attrs[JobTemplate.PostProcessorArgs] = obj.PostProcessorArgs
            attrs[JobTemplate.Fixtures] = [{f: True} for f in obj.Fixtures]
            attrs[JobTemplate.OrderOutputBy] = obj.OrderOutputBy
            attrs[JobTemplate.SplitOutput] = obj.SplitOutput
        if obj.PostProcessorOutputFile:
            attrs[JobTemplate.PostProcessorOutputFile] = obj.PostProcessorOutputFile
        attrs[JobTemplate.GeometryTolerance] = str(obj.GeometryTolerance.Value)
        if obj.Description:
            attrs[JobTemplate.Description] = obj.Description
        return attrs

    def dumps(self):
        return None

    def loads(self, state):
        for obj in FreeCAD.ActiveDocument.Objects:
            if hasattr(obj, "Proxy") and obj.Proxy == self:
                self.obj = obj
                break
        return None

    def execute(self, obj):
        if getattr(obj, "Operations", None):
            # obj.Path = obj.Operations.Path
            self.getCycleTime()
            if hasattr(obj, "PathChanged"):
                obj.PathChanged = True

    def getCycleTime(self):
        seconds = 0

        if len(self.obj.Operations.Group):
            for op in self.obj.Operations.Group:

                # Skip inactive operations
                if PathUtil.opProperty(op, "Active") is False:
                    continue

                # Skip operations that don't have a cycletime attribute
                if PathUtil.opProperty(op, "CycleTime") is None:
                    continue

                formattedCycleTime = PathUtil.opProperty(op, "CycleTime")
                opCycleTime = 0
                try:
                    # Convert the formatted time from HH:MM:SS to just seconds
                    opCycleTime = sum(
                        x * int(t)
                        for x, t in zip(
                            [1, 60, 3600], reversed(formattedCycleTime.split(":"))
                        )
                    )
                except Exception:
                    continue

                if opCycleTime > 0:
                    seconds = seconds + opCycleTime

        cycleTimeString = time.strftime("%H:%M:%S", time.gmtime(seconds))
        self.obj.CycleTime = cycleTimeString

    def addOperation(self, op, before=None, removeBefore=False):
        group = self.obj.Operations.Group
        if op not in group:
            if before:
                try:
                    group.insert(group.index(before), op)
                    if removeBefore:
                        group.remove(before)
                except Exception as e:
                    Path.Log.error(e)
                    group.append(op)
            else:
                group.append(op)
            self.obj.Operations.Group = group
            # op.Path.Center = self.obj.Operations.Path.Center

    def nextToolNumber(self):
        # returns the next available toolnumber in the job
        group = self.obj.Tools.Group
        if len(group) > 0:
            return sorted([t.ToolNumber for t in group])[-1] + 1
        else:
            return 1

    def addToolController(self, tc):
        group = self.obj.Tools.Group
        Path.Log.debug(
            "addToolController(%s): %s" % (tc.Label, [t.Label for t in group])
        )
        if tc.Name not in [str(t.Name) for t in group]:
            tc.setExpression(
                "VertRapid",
                "%s.%s"
                % (
                    self.setupSheet.expressionReference(),
                    PathSetupSheet.Template.VertRapid,
                ),
            )
            tc.setExpression(
                "HorizRapid",
                "%s.%s"
                % (
                    self.setupSheet.expressionReference(),
                    PathSetupSheet.Template.HorizRapid,
                ),
            )
            self.obj.Tools.addObject(tc)
            Notification.updateTC.emit(self.obj, tc)

    def allOperations(self):
        ops = []

        def collectBaseOps(op):
            if hasattr(op, "TypeId"):
                if op.TypeId == "Path::FeaturePython":
                    ops.append(op)
                    if hasattr(op, "Base"):
                        collectBaseOps(op.Base)
                if op.TypeId == "Path::FeatureCompoundPython":
                    ops.append(op)
                    for sub in op.Group:
                        collectBaseOps(sub)

        if getattr(self.obj, "Operations", None) and getattr(
            self.obj.Operations, "Group", None
        ):
            for op in self.obj.Operations.Group:
                collectBaseOps(op)

        return ops

    def setCenterOfRotation(self, center):
        if center != self.obj.Path.Center:
            self.obj.Path.Center = center
            self.obj.Operations.Path.Center = center
            for op in self.allOperations():
                op.Path.Center = center

    def integrityCheck(self, job):
        """integrityCheck(job)... Return True if job has all expected children objects.  Attempts to restore any missing children."""
        suffix = ""
        if len(job.Name) > 3:
            suffix = job.Name[3:]

        def errorMessage(grp, job):
            Path.Log.error("{} corrupt in {} job.".format(grp, job.Name))

        if not job.Operations:
            self.setupOperations(job)
            job.Operations.Label = "Operations" + suffix
            if not job.Operations:
                errorMessage("Operations", job)
                return False
        if not job.SetupSheet:
            self.setupSetupSheet(job)
            job.SetupSheet.Label = "SetupSheet" + suffix
            if not job.SetupSheet:
                errorMessage("SetupSheet", job)
                return False
        if not job.Model:
            self.setupBaseModel(job)
            job.Model.Label = "Model" + suffix
            if not job.Model:
                errorMessage("Model", job)
                return False
        if not job.Stock:
            self.setupStock(job)
            job.Stock.Label = "Stock" + suffix
            if not job.Stock:
                errorMessage("Stock", job)
                return False
        if not job.Tools:
            self.setupToolTable(job)
            job.Tools.Label = "Tools" + suffix
            if not job.Tools:
                errorMessage("Tools", job)
                return False
        return True

    @classmethod
    def baseCandidates(cls):
        """Answer all objects in the current document which could serve as a Base for a job."""
        return sorted(
            [obj for obj in FreeCAD.ActiveDocument.Objects if cls.isBaseCandidate(obj)],
            key=lambda o: o.Label,
        )

    @classmethod
    def isBaseCandidate(cls, obj):
        """Answer true if the given object can be used as a Base for a job."""
        return PathUtil.isValidBaseObject(obj)


def Instances():
    """Instances() ... Return all Jobs in the current active document."""
    if FreeCAD.ActiveDocument:
        return [
            job
            for job in FreeCAD.ActiveDocument.Objects
            if hasattr(job, "Proxy") and isinstance(job.Proxy, ObjectJob)
        ]
    return []


def Create(name, base, templateFile=None):
    """Create(name, base, templateFile=None) ... creates a new job and all it's resources.
    If a template file is specified the new job is initialized with the values from the template."""
    if isinstance(base[0], str):
        models = []
        for baseName in base:
            models.append(FreeCAD.ActiveDocument.getObject(baseName))
    else:
        models = base
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.addExtension("App::GroupExtensionPython")
    obj.Proxy = ObjectJob(obj, models, templateFile)
    return obj
