#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Sebastian Hoogen <github@sebastianhoogen.de>       *
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

__title__="FreeCAD OpenSCAD Workbench - GUI Commands"
__author__ = "Sebastian Hoogen"
__url__ = ["http://www.freecadweb.org"]

'''
This Script includes the GUI Commands of the OpenSCAD module
'''

import FreeCAD,FreeCADGui
from PySide import QtCore, QtGui
def translate(context,text):
    "convenience function for Qt translator"
    return QtGui.QApplication.translate(context, text, None, \
        QtGui.QApplication.UnicodeUTF8)
def utf8(unio):
    return unicode(unio).encode('UTF8')

class ExplodeGroup:
    "Ungroup Objects"
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0

    def Activated(self):
        def isdefault(shapecolor):
            def comparefloat(f1,f2):
                if f1 == 0.0:
                    return f1 == f2
                else:
                    return abs((f1-f2)/f1) < 2**-24
            scol=FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")\
                    .GetUnsigned('DefaultShapeColor',0xccccccff)
            defaultcolor = (((scol >> 24) & 0xff) / 255.0,\
                    ((scol >> 16) & 0xff) / 255.0,\
                    ((scol >>  8) & 0xff) / 255.0, 0.0)
            return all(all(comparefloat(fcc,dcc) for fcc,dcc in \
                    zip(facecolor,defaultcolor))  for facecolor in shapecolor)

        def isgrey(shapecolor):
            defaultcolor=(float.fromhex('0x1.99999ap-1'),float.fromhex(\
                    '0x1.99999ap-1'),float.fromhex('0x1.99999ap-1'),0.0)
            return all(facecolor == defaultcolor  for facecolor in shapecolor)

        def randomcolor(transp=0.0):
            import random
            return (random.random(),random.random(),random.random(),transp)

        def explode(obj,color=True):
            if obj.isDerivedFrom('Part::Fuse') or \
                    obj.isDerivedFrom('Part::MultiFuse') or \
                    obj.isDerivedFrom('Part::Compound'):
                plm = obj.Placement
                outlist = obj.OutList[:]
                if plm.isNull() or all((len(oo.InList)==1 and \
                        not oo.isDerivedFrom('PartDesign::Feature')) \
                        for oo in obj.OutList):
                    obj.Document.removeObject(obj.Name)
                    for oo in outlist:
                        if not plm.isNull():
                            oo.Placement=plm.multiply(oo.Placement)
                        if FreeCAD.GuiUp:
                            import FreeCADGui
                            oo.ViewObject.show()
                            if color and isdefault(oo.ViewObject.DiffuseColor):
                                if color == True:
                                    oo.ViewObject.DiffuseColor=randomcolor()
                                else:
                                    oo.ViewObject.DiffuseColor=color
                else:
                    FreeCAD.Console.PrintError(unicode(translate('OpenSCAD',\
                    'Unable to explode %s')) % obj.Name +u'\n')

        for obj in FreeCADGui.Selection.getSelection():
            if len(obj.InList) == 0: # allowed only for for top level objects
                explode(obj)

    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_Explode_Group', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ExplodeGroup',\
                'Explode Group'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ExplodeGroup',\
                'remove fusion, apply placement to children and color randomly')}

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
        return {'Pixmap'  : 'OpenSCAD_ColorCodeShape', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ColorCodeShape',\
                'Color Shapes'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ColorCodeShape',\
                'Color Shapes by validity and type')}

class Edgestofaces:
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0

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
        return {'Pixmap'  : 'python', 'MenuText': QtCore.QT_TRANSLATE_NOOP(\
                'OpenSCAD_Edgestofaces','Convert Edges To Faces'),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP('OpenSCAD',\
                'Convert Edges to Faces')}

