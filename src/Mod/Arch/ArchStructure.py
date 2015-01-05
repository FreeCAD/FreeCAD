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

import FreeCAD,Draft,ArchComponent,DraftVecUtils,ArchCommands
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD Structure"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

# Make some strings picked by the translator
if FreeCAD.GuiUp:
    QtCore.QT_TRANSLATE_NOOP("Arch","Wood")
    QtCore.QT_TRANSLATE_NOOP("Arch","Steel")

# Possible roles for structural elements
Roles = ["Beam","Column","Slab","Wall","Curtain Wall","Roof","Foundation","Pile","Tendon"]

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

            ["Steel","HEA100",100,96,5,8],
            ["Steel","HEA120",120,114,5,8],
            ["Steel","HEA140",140,133,5.5,8.5],
            ["Steel","HEA160",160,152,6,9],
            ["Steel","HEA180",180,171,6,9.5],
            ["Steel","HEA200",200,190,6.5,10],
            ["Steel","HEA220",220,210,7,11],
            ["Steel","HEA240",240,230,7.5,12],
            ["Steel","HEA260",260,250,7.5,12.5],
            ["Steel","HEA280",280,270,8,13],
            ["Steel","HEA300",300,290,8.5,14],
            ["Steel","HEA320",300,310,9,15.5],
            ["Steel","HEA340",300,330,9.5,16.5],
            ["Steel","HEA360",300,350,10,17.5],
            ["Steel","HEA400",300,390,11,19],
            ["Steel","HEA450",300,440,11.5,21],
            ["Steel","HEA500",300,490,12,23],
            ["Steel","HEA550",300,540,12.5,24],
            ["Steel","HEA600",300,590,13,25],
            ["Steel","HEA650",300,640,13.5,26],
            ["Steel","HEA700",300,690,14.5,27],
            ["Steel","HEA800",300,790,15,28],
            ["Steel","HEA900",300,890,16,30],
            ["Steel","HEA1000",300,990,16.5,31],

            # HEAA

            ["Steel","HEAA100",100,91,4.2,5.5],
            ["Steel","HEAA120",120,109,4.2,5.5],
            ["Steel","HEAA140",140,128,4.3,6],
            ["Steel","HEAA160",160,148,4.5,7],
            ["Steel","HEAA180",180,167,5,7.5],
            ["Steel","HEAA200",200,186,5.5,8],
            ["Steel","HEAA220",220,205,6,8.5],
            ["Steel","HEAA240",240,224,6.5,9],
            ["Steel","HEAA260",260,244,6.5,9.5],
            ["Steel","HEAA280",280,264,7,10],
            ["Steel","HEAA300",300,283,7.5,10.5],
            ["Steel","HEAA320",300,301,8,11],
            ["Steel","HEAA340",300,320,8.5,11.5],
            ["Steel","HEAA360",300,339,9,12],
            ["Steel","HEAA400",300,378,9.5,13],
            ["Steel","HEAA450",300,425,10,13.5],
            ["Steel","HEAA500",300,472,10.5,14],
            ["Steel","HEAA550",300,522,11.5,15],
            ["Steel","HEAA600",300,571,12,15.5],
            ["Steel","HEAA650",300,620,12.5,16],
            ["Steel","HEAA700",300,670,13,17],
            ["Steel","HEAA800",300,770,14,18],
            ["Steel","HEAA900",300,870,15,20],
            ["Steel","HEAA1000",300,970,16,21],

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
            ["Steel","HEB320",300,320,11.5,20.5],
            ["Steel","HEB340",300,340,12,21.5],
            ["Steel","HEB360",300,360,12.5,22.5],
            ["Steel","HEB400",300,400,13.5,24],
            ["Steel","HEB450",300,450,14,26],
            ["Steel","HEB500",300,500,14.5,28],
            ["Steel","HEB550",300,550,15,29],
            ["Steel","HEB600",300,600,15.5,30],
            ["Steel","HEB650",300,650,16,31],
            ["Steel","HEB700",300,700,17,32],
            ["Steel","HEB800",300,800,17.5,33],
            ["Steel","HEB900",300,900,18.5,35],
            ["Steel","HEB1000",300,1000,19,36],

            # HEM

            ["Steel","HEM160",166,180,14,23],
            ["Steel","HEM180",186,200,14.5,24],
            ["Steel","HEM200",206,220,15,25],
            ["Steel","HEM220",226,240,15.5,26],
            ["Steel","HEM240",248,270,18,32],
            ["Steel","HEM260",268,290,18,32.5],
            ["Steel","HEM280",288,310,18.5,33],
            ["Steel","HEM300",310,340,21,39],
            ["Steel","HEM320",309,359,21,40],
            ["Steel","HEM340",309,377,21,40],
            ["Steel","HEM360",308,395,21,40],
            ["Steel","HEM400",307,432,21,40],
            ["Steel","HEM450",307,478,21,40],
            ["Steel","HEM500",306,524,21,40],
            ["Steel","HEM550",306,572,21,40],
            ["Steel","HEM600",305,620,21,40],
            ["Steel","HEM650",305,668,21,40],
            ["Steel","HEM700",304,716,21,40],
            ["Steel","HEM800",303,814,21,40],
            ["Steel","HEM900",302,910,21,40],
            ["Steel","HEM1000",302,1008,21,40],

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

            ["Steel","IPE100",55,100,4.1,5.7],
            ["Steel","IPE120",64,120,4.4,6.3],
            ["Steel","IPE140",73,140,4.7,6.9],
            ["Steel","IPE160",82,160,5,7.4],
            ["Steel","IPE180",91,180,5.3,8],
            ["Steel","IPE200",100,200,5.6,8.5],
            ["Steel","IPE220",110,220,5.9,9.2],
            ["Steel","IPE240",120,240,6.2,9.8],
            ["Steel","IPE270",135,270,6.6,10.2],
            ["Steel","IPE300",150,300,7.1,10.7],
            ["Steel","IPE330",160,330,7.5,11.5],
            ["Steel","IPE360",170,360,8,12.7],
            ["Steel","IPE400",180,400,8.6,13.5],
            ["Steel","IPE450",190,450,9.4,14.6],
            ["Steel","IPE500",200,500,10.2,16],
            ["Steel","IPE550",210,550,11.1,17.2],
            ["Steel","IPE600",220,600,12,19],

            # IPEA

            ["Steel","IPEA100",55,98,3.6,4.7],
            ["Steel","IPEA120",64,118,3.8,5.1],
            ["Steel","IPEA140",73,138,3.8,5.6],
            ["Steel","IPEA160",82,157,4,5.9],
            ["Steel","IPEA180",91,177,4.3,6.5],
            ["Steel","IPEA200",100,197,4.5,7],
            ["Steel","IPEA220",110,217,5,7.7],
            ["Steel","IPEA240",120,237,5.2,8.3],
            ["Steel","IPEA270",135,267,5.5,8.7],
            ["Steel","IPEA300",150,297,6.1,9.2],
            ["Steel","IPEA330",160,327,6.5,10],
            ["Steel","IPEA360",170,357.6,6.6,11.5],
            ["Steel","IPEA400",180,397,7,12],
            ["Steel","IPEA450",190,447,7.6,13.1],
            ["Steel","IPEA500",200,497,8.4,14.5],
            ["Steel","IPEA550",210,547,9,15.7],
            ["Steel","IPEA600",220,597,9.8,17.5],

            # IPEO

            ["Steel","IPEO180",89,182,6.4,9.5],
            ["Steel","IPEO200",102,202,6.2,9.5],
            ["Steel","IPEO220",112,222,6.6,10.2],
            ["Steel","IPEO240",122,242,7,10.8],
            ["Steel","IPEO270",136,274,7.5,12.2],
            ["Steel","IPEO300",152,304,8,12.7],
            ["Steel","IPEO330",162,334,8.5,13.5],
            ["Steel","IPEO360",172,364,9.2,14.7],
            ["Steel","IPEO400",182,404,9.7,15.5],
            ["Steel","IPEO450",192,456,11,17.6],
            ["Steel","IPEO500",202,506,12,19],
            ["Steel","IPEO550",212,556,12.7,20.2],
            ["Steel","IPEO600",224,610,15,24],

            # IPER

            ["Steel","IPER140",72,142,5.3,7.8],
            ["Steel","IPER160",81,162,5.6,8.5],
            ["Steel","IPER180",92,183,6,9],
            ["Steel","IPER200",98,204,6.6,10.5],
            ["Steel","IPER220",108,225,6.7,11.8],
            ["Steel","IPER240",118,245,7.5,12.3],
            ["Steel","IPER270",133,276,7.1,13.1],
            ["Steel","IPER300",147,306,8.5,13.7],
            ["Steel","IPER330",158,336,9.2,14.5],
            ["Steel","IPER360",168,366,9.9,16],
            ["Steel","IPER400",178,407,10.6,17],
            ["Steel","IPER450",188,458,11.3,18.6],
            ["Steel","IPER500",198,508,12.6,20],
            ["Steel","IPER550",210,560,14,22.2],
            ["Steel","IPER600",218,608,14,23],

            # IPEV

            ["Steel","IPEV400",182,408,10.6,17.5],
            ["Steel","IPEV450",194,460,12.4,19.6],
            ["Steel","IPEV500",204,514,14.2,23],
            ["Steel","IPEV550",216,566,17.1,25.2],
            ["Steel","IPEV600",228,618,18,28],
            ["Steel","IPE750x137",263,753,11.5,17],
            ["Steel","IPE750x147",265,753,13.2,17],
            ["Steel","IPE750x161",266,758,13.8,19.3],
            ["Steel","IPE750x173",267,762,14.4,21.6],
            ["Steel","IPE750x185",267,766,14.9,23.6],
            ["Steel","IPE750x196",268,770,15.6,25.4],
            ["Steel","IPE750x210",268,775,16,28],
            ["Steel","IPE750x222",269,778,17,29.5]

            ]

