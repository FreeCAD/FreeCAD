#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

import FreeCAD,FreeCADGui,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
from PyQt4 import QtCore
from DraftTools import translate

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://free-cad.sourceforge.net"

# Make some strings picked by the translator
QtCore.QT_TRANSLATE_NOOP("Arch","Wood")
QtCore.QT_TRANSLATE_NOOP("Arch","Steel")

# Presets in the form: Class, Name, Width, Height, [Web thickness, Flange thickness]
Presets = [None,

            # wood sections
            
            ["Wood","1x2in",19,28],
            ["Wood","1x3in",19,64],
            ["Wood","1x4in",19,89],
            ["Wood","1x6in",19,89],
            ["Wood","1x8in",19,140],
            ["Wood","1x10in",19,184],
            ["Wood","1x12in",19,286],
            
            ["Wood","2x2in",38,38],
            ["Wood","2x3in",38,64],
            ["Wood","2x4in",38,89],
            ["Wood","2x6in",38,140],
            ["Wood","2x8in",38,184],
            ["Wood","2x10in",38,235],
            ["Wood","2x12in",38,286],
            
            ["Wood","4x4in",89,89],
            ["Wood","4x6in",89,140],
            ["Wood","6x6in",140,140],
            ["Wood","8x8in",184,184],
            
            # HEA 
            
            ["Steel","HEA100",96,100,5,8],
            ["Steel","HEA120",114,120,5,8],
            ["Steel","HEA140",133,140,5.5,8.5],
            ["Steel","HEA160",152,160,6,9],
            ["Steel","HEA180",171,180,6,9.5],
            ["Steel","HEA200",190,200,6.5,10],
            ["Steel","HEA220",210,220,7,11],
            ["Steel","HEA240",230,240,7.5,12],
            ["Steel","HEA260",250,260,7.5,12.5],
            ["Steel","HEA280",270,280,8,13],
            ["Steel","HEA300",290,300,8.5,14],
            ["Steel","HEA320",310,300,9,15.5],
            ["Steel","HEA340",330,300,9.5,16.5],
            ["Steel","HEA360",350,300,10,17.5],
            ["Steel","HEA400",390,300,11,19],
            ["Steel","HEA450",440,300,11.5,21],
            ["Steel","HEA500",490,300,12,23],
            ["Steel","HEA550",540,300,12.5,24],
            ["Steel","HEA600",590,300,13,25],
            ["Steel","HEA650",640,300,13.5,26],
            ["Steel","HEA700",690,300,14.5,27],
            ["Steel","HEA800",790,300,15,28],
            ["Steel","HEA900",890,300,16,30],
            ["Steel","HEA1000",990,300,16.5,31],
            
            # HEAA 
            
            ["Steel","HEAA100",91,100,4.2,5.5],
            ["Steel","HEAA120",109,120,4.2,5.5],
            ["Steel","HEAA140",128,140,4.3,6],
            ["Steel","HEAA160",148,160,4.5,7],
            ["Steel","HEAA180",167,180,5,7.5],
            ["Steel","HEAA200",186,200,5.5,8],
            ["Steel","HEAA220",205,220,6,8.5],
            ["Steel","HEAA240",224,240,6.5,9],
            ["Steel","HEAA260",244,260,6.5,9.5],
            ["Steel","HEAA280",264,280,7,10],
            ["Steel","HEAA300",283,300,7.5,10.5],
            ["Steel","HEAA320",301,300,8,11],
            ["Steel","HEAA340",320,300,8.5,11.5],
            ["Steel","HEAA360",339,300,9,12],
            ["Steel","HEAA400",378,300,9.5,13],
            ["Steel","HEAA450",425,300,10,13.5],
            ["Steel","HEAA500",472,300,10.5,14],
            ["Steel","HEAA550",522,300,11.5,15],
            ["Steel","HEAA600",571,300,12,15.5],
            ["Steel","HEAA650",620,300,12.5,16],
            ["Steel","HEAA700",670,300,13,17],
            ["Steel","HEAA800",770,300,14,18],
            ["Steel","HEAA900",870,300,15,20],
            ["Steel","HEAA1000",970,300,16,21],
            
            # HEB 
            
            ["Steel","HEB100",100,100,6,10],
            ["Steel","HEB120",120,120,6.5,11],
            ["Steel","HEB140",140,140,7,12],
            ["Steel","HEB160",160,160,8,13],
            ["Steel","HEB180",180,180,8.5,14],
            ["Steel","HEB200",200,200,9,15],
            ["Steel","HEB220",220,220,9.5,16],
            ["Steel","HEB240",240,240,10,17],
            ["Steel","HEB260",260,260,10,17.5],
            ["Steel","HEB280",280,280,10.5,18],
            ["Steel","HEB300",300,300,11,19],
            ["Steel","HEB320",320,300,11.5,20.5],
            ["Steel","HEB340",340,300,12,21.5],
            ["Steel","HEB360",360,300,12.5,22.5],
            ["Steel","HEB400",400,300,13.5,24],
            ["Steel","HEB450",450,300,14,26],
            ["Steel","HEB500",500,300,14.5,28],
            ["Steel","HEB550",550,300,15,29],
            ["Steel","HEB600",600,300,15.5,30],
            ["Steel","HEB650",650,300,16,31],
            ["Steel","HEB700",700,300,17,32],
            ["Steel","HEB800",800,300,17.5,33],
            ["Steel","HEB900",900,300,18.5,35],
            ["Steel","HEB1000",1000,300,19,36],
            
            # HEM 
            
            ["Steel","HEM160",180,166,14,23],
            ["Steel","HEM180",200,186,14.5,24],
            ["Steel","HEM200",220,206,15,25],
            ["Steel","HEM220",240,226,15.5,26],
            ["Steel","HEM240",270,248,18,32],
            ["Steel","HEM260",290,268,18,32.5],
            ["Steel","HEM280",310,288,18.5,33],
            ["Steel","HEM300",340,310,21,39],
            ["Steel","HEM320",359,309,21,40],
            ["Steel","HEM340",377,309,21,40],
            ["Steel","HEM360",395,308,21,40],
            ["Steel","HEM400",432,307,21,40],
            ["Steel","HEM450",478,307,21,40],
            ["Steel","HEM500",524,306,21,40],
            ["Steel","HEM550",572,306,21,40],
            ["Steel","HEM600",620,305,21,40],
            ["Steel","HEM650",668,305,21,40],
            ["Steel","HEM700",716,304,21,40],
            ["Steel","HEM800",814,303,21,40],
            ["Steel","HEM900",910,302,21,40],
            ["Steel","HEM1000",1008,302,21,40],
            
            # INP
            
            ["Steel","INP80",42,80,3.9,5.9],
            ["Steel","INP100",50,100,4.5,6.8],
            ["Steel","INP120",58,120,5.1,7.7],
            ["Steel","INP140",66,140,5.7,8.6],
            ["Steel","INP160",74,160,6.3,9.5],
            ["Steel","INP180",82,180,6.9,10.4],
            ["Steel","INP200",90,200,7.5,11.3],
            ["Steel","INP220",98,220,8.1,12.2],
            ["Steel","INP240",106,240,8.7,13.1],
            ["Steel","INP260",113,260,9.4,14.1],
            ["Steel","INP280",119,280,10.1,15.2],
            ["Steel","INP300",125,300,10.8,16.2],
            ["Steel","INP320",131,320,11.5,17.3],
            ["Steel","INP340",137,340,12.2,18.3],
            ["Steel","INP360",143,360,13,19.5],
            ["Steel","INP380",149,380,13.7,20.5],
            ["Steel","INP400",155,400,14.4,21.6],
            
            # IPE 
            
            ["Steel","IPE100",100,55,4.1,5.7],
            ["Steel","IPE120",120,64,4.4,6.3],
            ["Steel","IPE140",140,73,4.7,6.9],
            ["Steel","IPE160",160,82,5,7.4],
            ["Steel","IPE180",180,91,5.3,8],
            ["Steel","IPE200",200,100,5.6,8.5],
            ["Steel","IPE220",220,110,5.9,9.2],
            ["Steel","IPE240",240,120,6.2,9.8],
            ["Steel","IPE270",270,135,6.6,10.2],
            ["Steel","IPE300",300,150,7.1,10.7],
            ["Steel","IPE330",330,160,7.5,11.5],
            ["Steel","IPE360",360,170,8,12.7],
            ["Steel","IPE400",400,180,8.6,13.5],
            ["Steel","IPE450",450,190,9.4,14.6],
            ["Steel","IPE500",500,200,10.2,16],
            ["Steel","IPE550",550,210,11.1,17.2],
            ["Steel","IPE600",600,220,12,19],
            ["Steel","IPE750x137",753,263,11.5,17],
            ["Steel","IPE750x147",753,265,13.2,17],
            ["Steel","IPE750x161",758,266,13.8,19.3],
            ["Steel","IPE750x173",762,267,14.4,21.6],
            ["Steel","IPE750x185",766,267,14.9,23.6],
            ["Steel","IPE750x196",770,268,15.6,25.4],
            ["Steel","IPE750x210",775,268,16,28],
            ["Steel","IPE750x222",778,269,17,29.5],
            
            # IPEA 
            
            ["Steel","IPEA100",98,55,3.6,4.7],
            ["Steel","IPEA120",118,64,3.8,5.1],
            ["Steel","IPEA140",138,73,3.8,5.6],
            ["Steel","IPEA160",157,82,4,5.9],
            ["Steel","IPEA180",177,91,4.3,6.5],
            ["Steel","IPEA200",197,100,4.5,7],
            ["Steel","IPEA220",217,110,5,7.7],
            ["Steel","IPEA240",237,120,5.2,8.3],
            ["Steel","IPEA270",267,135,5.5,8.7],
            ["Steel","IPEA300",297,150,6.1,9.2],
            ["Steel","IPEA330",327,160,6.5,10],
            ["Steel","IPEA360",357.6,170,6.6,11.5],
            ["Steel","IPEA400",397,180,7,12],
            ["Steel","IPEA450",447,190,7.6,13.1],
            ["Steel","IPEA500",497,200,8.4,14.5],
            ["Steel","IPEA550",547,210,9,15.7],
            ["Steel","IPEA600",597,220,9.8,17.5],
            
            # IPEO 
            
            ["Steel","IPEO180",182,89,6.4,9.5],
            ["Steel","IPEO200",202,102,6.2,9.5],
            ["Steel","IPEO220",222,112,6.6,10.2],
            ["Steel","IPEO240",242,122,7,10.8],
            ["Steel","IPEO270",274,136,7.5,12.2],
            ["Steel","IPEO300",304,152,8,12.7],
            ["Steel","IPEO330",334,162,8.5,13.5],
            ["Steel","IPEO360",364,172,9.2,14.7],
            ["Steel","IPEO400",404,182,9.7,15.5],
            ["Steel","IPEO450",456,192,11,17.6],
            ["Steel","IPEO500",506,202,12,19],
            ["Steel","IPEO550",556,212,12.7,20.2],
            ["Steel","IPEO600",610,224,15,24],
            ["Steel","IPER140",142,72,5.3,7.8],
            ["Steel","IPER160",162,81,5.6,8.5],
            ["Steel","IPER180",183,92,6,9],
            ["Steel","IPER200",204,98,6.6,10.5],
            ["Steel","IPER220",225,108,6.7,11.8],
            ["Steel","IPER240",245,118,7.5,12.3],
            ["Steel","IPER270",276,133,7.1,13.1],
            ["Steel","IPER300",306,147,8.5,13.7],
            ["Steel","IPER330",336,158,9.2,14.5],
            ["Steel","IPER360",366,168,9.9,16],
            ["Steel","IPER400",407,178,10.6,17],
            ["Steel","IPER450",458,188,11.3,18.6],
            ["Steel","IPER500",508,198,12.6,20],
            ["Steel","IPER550",560,210,14,22.2],
            ["Steel","IPER600",608,218,14,23],
            
            # IPEV 
            
            ["Steel","IPEV400",408,182,10.6,17.5],
            ["Steel","IPEV450",460,194,12.4,19.6],
            ["Steel","IPEV500",514,204,14.2,23],
            ["Steel","IPEV550",566,216,17.1,25.2],
            ["Steel","IPEV600",618,228,18,28]

            ]