class RefineShapeFeature:
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0

    def Activated(self):
        import Part,OpenSCADFeatures
        selection=FreeCADGui.Selection.getSelectionEx()
        for selobj in selection:
            newobj=selobj.Document.addObject("Part::FeaturePython",'refine')
            OpenSCADFeatures.RefineShape(newobj,selobj.Object)
            OpenSCADFeatures.ViewProviderTree(newobj.ViewObject)
            newobj.Label='refine_%s' % selobj.Object.Label
            selobj.Object.ViewObject.hide()
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_RefineShapeFeature', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_RefineShapeFeature',\
                'Refine Shape Feature'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_RefineShapeFeature',\
                'Create Refine Shape Feature')}

class IncreaseToleranceFeature:
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0

    def Activated(self):
        import Part,OpenSCADFeatures
        selection=FreeCADGui.Selection.getSelectionEx()
        for selobj in selection:
            newobj=selobj.Document.addObject("Part::FeaturePython",'tolerance')
            OpenSCADFeatures.IncreaseTolerance(newobj,selobj.Object)
            OpenSCADFeatures.ViewProviderTree(newobj.ViewObject)
            newobj.Label='tolerance_%s' % selobj.Object.Label
            selobj.Object.ViewObject.hide()
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_IncreaseToleranceFeature', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_IncreaseToleranceFeature',\
                'Increase Tolerance Feature'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_IncreaseToleranceFeature',\
                'Create Feature that allows to increase the tolerance')}


class ExpandPlacements:
    '''This should aid interactive repair in the future
    but currently it breaks extrusions, as axis, base and so on have to be
    recalculated'''
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0

    def Activated(self):
        import expandplacements
        for selobj in FreeCADGui.Selection.getSelectionEx():
            expandplacements.expandplacements(selobj.Object,FreeCAD.Placement())
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': QtCore.QT_TRANSLATE_NOOP(\
                'OpenSCAD_ExpandPlacements','Expand Placements'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ExpandPlacements',\
                'Expand all placements downwards the FeatureTree')}

class ReplaceObject:
    def IsActive(self):
        nobj = FreeCADGui.Selection.countObjectsOfType('Part::Feature')
        if nobj == 3: return True
        elif nobj == 2: return tuple((len(obj.InList)) for obj in \
                FreeCADGui.Selection.getSelection()) in ((0,1),(1,0))
        #else: return False

    def Activated(self):
        import replaceobj
        objs=FreeCADGui.Selection.getSelection()
        if len(objs)==3 or \
                tuple((len(obj.InList)) for obj in objs) in ((0,1),(1,0)):
            replaceobj.replaceobjfromselection(objs)
        else:
            FreeCAD.Console.PrintError(unicode(translate('OpenSCAD',\
                    'Please select 3 objects first'))+u'\n')
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_ReplaceObject', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ReplaceObject',\
                'Replace Object'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_ReplaceObject',\
                'Replace an object in the Feature Tree. Please select old, new and parent object')}


class RemoveSubtree:
    def IsActive(self):
        return FreeCADGui.Selection.countObjectsOfType('Part::Feature') > 0
    def Activated(self):
        import OpenSCADUtils,FreeCADGui
        OpenSCADUtils.removesubtree(FreeCADGui.Selection.getSelection())

    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_RemoveSubtree', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_RemoveSubtree',\
                'Remove Objects and their Children'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_RemoveSubtree',\
                'Removes the selected objects and all children that are not referenced from other objects')}