def makeStructure(baseobj=None,length=None,width=None,height=None,name="Structure"):
    '''makeStructure([obj],[length],[width],[heigth],[swap]): creates a
    structure element based on the given profile object and the given
    extrusion height. If no base object is given, you can also specify
    length and width for a cubic object.'''
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    obj.Label = translate("Arch",name)
    _Structure(obj)
    if FreeCAD.GuiUp:
        _ViewProviderStructure(obj.ViewObject)
        obj.ViewObject.ShapeColor = ArchCommands.getDefaultColor("Structure")
    if baseobj:
        obj.Base = baseobj
        if FreeCAD.GuiUp:
            obj.Base.ViewObject.hide()
    if width:
        obj.Width = width
    else:
        obj.Width = p.GetFloat("StructureWidth",100)
    if height:
        obj.Height = height
    else:
        obj.Height = p.GetFloat("StructureHeight",1000)
    if length:
        obj.Length = length
    else:
        if not baseobj:
            # don't set the length if we have a base object, otherwise the lenght X height calc
            # gets wrong
            obj.Length = p.GetFloat("StructureLength",100)
    if height > length:
        obj.Role = "Column"
    return obj

def makeStructuralSystem(objects=[],axes=[],name="StructuralSystem"):
    '''makeStructuralSystem(objects,axes): makes a structural system
    based on the given objects and axes'''
    result = []
    if not axes:
        print "At least one axis must be given"
        return
    if objects:
        if not isinstance(objects,list):
            objects = [objects]
    else:
        objects = [None]
    for o in objects:
        obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
        obj.Label = translate("Arch",name)
        _StructuralSystem(obj)
        if FreeCAD.GuiUp:
            _ViewProviderStructuralSystem(obj.ViewObject)
        if o:
            obj.Base = o
        obj.Axes = axes
        result.append(obj)
        if FreeCAD.GuiUp and o:
            o.ViewObject.hide()
            Draft.formatObject(obj,o)
    FreeCAD.ActiveDocument.recompute()
    if len(result) == 1:
        return result[0]
    else:
        return result

