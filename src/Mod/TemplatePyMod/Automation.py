# FreeCAD TemplatePyMod module  
# (c) 2010 Werner Mayer LGPL

"""
The module can be executed with:
FreeCAD -P <path_to_file> Automation.py
FreeCADCmd -P <path_to_file> Automation.py
"""

import FreeCAD, Part
import os
import tempfile

def makeSnapshotWithGui():
	from PySide import QtGui
	import FreeCADGui

	def getMainWindow():
		toplevel = QtGui.QApplication.topLevelWidgets()
		for i in toplevel:
			if i.metaObject().className() == "Gui::MainWindow":
				return i
		raise RuntimeError("No main window found")

	mw=getMainWindow()
	mw.hide()
	#mw.showMinimized()

	# Create a test geometry and add it to the document
	obj=Part.makeCone(10,8,10)
	doc = FreeCAD.newDocument()
	Part.show(obj)

	# switch off animation so that the camera is moved to the final position immediately
	view = FreeCADGui.getDocument(doc.Name).activeView()
	view.setAnimationEnabled(False)
	view.viewAxometric()
	view.fitAll()
	view.saveImage('crystal.png',800,600,'Current')
	FreeCAD.closeDocument(doc.Name)
	# close the application
	QtGui.QApplication.quit()

def makeSnapshotWithoutGui():
	from pivy import coin

	# create a test geometry and create an IV representation as string
	box=Part.makeCone(10,8,10)
	iv=box.writeInventor()

	# load it into a buffer
	inp=coin.SoInput()
	try:
		inp.setBuffer(iv)
	except:
		tempPath = tempfile.gettempdir()
		fileName = tempPath + os.sep + "cone.iv"
		file = open(fileName, "w")
		file.write(iv)
		file.close()
		inp.openFile(fileName)

	# and create a scenegraph
	data = coin.SoDB.readAll(inp)
	base = coin.SoBaseColor()
	base.rgb.setValue(0.6,0.7,1.0)
	data.insertChild(base,0)

	# add light and camera so that the rendered geometry is visible
	root = coin.SoSeparator()
	light = coin.SoDirectionalLight()
	cam = coin.SoOrthographicCamera()
	root.addChild(cam)
	root.addChild(light)
	root.addChild(data)

	# do the rendering now
	axo = coin.SbRotation(-0.353553, -0.146447, -0.353553, -0.853553)
	viewport=coin.SbViewportRegion(400,400)
	cam.orientation.setValue(axo)
	cam.viewAll(root,viewport)
	off=coin.SoOffscreenRenderer(viewport)
	root.ref()
	off.render(root)
	root.unref()

	# export the image, PS is always available
	off.writeToPostScript("crystal.ps")

	# Other formats are only available if simage package is installed
	if off.isWriteSupported("PNG"):
		print("Save as PNG")
		off.writeToFile("crystal.png","PNG")

if FreeCAD.GuiUp:
	makeSnapshotWithGui()
else:
	makeSnapshotWithoutGui()