def makeStructure(baseobj=None,length=0,width=0,height=0,name=str(translate("Arch","Structure"))):
    '''makeStructure([obj],[length],[width],[heigth],[swap]): creates a
    structure element based on the given profile object and the given
    extrusion height. If no base object is given, you can also specify
    length and width for a cubic object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    _Structure(obj)
    _ViewProviderStructure(obj.ViewObject)
    if baseobj:
        obj.Base = baseobj
        obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    if height:
        obj.Height = height
    if length:
        obj.Length = length
    obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Structure")
    return obj

def makeStructuralSystem(objects,axes):
    '''makeStructuralSystem(objects,axes): makes a structural system
    based on the given objects and axes'''
    result = []
    if objects and axes:
        for o in objects:
            s = makeStructure(o)
            s.Axes = axes
            result.append(s)
        FreeCAD.ActiveDocument.recompute()
    return result
    
def makeProfile(W=46,H=80,tw=3.8,tf=5.2):
    '''makeProfile(W,H,tw,tf): returns a shape with one face describing 
    the profile of a steel beam (IPE, IPN, HE, etc...) based on the following
    dimensions: W = total width, H = total height, tw = web thickness
    tw = flange thickness (see http://en.wikipedia.org/wiki/I-beam for
    reference)'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython","Profile")
    _Profile(obj)
    obj.Width = W
    obj.Height = H
    obj.WebThickness = tw
    obj.FlangeThickness = tf
    Draft._ViewProviderDraft(obj.ViewObject)
    return obj