class AddSCADWidget(QtGui.QWidget):
    def __init__(self,*args):
        QtGui.QWidget.__init__(self,*args)
        self.textEdit=QtGui.QTextEdit()
        self.buttonadd = QtGui.QPushButton(translate('OpenSCAD','Add'))
        self.buttonclear = QtGui.QPushButton(translate('OpenSCAD','Clear'))
        self.checkboxmesh = QtGui.QCheckBox(translate('OpenSCAD','as Mesh'))
        layouth=QtGui.QHBoxLayout()
        layouth.addWidget(self.buttonadd)
        layouth.addWidget(self.buttonclear)
        layout= QtGui.QVBoxLayout()
        layout.addLayout(layouth)
        layout.addWidget(self.checkboxmesh)
        layout.addWidget(self.textEdit)
        self.setLayout(layout)
        self.setWindowTitle(translate('OpenSCAD','Add OpenSCAD Element'))
        self.textEdit.setText(u'cube();')
        self.buttonclear.clicked.connect(self.textEdit.clear)

    def retranslateUi(self, widget=None):
        self.buttonadd.setText(translate('OpenSCAD','Add'))
        self.buttonclear.setText(translate('OpenSCAD','Clear'))
        self.checkboxmesh.setText(translate('OpenSCAD','as Mesh'))
        self.setWindowTitle(translate('OpenSCAD','Add OpenSCAD Element'))

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
        scadstr=unicode(self.form.textEdit.toPlainText()).encode('utf8')
        asmesh=self.form.checkboxmesh.checkState()
        import OpenSCADUtils, os
        extension= 'stl' if asmesh else 'csg'
        try:
            tmpfilename=OpenSCADUtils.callopenscadstring(scadstr,extension)
            doc=FreeCAD.activeDocument() or FreeCAD.newDocument()
            if asmesh:
                import Mesh
                Mesh.insert(tmpfilename,doc.Name)
            else:
                import importCSG
                importCSG.insert(tmpfilename,doc.Name)
            try:
                os.unlink(tmpfilename)
            except OSError:
                pass

        except OpenSCADUtils.OpenSCADError as e:
            FreeCAD.Console.PrintError(e.value)

class OpenSCADMeshBooleanWidget(QtGui.QWidget):
    def __init__(self,*args):
        QtGui.QWidget.__init__(self,*args)
        #self.textEdit=QtGui.QTextEdit()
        self.buttonadd = QtGui.QPushButton(translate('OpenSCAD','Perform'))
        self.rb_group = QtGui.QButtonGroup()
        self.rb_group_box = QtGui.QGroupBox()
        self.rb_group_box_layout = QtGui.QVBoxLayout()
        self.rb_group_box.setLayout(self.rb_group_box_layout)
        self.rb_union = QtGui.QRadioButton("Union")
        self.rb_group.addButton(self.rb_union)
        self.rb_group_box_layout.addWidget(self.rb_union)
        self.rb_intersection = QtGui.QRadioButton("Intersection")
        self.rb_group.addButton(self.rb_intersection)
        self.rb_group_box_layout.addWidget(self.rb_intersection)
        self.rb_difference = QtGui.QRadioButton("Difference")
        self.rb_group.addButton(self.rb_difference)
        self.rb_group_box_layout.addWidget(self.rb_difference)
        self.rb_hull = QtGui.QRadioButton("Hull")
        self.rb_group.addButton(self.rb_hull)
        self.rb_group_box_layout.addWidget(self.rb_hull)
        self.rb_minkowski = QtGui.QRadioButton("Minkowski")
        self.rb_group.addButton(self.rb_minkowski)
        self.rb_group_box_layout.addWidget(self.rb_minkowski)
        layouth=QtGui.QHBoxLayout()
        layouth.addWidget(self.buttonadd)
        layout= QtGui.QVBoxLayout()
        layout.addLayout(layouth)
        layout.addWidget(self.rb_group_box)
        self.setLayout(layout)
        self.setWindowTitle(translate('OpenSCAD','Mesh Boolean'))

    def retranslateUi(self, widget=None):
        self.buttonadd.setText(translate('OpenSCAD','Perform'))
        self.setWindowTitle(translate('OpenSCAD','Mesh Boolean'))

class OpenSCADMeshBooleanTask:
    def __init__(self):
        pass
        self.form = OpenSCADMeshBooleanWidget()
        self.form.buttonadd.clicked.connect(self.doboolean)
    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def isAllowedAlterSelection(self):
        return False

    def isAllowedAlterView(self):
        return False

    def isAllowedAlterDocument(self):
        return True

    def doboolean(self):
        from OpenSCADUtils import meshoponobjs
        if self.form.rb_intersection.isChecked(): opname = 'intersection'
        elif self.form.rb_difference.isChecked(): opname = 'difference'
        elif self.form.rb_hull.isChecked():       opname = 'hull'
        elif self.form.rb_minkowski.isChecked():  opname = 'minkowski'
        else: opname = 'union'
        newmesh,objsused = meshoponobjs(opname,FreeCADGui.Selection.getSelection())
        if len(objsused) > 0:
            newmeshobj = FreeCAD.activeDocument().addObject('Mesh::Feature',opname) #create a Feature for the result
            newmeshobj.Mesh = newmesh #assign the result to the new Feature
            for obj in objsused:
                obj.ViewObject.hide() #hide the selected Features