def makeProfile(W=46,H=80,tw=3.8,tf=5.2,name="Profile"):
    '''makeProfile(W,H,tw,tf): returns a shape with one face describing
    the profile of a steel beam (IPE, IPN, HE, etc...) based on the following
    dimensions: W = total width, H = total height, tw = web thickness
    tw = flange thickness (see http://en.wikipedia.org/wiki/I-beam for
    reference)'''
    obj = FreeCAD.ActiveDocument.addObject("Part::Part2DObjectPython",name)
    obj.Label = translate("Arch",name)
    _Profile(obj)
    obj.Width = W
    obj.Height = H
    obj.WebThickness = tw
    obj.FlangeThickness = tf
    if FreeCAD.GuiUp:
        Draft._ViewProviderDraft(obj.ViewObject)
    return obj

class _CommandStructure:
    "the Arch Structure command definition"
    def GetResources(self):
        return {'Pixmap'  : 'Arch_Structure',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Structure"),
                'Accel': "S, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_Structure","Creates a structure object from scratch or from a selected object (sketch, wire, face or solid)")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch")
        self.Length = p.GetFloat("StructureLength",100)
        self.Width = p.GetFloat("StructureWidth",100)
        self.Height = p.GetFloat("StructureHeight",1000)
        self.Profile = 0
        self.continueCmd = False
        self.DECIMALS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt("Decimals",2)
        import DraftGui
        self.FORMAT = DraftGui.makeFormatSpec(self.DECIMALS,'Length')
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            st = Draft.getObjectsOfType(sel,"Structure")
            ax = Draft.getObjectsOfType(sel,"Axis")
            if ax:
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structural System")))
                FreeCADGui.addModule("Arch")
                if st:
                    FreeCADGui.doCommand("Arch.makeStructuralSystem(" + ArchCommands.getStringList(st) + "," + ArchCommands.getStringList(ax) + ")")
                else:
                    FreeCADGui.doCommand("Arch.makeStructuralSystem(axes=" + ArchCommands.getStringList(ax) + ")")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                return
            elif not(ax) and not(st):
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Structure")))
                FreeCADGui.addModule("Arch")
                for obj in sel:
                    FreeCADGui.doCommand("Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ")")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
                return

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
        FreeCADGui.addModule("Arch")
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
            FreeCADGui.doCommand('s.Profile = "'+pr[1]+'"')
        else:
            FreeCADGui.doCommand('s = Arch.makeStructure(length='+str(self.Length)+',width='+str(self.Width)+',height='+str(self.Height)+')')
        FreeCADGui.doCommand('s.Placement.Base = '+DraftVecUtils.toString(point))
        FreeCADGui.doCommand('s.Placement.Rotation=FreeCAD.DraftWorkingPlane.getRotation().Rotation')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        if self.continueCmd:
            self.Activated()

    def taskbox(self):
        "sets up a taskbox widget"
        w = QtGui.QWidget()
        ui = FreeCADGui.UiLoader()
        w.setWindowTitle(translate("Arch","Structure options").decode("utf8"))
        grid = QtGui.QGridLayout(w)

        # presets box
        labelp = QtGui.QLabel(translate("Arch","Preset").decode("utf8"))
        valuep = QtGui.QComboBox()
        fpresets = [" "]
        for p in Presets[1:]:
            fpresets.append(str(translate("Arch",p[0]))+" "+p[1]+" ("+str(p[2])+"x"+str(p[3])+"mm)")
        valuep.addItems(fpresets)
        grid.addWidget(labelp,0,0,1,1)
        grid.addWidget(valuep,0,1,1,1)

        # length
        label1 = QtGui.QLabel(translate("Arch","Length").decode("utf8"))
        self.vLength = ui.createWidget("Gui::InputField")
        self.vLength.setText(self.FORMAT % self.Length)
        grid.addWidget(label1,1,0,1,1)
        grid.addWidget(self.vLength,1,1,1,1)

        # width
        label2 = QtGui.QLabel(translate("Arch","Width").decode("utf8"))
        self.vWidth = ui.createWidget("Gui::InputField")
        self.vWidth.setText(self.FORMAT % self.Width)
        grid.addWidget(label2,2,0,1,1)
        grid.addWidget(self.vWidth,2,1,1,1)

        # height
        label3 = QtGui.QLabel(translate("Arch","Height").decode("utf8"))
        self.vHeight = ui.createWidget("Gui::InputField")
        self.vHeight.setText(self.FORMAT % self.Height)
        grid.addWidget(label3,3,0,1,1)
        grid.addWidget(self.vHeight,3,1,1,1)

        # horizontal button
        value5 = QtGui.QPushButton(translate("Arch","Rotate").decode("utf8"))
        grid.addWidget(value5,4,0,1,2)

        # continue button
        label4 = QtGui.QLabel(translate("Arch","Con&tinue").decode("utf8"))
        value4 = QtGui.QCheckBox()
        value4.setObjectName("ContinueCmd")
        value4.setLayoutDirection(QtCore.Qt.RightToLeft)
        label4.setBuddy(value4)
        if hasattr(FreeCADGui,"draftToolBar"):
            value4.setChecked(FreeCADGui.draftToolBar.continueMode)
            self.continueCmd = FreeCADGui.draftToolBar.continueMode
        grid.addWidget(label4,5,0,1,1)
        grid.addWidget(value4,5,1,1,1)

        QtCore.QObject.connect(valuep,QtCore.SIGNAL("currentIndexChanged(int)"),self.setPreset)
        QtCore.QObject.connect(self.vLength,QtCore.SIGNAL("valueChanged(double)"),self.setLength)
        QtCore.QObject.connect(self.vWidth,QtCore.SIGNAL("valueChanged(double)"),self.setWidth)
        QtCore.QObject.connect(self.vHeight,QtCore.SIGNAL("valueChanged(double)"),self.setHeight)
        QtCore.QObject.connect(value4,QtCore.SIGNAL("stateChanged(int)"),self.setContinue)
        QtCore.QObject.connect(value5,QtCore.SIGNAL("pressed()"),self.rotate)
        return w

    def update(self,point,info):
        "this function is called by the Snapper when the mouse is moved"
        if FreeCADGui.Control.activeDialog():
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
        if hasattr(FreeCADGui,"draftToolBar"):
            FreeCADGui.draftToolBar.continueMode = bool(i)

    def setPreset(self,i):
        if i > 0:
            self.vLength.setText(self.FORMAT % float(Presets[i][2]))
            self.vWidth.setText(self.FORMAT % float(Presets[i][3]))
        if len(Presets[i]) == 6:
            self.Profile = i
        else:
            self.Profile = 0

    def rotate(self):
        l = self.Length
        w = self.Width
        h = self.Height
        self.vLength.setText(self.FORMAT % h)
        self.vHeight.setText(self.FORMAT % w)
        self.vWidth.setText(self.FORMAT % l)