class _CommandStructure:
    "the Arch Structure command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}
        
    def Activated(self):
        
        global QtGui, QtCore
        from PyQt4 import QtGui, QtCore
        
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("StructureLength",100)
        self.Width = p.GetFloat("StructureWidth",100)
        self.Height = p.GetFloat("StructureHeight",1000)
        self.Profile = 0
        self.continueCmd = False
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            # direct creation
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
            FreeCADGui.doCommand("import Arch")
            # if selection contains structs and axes, make a system
            st = Draft.getObjectsOfType(sel,"Structure")
            ax = Draft.getObjectsOfType(sel,"Axis")
            if st and ax:
                FreeCADGui.doCommand("Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + "," + ArchCommands.getStringList(ax) + ")")
            else:
                # else, do normal structs
                for obj in sel:
                    FreeCADGui.doCommand("Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            # interactive mode
            if hasattr(FreeCAD,"DraftWorkingPlane"):
                FreeCAD.DraftWorkingPlane.setup()
            import DraftTrackers
            self.points = []
            self.tracker = DraftTrackers.boxTracker()
            self.tracker.width(self.Width)
            self.tracker.height(self.Height)
            self.tracker.length(self.Length)
            self.tracker.on()
            FreeCADGui.Snapper.getPoint(callback=self.getPoint,movecallback=self.update,extradlg=self.taskbox())
            
    def getPoint(self,point=None,obj=None):
        "this function is called by the snapper when it has a 3D point"
        self.tracker.finalize()
        if point == None:
            return
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
        FreeCADGui.doCommand('import Arch')
        if self.Profile:
            pr = Presets[self.Profile]
            FreeCADGui.doCommand('p = Arch.makeProfile('+str(pr[2])+','+str(pr[3])+','+str(pr[4])+','+str(pr[5])+')')
            if self.Length == pr[2]:
                # vertical
                FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Height)+')')
            else:
                # horizontal
                FreeCADGui.doCommand('s = Arch.makeStructure(p,height='+str(self.Length)+')')
                FreeCADGui.doCommand('s.Placement.Rotation = FreeCAD.Rotation(-0.5,0.5,-0.5,0.5)')
        else:
            FreeCADGui.doCommand('s = Arch.makeStructure(length='+str(self.Length)+',width='+str(self.Width)+',height='+str(self.Height)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if self.continueCmd:
            self.Activated()

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        w.setWindowTitle(str(translate("Arch","Structure options")))
        lay0 = QtGui.QVBoxLayout(w)
        
        # presets box
        layp = QtGui.QHBoxLayout()
        lay0.addLayout(layp)
        labelp = QtGui.QLabel(str(translate("Arch","Preset")))
        layp.addWidget(labelp)
        valuep = QtGui.QComboBox()
        fpresets = [" "]
        for p in Presets[1:]:
            fpresets.append(str(translate("Arch",p[0]))+" "+p[1]+" ("+str(p[2])+"x"+str(p[3])+"mm)")
        valuep.addItems(fpresets)
        layp.addWidget(valuep)
        
        # length
        lay1 = QtGui.QHBoxLayout()
        lay0.addLayout(lay1)
        label1 = QtGui.QLabel(str(translate("Arch","Length")))
        lay1.addWidget(label1)
        self.vLength = QtGui.QDoubleSpinBox()
        self.vLength.setDecimals(2)
        self.vLength.setMaximum(99999.99)
        self.vLength.setValue(self.Length)
        lay1.addWidget(self.vLength)
        
        # width
        lay2 = QtGui.QHBoxLayout()
        lay0.addLayout(lay2)
        label2 = QtGui.QLabel(str(translate("Arch","Width")))
        lay2.addWidget(label2)
        self.vWidth = QtGui.QDoubleSpinBox()
        self.vWidth.setDecimals(2)
        self.vWidth.setMaximum(99999.99)
        self.vWidth.setValue(self.Width)
        lay2.addWidget(self.vWidth)

        # height
        lay3 = QtGui.QHBoxLayout()
        lay0.addLayout(lay3)
        label3 = QtGui.QLabel(str(translate("Arch","Height")))
        lay3.addWidget(label3)
        self.vHeight = QtGui.QDoubleSpinBox()
        self.vHeight.setDecimals(2)
        self.vHeight.setMaximum(99999.99)
        self.vHeight.setValue(self.Height)
        lay3.addWidget(self.vHeight)
        
        # horizontal button
        value5 = QtGui.QPushButton(str(translate("Arch","Rotate")))
        lay0.addWidget(value5)

        # continue button
        value4 = QtGui.QCheckBox(str(translate("Arch","Continue")))
        lay0.addWidget(value4)
        
        QtCore.QObject.connect(valuep,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotate)
        return w
        
    def update(self,point):
        "this function is called by the Snapper when the mouse is moved"
        if self.Height >= self.Length:
            delta = Vector(0,0,self.Height/2)
        else:
            delta = Vector(self.Length/2,0,0)
        self.tracker.pos(point.add(delta))
        
    def setWidth(self,d):
        self.Width = d
        self.tracker.width(d)

    def setHeight(self,d):
        self.Height = d
        self.tracker.height(d)

    def setLength(self,d):
        self.Length = d
        self.tracker.length(d)

    def setContinue(self,i):
        self.continueCmd = bool(i)
        
    def setPreset(self,i):
        if i > 0:
            self.vLength.setValue(float(Presets[i][2]))
            self.vWidth.setValue(float(Presets[i][3]))
        if len(Presets[i]) == 6:
            self.Profile = i
        else:
            self.Profile = 0
            
    def rotate(self):
        l = self.Length
        w = self.Width
        h = self.Height
        self.vLength.setValue(h)
        self.vHeight.setValue(w)
        self.vWidth.setValue(l)
       
class _Structure(ArchComponent.Component):
    "The Structure object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Tool","Arch",
                        "An optional extrusion path for this element")
        obj.addProperty("App::PropertyLength","Length","Arch",
                        str(translate("Arch","The length of this element, if not based on a profile")))
        obj.addProperty("App::PropertyLength","Width","Arch",
                        str(translate("Arch","The width of this element, if not based on a profile")))
        obj.addProperty("App::PropertyLength","Height","Arch",
                        str(translate("Arch","The height or extrusion depth of this element. Keep 0 for automatic")))
        obj.addProperty("App::PropertyLinkList","Axes","Arch",
                        str(translate("Arch","Axes systems this structure is built on")))
        obj.addProperty("App::PropertyVector","Normal","Arch",
                        str(translate("Arch","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)")))
        obj.addProperty("App::PropertyIntegerList","Exclude","Arch",
                        str(translate("Arch","The element numbers to exclude when this structure is based on axes")))
        self.Type = "Structure"
        obj.Length = 1
        obj.Width = 1
        obj.Height = 1
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop in ["Base","Tool","Length","Width","Height","Normal","Additions","Subtractions","Axes"]:
            self.createGeometry(obj)

    def getAxisPoints(self,obj):
        "returns the gridpoints of linked axes"
        import DraftGeomUtils
        pts = []
        if len(obj.Axes) == 1:
            for e in obj.Axes[0].Shape.Edges:
                pts.append(e.Vertexes[0].Point)
        elif len(obj.Axes) >= 2:
            set1 = obj.Axes[0].Shape.Edges
            set2 = obj.Axes[1].Shape.Edges
            for e1 in set1:
                for e2 in set2: 
                    pts.extend(DraftGeomUtils.findIntersection(e1,e2))
        return pts

    def getAxisPlacement(self,obj):
        "returns an axis placement"
        if obj.Axes:
            return obj.Axes[0].Placement
        return None

    def createGeometry(self,obj):
        import Part, DraftGeomUtils
        
        # getting default values
        height = width = length = 1
        if hasattr(obj,"Length"):
            if obj.Length:
                length = obj.Length
        if hasattr(obj,"Width"):
            if obj.Width:
                width = obj.Width
        if hasattr(obj,"Height"):
            if obj.Height:
                height = obj.Height

        # creating base shape
        pl = obj.Placement
        base = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if hasattr(obj,"Tool"):
                    if obj.Tool:
                        try:
                            base = obj.Tool.Shape.copy().makePipe(obj.Base.Shape.copy())
                        except:
                            FreeCAD.Console.PrintError(str(translate("Arch","Error: The base shape couldn't be extruded along this tool object")))
                            return
                if not base:
                    if obj.Normal == Vector(0,0,0):
                        p = FreeCAD.Placement(obj.Base.Placement)
                        normal = p.Rotation.multVec(Vector(0,0,1))
                    else:
                        normal = Vector(obj.Normal)
                    normal = normal.multiply(height)
                    base = obj.Base.Shape.copy()
                    if base.Solids:
                        pass
                    elif base.Faces:
                        base = base.extrude(normal)
                    elif (len(base.Wires) == 1):
                        if base.Wires[0].isClosed():
                            base = Part.Face(base.Wires[0])
                            base = base.extrude(normal)
                            
            elif obj.Base.isDerivedFrom("Mesh::Feature"):
                if obj.Base.Mesh.isSolid():
                    if obj.Base.Mesh.countComponents() == 1:
                        sh = ArchCommands.getShapeFromMesh(obj.Base.Mesh)
                        if sh.isClosed() and sh.isValid() and sh.Solids:
                            base = sh
        else:
            if obj.Normal == Vector(0,0,0):
                normal = Vector(0,0,1)
            else:
                normal = Vector(obj.Normal)
            normal = normal.multiply(height)
            l2 = length/2 or 0.5
            w2 = width/2 or 0.5
            v1 = Vector(-l2,-w2,0)
            v2 = Vector(l2,-w2,0)
            v3 = Vector(l2,w2,0)
            v4 = Vector(-l2,w2,0)
            base = Part.makePolygon([v1,v2,v3,v4,v1])
            base = Part.Face(base)
            base = base.extrude(normal)
            
        base = self.processSubShapes(obj,base)
            
        if base:
            # applying axes
            pts = self.getAxisPoints(obj)
            apl = self.getAxisPlacement(obj)
            if pts:
                fsh = []
                for i in range(len(pts)):
                    if hasattr(obj,"Exclude"):
                        if i in obj.Exclude:
                            continue
                    sh = base.copy()
                    if apl:
                        sh.Placement.Rotation = apl.Rotation
                    sh.translate(pts[i])
                    fsh.append(sh)
                    obj.Shape = Part.makeCompound(fsh)

            # finalizing
            
            else:
                if base:
                    if not base.isNull():
                        if base.isValid() and base.Solids:
                            if base.Volume < 0:
                                base.reverse()
                            if base.Volume < 0:
                                FreeCAD.Console.PrintError(str(translate("Arch","Couldn't compute a shape")))
                                return
                            base = base.removeSplitter()
                            obj.Shape = base
                if not DraftGeomUtils.isNull(pl):
                    obj.Placement = pl


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Structure_Tree.svg"


