# FreeCAD TemplatePyMod module  
# (c) 2007 Juergen Riegel LGPL

#


# import FreeCAD modules
import FreeCAD, FreeCADGui,inspect

# helper -------------------------------------------------------------------

def addCommand(name,cmdObject):
	(list,num) = inspect.getsourcelines(cmdObject.Activated)
	pos = 0
	# check for indentation
	while(list[1][pos] == ' ' or list[1][pos] == '\t'):
		pos += 1
	source = ""
	for i in range(len(list)-1):
		source += list[i+1][pos:]
	FreeCADGui.addCommand(name,cmdObject,source)
	

#---------------------------------------------------------------------------
# The command classes
#---------------------------------------------------------------------------

class TemplatePyMod_Cmd1:
    "Example command class"
    def Activated(self):
        print("TemplatePyMod_Cmd1 activated ;-) ")

    def GetResources(self):
        return {'Pixmap'  : 'Std_Tool1', 'MenuText': 'Example command', 'ToolTip': 'Very unimportand example command'}


class TemplatePyMod_Cmd2:
    "Example command class"
    def Activated(self):
        d = FreeCAD.ActiveDocument
        v = FreeCADGui.ActiveDocument.ActiveView
        class PolygonCreator:
            def __init__(self, doc, view, max):
                self.doc = doc
                self.view = view
                self.call = view.addEventCallback("SoMouseButtonEvent",self.addVertex)
                self.max = max
                self.node=[]
                self.count=0
                self.poly=None

            def addVertex(self, d):
                if (d["State"] == "DOWN"):
                    pos = d["Position"]
                    self.node.append(self.view.getPoint(pos[0],pos[1]))
                    self.count = self.count+1
                    if (self.count == 1):
                        import Part,PartGui
                        self.poly=self.doc.addObject("Part::Polygon","Polygon")
                        self.poly.Nodes = self.node
                        self.poly.Close=True
                    else:
                        self.poly.Nodes = self.node
                        self.doc.recompute()
                    if (self.count == self.max):
                        self.node=[]
                        self.view.removeEventCallback("SoMouseButtonEvent",self.call)

        self.polycreator = PolygonCreator(d,v,10)

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        else:
            return True

    def GetResources(self):
        return {'Pixmap'  : 'Std_Tool2', 'MenuText': 'Create polygon...', 'ToolTip': 'Create a polygon by clicking inside the viewer'}


class TemplatePyMod_Cmd3:
    "Import PySide"
    def Activated(self):
        import PythonQt
        from PySide import QtGui
        mw=FreeCADGui.getMainWindow()
        QtGui.QMessageBox.information(mw,"PySide","""PySide was loaded successfully.""")
        FreeCADGui.activateWorkbench("PythonQtWorkbench")

    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': 'Import PySide', 'ToolTip': 'Add a workbench for PySide samples'}

class SphereCreator:
	def __init__(self):
		import Part
		self.pt = Part
		self.mode = False
		FreeCAD.Console.PrintMessage("Create instance of SphereCreator\n")

	def __del__(self):
		FreeCAD.Console.PrintMessage("Delete instance of SphereCreator\n")

	def enter(self):
		if (self.mode):
			return
		FreeCAD.Console.PrintMessage("Enter sphere creation mode\n")
		self.av = FreeCADGui.ActiveDocument.ActiveView
		self.cb = self.av.addEventCallback("SoMouseButtonEvent",self.create)
		self.ex = self.av.addEventCallback("SoKeyboardEvent",self.exit)
		self.mode = True

	def leave(self):
		if (not self.mode):
			return
		FreeCAD.Console.PrintMessage("Leave sphere creation mode\n")
		self.av.removeEventCallback("SoMouseButtonEvent",self.cb)
		self.av.removeEventCallback("SoKeyboardEvent",self.ex)
		self.mode = False

	def create(self, info):
		down = (info["State"] == "DOWN")
		pos = info["Position"]
		if (down):
			pnt = self.av.getPoint(pos[0],pos[1])
			FreeCAD.Console.PrintMessage("Clicked on position: ("+str(pos[0])+", "+str(pos[0])+")")
			msg = " -> (%f,%f,%f)\n" % (pnt.x, pnt.y, pnt.z)
			FreeCAD.Console.PrintMessage(msg)
			sph=self.pt.makeSphere(1.0, pnt)
			self.pt.show(sph)

	def exit(self, info):
		esc = (info["Key"] == "ESCAPE")
		if (esc):
			self.leave()