class _Structure(ArchComponent.Component):
    "The Structure object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLink","Tool","Arch",translate("Arch","An optional extrusion path for this element"))
        obj.addProperty("App::PropertyLength","Length","Arch",translate("Arch","The length of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Width","Arch",translate("Arch","The width of this element, if not based on a profile"))
        obj.addProperty("App::PropertyLength","Height","Arch",translate("Arch","The height or extrusion depth of this element. Keep 0 for automatic"))
        obj.addProperty("App::PropertyLinkList","Armatures","Arch",translate("Arch","Armatures contained in this element"))
        obj.addProperty("App::PropertyVector","Normal","Arch",translate("Arch","The normal extrusion direction of this object (keep (0,0,0) for automatic normal)"))
        obj.addProperty("App::PropertyVectorList","Nodes","Arch",translate("Arch","The structural nodes of this element"))
        obj.addProperty("App::PropertyString","Profile","Arch",translate("Arch","A description of the standard profile this element is based upon"))
        self.Type = "Structure"
        obj.Role = Roles

    def execute(self,obj):
        "creates the structure shape"

        import Part, DraftGeomUtils

        normal,length,width,height = self.getDefaultValues(obj)

        # creating base shape
        pl = obj.Placement
        base = None
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.isValid():
                    if not obj.Base.Shape.Solids:
                        # let pass invalid objects if they have solids...
                        return
                if hasattr(obj,"Tool"):
                    if obj.Tool:
                        try:
                            base = obj.Tool.Shape.copy().makePipe(obj.Base.Shape.copy())
                        except Part.OCCError:
                            FreeCAD.Console.PrintError(translate("Arch","Error: The base shape couldn't be extruded along this tool object"))
                            return
                if not base:
                    if not height:
                        return
                    if obj.Normal == Vector(0,0,0):
                        p = FreeCAD.Placement(obj.Base.Placement)
                        normal = p.Rotation.multVec(normal)
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
                        if sh.isClosed() and sh.isValid() and sh.Solids and (not sh.isNull()):
                            base = sh
                        else:
                            FreeCAD.Console.PrintWarning(str(translate("Arch","This mesh is an invalid solid")))
                            obj.Base.ViewObject.show()
        else:
            base = self.getProfiles(obj)
            if base:
                if length > height:
                    normal = normal.multiply(length)
                else:
                    normal = normal.multiply(height)
                base = Part.Face(base[0])
                base = base.extrude(normal)

        base = self.processSubShapes(obj,base,pl)
        self.applyShape(obj,base,pl)

    def onChanged(self,obj,prop):
        self.hideSubobjects(obj,prop)
        if prop == "Shape":
            if hasattr(obj,"Nodes"):
                # update structural nodes
                if obj.Nodes:
                    if hasattr(self,"nodes"):
                        if self.nodes:
                            if obj.Nodes != self.nodes:
                                # nodes are set manually: don't touch them
                                return
                    else:
                        # nodes haven't been calculated yet, but are set (file load)
                        # we calculate the nodes now but don't change the property
                        axis = self.getAxis(obj)
                        if axis:
                            self.nodes = [v.Point for v in axis.Vertexes]
                            return
                # we calculate and set the nodes
                axis = self.getAxis(obj)
                if axis:
                    self.nodes = [v.Point for v in axis.Vertexes]
                    obj.Nodes = self.nodes


class _ViewProviderStructure(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structure object"

    def __init__(self,vobj):
        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        vobj.addProperty("App::PropertyBool","ShowNodes","Arch","If the nodes are visible or not").ShowNodes = False
        vobj.addProperty("App::PropertyFloat","NodeLine","Base","The width of the nodes line")
        vobj.addProperty("App::PropertyFloat","NodeSize","Base","The size of the node points")
        vobj.addProperty("App::PropertyColor","NodeColor","Base","The color of the nodes line")
        vobj.NodeColor = (1.0,1.0,1.0,1.0)
        vobj.NodeSize = 6

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_Structure_Tree.svg"

    def updateData(self,obj,prop):
        if prop == "Nodes":
            if obj.Nodes:
                if hasattr(self,"nodes"):
                    p = []
                    for n in obj.Nodes:
                        p.append([n.x,n.y,n.z])
                    self.coords.point.setValues(0,len(p),p)
                    self.pointset.numPoints.setValue(len(p))
                    self.lineset.coordIndex.setValues(0,len(p)+1,range(len(p))+[-1])

    def onChanged(self,vobj,prop):
        if prop == "ShowNodes":
            if hasattr(self,"nodes"):
                vobj.Annotation.removeChild(self.nodes)
                del self.nodes
            if vobj.ShowNodes:
                from pivy import coin
                self.nodes = coin.SoAnnotation()
                self.coords = coin.SoCoordinate3()
                self.mat = coin.SoMaterial()
                self.pointstyle = coin.SoDrawStyle()
                self.pointstyle.style = coin.SoDrawStyle.POINTS
                self.pointset = coin.SoType.fromName("SoBrepPointSet").createInstance()
                self.linestyle = coin.SoDrawStyle()
                self.linestyle.style = coin.SoDrawStyle.LINES
                self.lineset = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
                self.nodes.addChild(self.coords)
                self.nodes.addChild(self.mat)
                self.nodes.addChild(self.pointstyle)
                self.nodes.addChild(self.pointset)
                self.nodes.addChild(self.linestyle)
                self.nodes.addChild(self.lineset)
                vobj.Annotation.addChild(self.nodes)
                self.updateData(vobj.Object,"Nodes")
                self.onChanged(vobj,"NodeColor")
                self.onChanged(vobj,"NodeLine")
                self.onChanged(vobj,"NodeSize")
        elif prop == "NodeColor":
            if hasattr(self,"mat"):
                l = vobj.NodeColor
                self.mat.diffuseColor.setValue([l[0],l[1],l[2]])
        elif prop == "NodeLine":
            if hasattr(self,"linestyle"):
                self.linestyle.lineWidth = vobj.NodeLine
        elif prop == "NodeSize":
            if hasattr(self,"pointstyle"):
                self.pointstyle.pointSize = vobj.NodeSize
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

class _Profile(Draft._DraftObject):
    "A parametric beam profile object"

    def __init__(self,obj):
        obj.addProperty("App::PropertyLength","Width","Draft","Width of the beam").Width = 10
        obj.addProperty("App::PropertyLength","Height","Draft","Height of the beam").Height = 30
        obj.addProperty("App::PropertyLength","WebThickness","Draft","Thickness of the webs").WebThickness = 3
        obj.addProperty("App::PropertyLength","FlangeThickness","Draft","Thickness of the flange").FlangeThickness = 2
        Draft._DraftObject.__init__(self,obj,"Profile")

    def execute(self,obj):
        import Part
        pl = obj.Placement
        p1 = Vector(-obj.Width.Value/2,-obj.Height.Value/2,0)
        p2 = Vector(obj.Width.Value/2,-obj.Height.Value/2,0)
        p3 = Vector(obj.Width.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p4 = Vector(obj.WebThickness.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p5 = Vector(obj.WebThickness.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p6 = Vector(obj.Width.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p7 = Vector(obj.Width.Value/2,obj.Height.Value/2,0)
        p8 = Vector(-obj.Width.Value/2,obj.Height.Value/2,0)
        p9 = Vector(-obj.Width.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p10 = Vector(-obj.WebThickness.Value/2,obj.Height.Value/2-obj.FlangeThickness.Value,0)
        p11 = Vector(-obj.WebThickness.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p12 = Vector(-obj.Width.Value/2,(-obj.Height.Value/2)+obj.FlangeThickness.Value,0)
        p = Part.makePolygon([p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p1])
        p = Part.Face(p)
        obj.Shape = p
        obj.Placement = pl

class _StructuralSystem(ArchComponent.Component):
    "The Structural System object"
    def __init__(self,obj):
        ArchComponent.Component.__init__(self,obj)
        obj.addProperty("App::PropertyLinkList","Axes","Arch",translate("Arch","Axes systems this structure is built on"))
        obj.addProperty("App::PropertyIntegerList","Exclude","Arch",translate("Arch","The element numbers to exclude when this structure is based on axes"))
        self.Type = "StructuralSystem"

    def execute(self,obj):
        "creates the structure shape"

        import Part, DraftGeomUtils

        # creating base shape
        pl = obj.Placement
        if obj.Base:
            if obj.Base.isDerivedFrom("Part::Feature"):
                if obj.Base.Shape.isNull():
                    return
                if not obj.Base.Shape.Solids:
                    return

                base = None

                # applying axes
                pts = self.getAxisPoints(obj)
                apl = self.getAxisPlacement(obj)

                if pts:
                    fsh = []
                    for i in range(len(pts)):
                        sh = obj.Base.Shape.copy()
                        if hasattr(obj,"Exclude"):
                            if i in obj.Exclude:
                                continue
                        if apl:
                            sh.Placement.Rotation = sh.Placement.Rotation.multiply(apl.Rotation)
                        sh.translate(pts[i])
                        fsh.append(sh)

                    if fsh:
                        base = Part.makeCompound(fsh)
                        base = self.processSubShapes(obj,base,pl)

                if base:
                    if not base.isNull():
                        if base.isValid() and base.Solids:
                            if base.Volume < 0:
                                base.reverse()
                            if base.Volume < 0:
                                FreeCAD.Console.PrintError(translate("Arch","Couldn't compute a shape"))
                                return
                            base = base.removeSplitter()
                            obj.Shape = base
                            if not pl.isNull():
                                obj.Placement = pl

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


class _ViewProviderStructuralSystem(ArchComponent.ViewProviderComponent):
    "A View Provider for the Structural System object"

    def getIcon(self):
        import Arch_rc
        return ":/icons/Arch_StructuralSystem_Tree.svg"


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_Structure',_CommandStructure())
