# (c) 2012 Werner Mayer LGPL

import FreeCAD, FreeCADGui
from pivy import coin

class Texture:
	def __init__(self, obj, source):
		"Add some custom properties to our box feature"
		obj.addProperty("App::PropertyLink","Source","Texture", "Link to the shape").Source = source
		obj.Proxy = self

	def onChanged(self, fp, prop):
		return

	def execute(self, fp):
		return

class ViewProviderTexture:
	def __init__(self, obj):
		obj.addProperty("App::PropertyPath","File","Texture", "File name to the texture resource")
		self.obj = obj
		obj.Proxy = self

	def onChanged(self, obj, prop):
		if prop == "File":
			self.tex.filename = str(obj.File)
		return

	def updateData(self, fp, prop):
		return

	def getDisplayModes(self,obj):
		''' Return a list of display modes. '''
		modes=["Texture"]
		return modes

	def attach(self, obj):
		self.grp = coin.SoGroup()
		self.tex = coin.SoTexture2()
		#self.env = coin.SoTextureCoordinateEnvironment()
 
		self.grp.addChild(self.tex)
		#self.grp.addChild(self.env)
		root = obj.Object.Source.ViewObject.RootNode
		self.grp.addChild(root)
		obj.addDisplayMode(self.grp,"Texture")
		# move the original node
		doc = obj.Object.Document
		doc = FreeCADGui.getDocument(doc.Name)
		graph = doc.ActiveView.getSceneGraph()
		graph.removeChild(root)

	def claimChildren(self):
		return [self.obj.Object.Source]

	def __getstate__(self):
		return None
 
	def __setstate__(self,state):
		return None

def makeTexture():
	FreeCAD.newDocument()
	box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")
	tex=FreeCAD.ActiveDocument.addObject("App::FeaturePython","Texture")
	Texture(tex, box)
	box.ViewObject.Selectable = False
	ViewProviderTexture(tex.ViewObject)
	box.touch()
	FreeCAD.ActiveDocument.recompute()
