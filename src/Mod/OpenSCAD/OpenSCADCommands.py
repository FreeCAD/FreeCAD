import FreeCAD,FreeCADGui
from PyQt4 import QtGui, QtCore

class ColorCodeShape:
    "Change the Color of selected or all Shapes based on their validity"
    def Activated(self):
        import colorcodeshapes
        selection=FreeCADGui.Selection.getSelectionEx()
        if len(selection) > 0:
            objs=[selobj.Object for selobj in selection]

        else:
            objs=FreeCAD.ActiveDocument.Objects
        colorcodeshapes.colorcodeshapes(objs)
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_ColorCodeShape', 'MenuText': 'Color Shapes', 'ToolTip': 'Color Shapes by validity and type'}

class Edgestofaces:
    def Activated(self):
        from OpenSCAD2Dgeom import edgestofaces,Overlappingfaces
        selection=FreeCADGui.Selection.getSelectionEx()
        edges=[]
        for selobj in selection:
            edges.extend(selobj.Object.Shape.Edges)
        Overlappingfaces(edgestofaces(edges,None)).makefeatures(FreeCAD.ActiveDocument)
        for selobj in selection:
            selobj.Object.ViewObject.hide()
        FreeCAD.ActiveDocument.recompute()

    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': 'EdgesToFaces', 'ToolTip': 'Convert Edges to Faces'}

class RefineShapeFeature:
    def Activated(self):
        import Part,OpenSCADFeatures
        selection=FreeCADGui.Selection.getSelectionEx()
        for selobj in selection:
            #newobj=FreeCAD.ActiveDocument.addObject("Part::FeaturePython",'refine')
            newobj=selobj.Document.addObject("Part::FeaturePython",'refine')
            OpenSCADFeatures.RefineShape(newobj,selobj.Object)
            OpenSCADFeatures.ViewProviderTree(newobj.ViewObject)
            newobj.Label='refine_%s' % selobj.Object.Label
            selobj.Object.ViewObject.hide()
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_RefineShapeFeature', 'MenuText': \
                'Refine Shape Feature', 'ToolTip': 'Create Refine Shape Feature'}


class ExpandPlacements:
    '''This should aid interactive repair in the future
    but currently it breaks extrusions, as axis, base and so on have to be
    recalculated'''
    def Activated(self):
        import expandplacements
        selobj=FreeCADGui.Selection.getSelectionEx()[0]
        expandplacements.expandplacements(selobj.Object,FreeCAD.Placement())
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': 'Expand Placements', 'ToolTip': 'Expand all placements downwards the FeatureTree'}

class ReplaceObject:
    def Activated(self):
        import replaceobj
        #objs=[selobj.Object for selobj in FreeCADGui.Selection.getSelectionEx()]
        objs=FreeCADGui.Selection.getSelection()
        if len(objs)==3:
            replaceobj.replaceobjfromselection(objs)
        else:
            FreeCAD.Console.PrintError('please select 3 objects first')
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_ReplaceObject', 'MenuText': \
                'Replace Object', 'ToolTip': \
                'Replace an object in the Feature Tree select old, new and parent object'}


class RemoveSubtree:
    def Activated(self):
        def addsubobjs(obj,toremoveset):
            toremove.add(obj)
            for subobj in obj.OutList:
                addsubobjs(subobj,toremoveset)

        import FreeCAD,FreeCADGui
        objs=FreeCADGui.Selection.getSelection()
        toremove=set()
        for obj in objs:
            addsubobjs(obj,toremove)
        checkinlistcomplete =False
        while not checkinlistcomplete:
            for obj in toremove:
                if (obj not in objs) and (frozenset(obj.InList) - toremove):
                    toremove.remove(obj)
                    break
            else:
                checkinlistcomplete = True
        for obj in toremove:
            obj.Document.removeObject(obj.Name)
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_RemoveSubtree', 'MenuText': \
                'Remove Objects and thier Children', 'ToolTip': \
                'Removes the selected Objects and all Children that are not referenced from other objects'}

class AddSCADWidget(QtGui.QWidget):
    def __init__(self,*args):
        QtGui.QWidget.__init__(self,*args)
        self.textEdit=QtGui.QTextEdit()
        self.buttonadd = QtGui.QPushButton(u'Add')
        self.buttonclear = QtGui.QPushButton(u'Clear')
        self.checkboxmesh = QtGui.QCheckBox(u'as Mesh')
        layouth=QtGui.QHBoxLayout()
        layouth.addWidget(self.buttonadd)
        layouth.addWidget(self.buttonclear)
        layout= QtGui.QVBoxLayout()
        layout.addLayout(layouth)
        layout.addWidget(self.checkboxmesh)
        layout.addWidget(self.textEdit)
        self.setLayout(layout)
        self.setWindowTitle(u'Add OpenSCAD Element')
        self.textEdit.setText(u'cube();')
        self.buttonclear.clicked.connect(self.textEdit.clear)

class AddSCADTask:
    def __init__(self):
        self.form = AddSCADWidget()
        self.form.buttonadd.clicked.connect(self.addelement)
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def isAllowedAlterSelection(self):
        return True

    def isAllowedAlterView(self):
        return True

    def isAllowedAlterDocument(self):
        return True

    def addelement(self):
        scadstr=unicode(self.form.textEdit.toPlainText())
        asmesh=self.form.checkboxmesh.checkState()
        import OpenSCADUtils, os
        extension= 'stl' if asmesh else 'csg'
        tmpfilename=OpenSCADUtils.callopenscadstring(scadstr,extension)
        if tmpfilename:
            doc=FreeCAD.activeDocument() or FreeCAD.newDocument()
            if asmesh:
                import Mesh
                Mesh.insert(tmpfilename,doc.Name)
            else:
                import importCSG
                importCSG.insert(tmpfilename,doc.Name)
            os.unlink(tmpfilename)
        else:
            FreeCAD.Console.PrintError('Running OpenSCAD failed\n')

class AddOpenSCADElement:
    def Activated(self):
        panel = AddSCADTask()
        FreeCADGui.Control.showDialog(panel)
    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': \
                'Add OpenSCAD Element...', 'ToolTip': \
                'Add an OpenSCAD Element by entering OpenSCAD Code and executing the OpenSCAD binary'}


FreeCADGui.addCommand('ColorCodeShape',ColorCodeShape())
FreeCADGui.addCommand('Edgestofaces',Edgestofaces())
FreeCADGui.addCommand('RefineShapeFeature',RefineShapeFeature())
FreeCADGui.addCommand('ExpandPlacements',ExpandPlacements())
FreeCADGui.addCommand('ReplaceObject',ReplaceObject())
FreeCADGui.addCommand('RemoveSubtree',RemoveSubtree())
FreeCADGui.addCommand('AddOpenSCADElement',AddOpenSCADElement())