class AddOpenSCADElement:
    def IsActive(self):
        return not FreeCADGui.Control.activeDialog()
    def Activated(self):
        panel = AddSCADTask()
        FreeCADGui.Control.showDialog(panel)
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_AddOpenSCADElement', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_AddOpenSCADElement',\
                'Add OpenSCAD Element...'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_AddOpenSCADElement',\
                'Add an OpenSCAD element by entering OpenSCAD code and executing the OpenSCAD binary')}

class OpenSCADMeshBoolean:
    def IsActive(self):
        return not FreeCADGui.Control.activeDialog() and \
            len(FreeCADGui.Selection.getSelection()) >= 1
    def Activated(self):
        panel = OpenSCADMeshBooleanTask()
        FreeCADGui.Control.showDialog(panel)
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_MeshBooleans', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_MeshBoolean',\
                'Mesh Boolean...'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_MeshBoolean',\
                'Export objects as meshes and use OpenSCAD to perform a boolean operation.')}

class Hull:
    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) >= 2

    def Activated(self):
        import Part,OpenSCADFeatures
        import importCSG
        selection=FreeCADGui.Selection.getSelectionEx()
        objList = []
        for selobj in selection:
            objList.append(selobj.Object)
            selobj.Object.ViewObject.hide()
        importCSG.process_ObjectsViaOpenSCAD(FreeCAD.activeDocument(),objList,"hull")
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_Hull', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_Hull',\
                'Hull'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_Hull',\
                'Perform Hull')}

class Minkowski:
    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) >= 2

    def Activated(self):
        import Part,OpenSCADFeatures
        import importCSG
        selection=FreeCADGui.Selection.getSelectionEx()
        objList = []
        for selobj in selection:
            objList.append(selobj.Object)
            selobj.Object.ViewObject.hide()
        importCSG.process_ObjectsViaOpenSCAD(FreeCAD.activeDocument(),objList,"minkowski")
        FreeCAD.ActiveDocument.recompute()
    def GetResources(self):
        return {'Pixmap'  : 'OpenSCAD_Minkowski', 'MenuText': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_Minkowski',\
                'Minkowski'), 'ToolTip': \
                QtCore.QT_TRANSLATE_NOOP('OpenSCAD_Minkowski',\
                'Perform Minkowski')}

FreeCADGui.addCommand('OpenSCAD_ColorCodeShape',ColorCodeShape())
FreeCADGui.addCommand('OpenSCAD_ExplodeGroup',ExplodeGroup())
FreeCADGui.addCommand('OpenSCAD_Edgestofaces',Edgestofaces())
FreeCADGui.addCommand('OpenSCAD_RefineShapeFeature',RefineShapeFeature())
FreeCADGui.addCommand('OpenSCAD_IncreaseToleranceFeature',IncreaseToleranceFeature())
FreeCADGui.addCommand('OpenSCAD_ExpandPlacements',ExpandPlacements())
FreeCADGui.addCommand('OpenSCAD_ReplaceObject',ReplaceObject())
FreeCADGui.addCommand('OpenSCAD_RemoveSubtree',RemoveSubtree())
FreeCADGui.addCommand('OpenSCAD_AddOpenSCADElement',AddOpenSCADElement())
FreeCADGui.addCommand('OpenSCAD_MeshBoolean',OpenSCADMeshBoolean())
FreeCADGui.addCommand('OpenSCAD_Hull',Hull())
FreeCADGui.addCommand('OpenSCAD_Minkowski',Minkowski())