class _Profile(Draft._DraftObject):
    "A parametric beam profile object"
    
    def __init__(self,obj):
        obj.addProperty("App::PropertyDistance","Width","Base","Width of the beam").Width = 10
        obj.addProperty("App::PropertyDistance","Height","Base","Height of the beam").Height = 30
        obj.addProperty("App::PropertyDistance","WebThickness","Base","Thickness of the webs").WebThickness = 3
        obj.addProperty("App::PropertyDistance","FlangeThickness","Base","Thickness of the flange").FlangeThickness = 2
        Draft._DraftObject.__init__(self,obj,"Profile")
        
    def execute(self,obj):
        self.createGeometry(obj)
        
    def onChanged(self,obj,prop):
        if prop in ["Width","Height","WebThickness","FlangeThickness"]:
            self.createGeometry(obj)
        
    def createGeometry(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width/2,-obj.Height/2,0)
        p2 = Vector(obj.Width/2,-obj.Height/2,0)
        p3 = Vector(obj.Width/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p4 = Vector(obj.WebThickness/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p5 = Vector(obj.WebThickness/2,obj.Height/2-obj.FlangeThickness,0)
        p6 = Vector(obj.Width/2,obj.Height/2-obj.FlangeThickness,0)
        p7 = Vector(obj.Width/2,obj.Height/2,0)
        p8 = Vector(-obj.Width/2,obj.Height/2,0)
        p9 = Vector(-obj.Width/2,obj.Height/2-obj.FlangeThickness,0)
        p10 = Vector(-obj.WebThickness/2,obj.Height/2-obj.FlangeThickness,0)
        p11 = Vector(-obj.WebThickness/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p12 = Vector(-obj.Width/2,(-obj.Height/2)+obj.FlangeThickness,0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p1])
        p = Part.Face(p)
        obj.Shape = p
        obj.Placement = pl


FreeCADGui.addCommand('Arch_Structure',_CommandStructure())