class TemplatePyMod_Cmd4:
	def __init__(self):
		self.sc = SphereCreator()

	def __del__(self):
		FreeCAD.Console.PrintError('TemplatePyMod_Cmd4 was destroyed\n')

	def Activated(self):
		if FreeCADGui.ActiveDocument is not None:
			self.sc.enter()
		else:
			FreeCAD.Console.PrintWarning('A 3d view must be created\n')

	def GetResources(self):
		return {'Pixmap'  : 'python', 'MenuText': 'Create spheres...', 'ToolTip': 'Click on the screen to create a sphere'}


myRenderArea = None
class TemplatePyMod_Cmd5:
	"Example command class"
	def Activated(self):
		from pivy import sogui
		from pivy import coin

		global myRenderArea
		if myRenderArea is None:
			root = coin.SoSeparator()
			myCamera = coin.SoPerspectiveCamera()
			myMaterial = coin.SoMaterial()
			root.addChild(myCamera)
			root.addChild(coin.SoDirectionalLight())
			#myMaterial.diffuseColor = (1.0, 0.0, 0.0)   # Red
			root.addChild(myMaterial)
			root.addChild(coin.SoCone())

			# Create a renderArea in which to see our scene graph.
			# The render area will appear within the main window.
			myRenderArea = sogui.SoGuiRenderArea(FreeCADGui.getMainWindow())

			# Make myCamera see everything.
			myCamera.viewAll(root, myRenderArea.getViewportRegion())

			# Put our scene in myRenderArea, change the title
			myRenderArea.setSceneGraph(root)
			myRenderArea.setTitle("Hello Cone")
		myRenderArea.show()

	def GetResources(self):
		return {'Pixmap'  : 'Std_Tool1', 'MenuText': 'Render area', 'ToolTip': 'Show render area'}


class TemplatePyMod_Cmd6:
	def Activated(self):
		import FeaturePython
		FeaturePython.makeBox()

	def GetResources(self):
		return {'Pixmap'  : 'python', 'MenuText': 'Create a box', 'ToolTip': 'Use Box feature class which is completely written in Python'}

class TemplatePyGrp_1:
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("TemplatePyGrp_1\n")

    def GetResources(self):
        return {'Pixmap'  : 'Part_JoinConnect', 'MenuText': 'TemplatePyGrp_1', 'ToolTip': 'Print a message'}

class TemplatePyGrp_2:
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("TemplatePyGrp_2\n")

    def GetResources(self):
        return {'Pixmap'  : 'Part_JoinEmbed', 'MenuText': 'TemplatePyGrp_2', 'ToolTip': 'Print a message'}

class TemplatePyGrp_3:
    def Activated(self):
        import FreeCAD
        FreeCAD.Console.PrintMessage("TemplatePyGrp_3\n")

    def GetResources(self):
        return {'Pixmap'  : 'Part_JoinCutout', 'MenuText': 'TemplatePyGrp_3', 'ToolTip': 'Print a message'}

class TemplatePyGroup:
    "Example group command class"
    #def Activated(self, index):
    #    print "TemplatePyGroup activated ;-) "

    def GetCommands(self):
        return ("TemplatePyGrp_1", "TemplatePyGrp_2", "TemplatePyGrp_3", "Std_New")

    def GetDefaultCommand(self):
        return 2

    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': 'Group command', 'ToolTip': 'Example group command'}

class TemplatePyCheckable:
    "Example toggle command class"
    def Activated(self, index):
        if index == 0:
            print("Toggle is off")
        else:
            print("Toggle is on")

    def GetResources(self):
        return {'Pixmap'  : 'python', 'MenuText': 'Toggle command', 'ToolTip': 'Example toggle command', 'Checkable': True}

#---------------------------------------------------------------------------
# Adds the commands to the FreeCAD command manager
#---------------------------------------------------------------------------
addCommand('TemplatePyMod_Cmd1',TemplatePyMod_Cmd1())
addCommand('TemplatePyMod_Cmd2',TemplatePyMod_Cmd2())
addCommand('TemplatePyMod_Cmd3',TemplatePyMod_Cmd3())
FreeCADGui.addCommand('TemplatePyMod_Cmd4',TemplatePyMod_Cmd4())
FreeCADGui.addCommand('TemplatePyMod_Cmd5',TemplatePyMod_Cmd5())
FreeCADGui.addCommand('TemplatePyMod_Cmd6',TemplatePyMod_Cmd6())
FreeCADGui.addCommand('TemplatePyGrp_1',TemplatePyGrp_1())
FreeCADGui.addCommand('TemplatePyGrp_2',TemplatePyGrp_2())
FreeCADGui.addCommand('TemplatePyGrp_3',TemplatePyGrp_3())
FreeCADGui.addCommand('TemplatePyGroup',TemplatePyGroup())
FreeCADGui.addCommand('TemplatePyCheckable',TemplatePyCheckable())
