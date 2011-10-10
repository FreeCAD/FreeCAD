#   (c) Juergen Riegel (juergen.riegel@web.de) 2007      LGPL

import FreeCAD, os, sys, unittest, Mesh
import thread, time, tempfile


#---------------------------------------------------------------------------
# define the functions to test the FreeCAD mesh module
#---------------------------------------------------------------------------


class MeshTopoTestCases(unittest.TestCase):
	def setUp(self):
		# set up a planar face with 18 triangles
		self.planarMesh = []
		for x in range(3):
			for y in range(3):
				self.planarMesh.append( [0.0 + x, 0.0 + y,0.0000] ) 
				self.planarMesh.append( [1.0 + x, 1.0 + y,0.0000] )
				self.planarMesh.append( [0.0 + x, 1.0 + y,0.0000] )
				self.planarMesh.append( [0.0 + x, 0.0 + y,0.0000] )
				self.planarMesh.append( [1.0 + x, 0.0 + y,0.0000] )
				self.planarMesh.append( [1.0 + x, 1.0 + y,0.0000] )


	def testCollapseFacetsSingle(self):
		for i in range(18):
			planarMeshObject = Mesh.Mesh(self.planarMesh)
			planarMeshObject.collapseFacets([i])

	def testCollapseFacetsMultible(self):
		planarMeshObject = Mesh.Mesh(self.planarMesh)
		planarMeshObject.collapseFacets(range(7))

	def testCollapseFacetsAll(self):
		planarMeshObject = Mesh.Mesh(self.planarMesh)
		planarMeshObject.collapseFacets(range(18))


class MeshGeoTestCases(unittest.TestCase):
	def setUp(self):
		# set up a planar face with 2 triangles
		self.planarMesh = []


	def testIntersection(self):
		self.planarMesh.append( [0.9961,1.5413,4.3943] ) 
		self.planarMesh.append( [9.4796,10.024,-3.0937] )
		self.planarMesh.append( [1.4308,11.3841,2.6829] )
		self.planarMesh.append( [2.6493,2.2536,3.0679] )
		self.planarMesh.append( [13.1126,0.4857,-4.4417] )
		self.planarMesh.append( [10.2410,8.9040,-3.5002] )
		planarMeshObject = Mesh.Mesh(self.planarMesh)
		f1 = planarMeshObject.Facets[0]
		f2 = planarMeshObject.Facets[1]
		res=f1.intersect(f2)
		self.failUnless(len(res) == 0)


	def testIntersection2(self):
		self.planarMesh.append( [-16.097176,-29.891157,15.987688] ) 
		self.planarMesh.append( [-16.176304,-29.859991,15.947966] )
		self.planarMesh.append( [-16.071451,-29.900553,15.912505] )
		self.planarMesh.append( [-16.092241,-29.893408,16.020439] )
		self.planarMesh.append( [-16.007210,-29.926180,15.967641] )
		self.planarMesh.append( [-16.064457,-29.904951,16.090832] )
		planarMeshObject = Mesh.Mesh(self.planarMesh)
		f1 = planarMeshObject.Facets[0]
		f2 = planarMeshObject.Facets[1]
		# does definitely NOT intersect
		res=f1.intersect(f2)
		self.failUnless(len(res) == 0)

class PivyTestCases(unittest.TestCase):
	def setUp(self):
		# set up a planar face with 2 triangles
		self.planarMesh = []
		FreeCAD.newDocument("MeshTest")

	def testRayPick(self):
		if not FreeCAD.GuiUp:
			return
		self.planarMesh.append( [-16.097176,-29.891157,15.987688] ) 
		self.planarMesh.append( [-16.176304,-29.859991,15.947966] )
		self.planarMesh.append( [-16.071451,-29.900553,15.912505] )
		self.planarMesh.append( [-16.092241,-29.893408,16.020439] )
		self.planarMesh.append( [-16.007210,-29.926180,15.967641] )
		self.planarMesh.append( [-16.064457,-29.904951,16.090832] )
		planarMeshObject = Mesh.Mesh(self.planarMesh)

		from pivy import coin, sogui; import FreeCADGui
		if not sys.modules.has_key("pivy.gui.soqt"): from pivy.gui import soqt
		Mesh.show(planarMeshObject)
		view=FreeCADGui.ActiveDocument.ActiveView.getViewer()
		rp=coin.SoRayPickAction(view.getViewportRegion())
		rp.setRay(coin.SbVec3f(-16.05,16.0,16.0),coin.SbVec3f(0,-1,0))
		rp.apply(view.getSceneManager().getSceneGraph())
		pp=rp.getPickedPoint()
		self.failUnless(pp != None)
		det=pp.getDetail()
		self.failUnless(det.getTypeId() == coin.SoFaceDetail.getClassTypeId())
		det=coin.cast(det,str(det.getTypeId().getName()))
		self.failUnless(det.getFaceIndex() == 1)

	def testPrimitiveCount(self):
		if not FreeCAD.GuiUp:
			return
		self.planarMesh.append( [-16.097176,-29.891157,15.987688] ) 
		self.planarMesh.append( [-16.176304,-29.859991,15.947966] )
		self.planarMesh.append( [-16.071451,-29.900553,15.912505] )
		self.planarMesh.append( [-16.092241,-29.893408,16.020439] )
		self.planarMesh.append( [-16.007210,-29.926180,15.967641] )
		self.planarMesh.append( [-16.064457,-29.904951,16.090832] )
		planarMeshObject = Mesh.Mesh(self.planarMesh)

		from pivy import coin, sogui; import FreeCADGui
		if not sys.modules.has_key("pivy.gui.soqt"): from pivy.gui import soqt
		Mesh.show(planarMeshObject)
		view=FreeCADGui.ActiveDocument.ActiveView.getViewer()
		pc=coin.SoGetPrimitiveCountAction()
		pc.apply(view.getSceneGraph())
		self.failUnless(pc.getTriangleCount() == 2)
		#self.failUnless(pc.getPointCount() == 6)

	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("MeshTest")

# Threads

def loadFile(name):
    #lock.acquire()
    mesh=Mesh.Mesh()
    FreeCAD.Console.PrintMessage("Create mesh instance\n")
    #lock.release()
    mesh.read(name)
    FreeCAD.Console.PrintMessage("Mesh loaded successfully.\n")

def createMesh(r,s):
    FreeCAD.Console.PrintMessage("Create sphere (%s,%s)...\n"%(r,s))
    mesh=Mesh.createSphere(r,s)
    FreeCAD.Console.PrintMessage("... destroy sphere\n")

class LoadMeshInThreadsCases(unittest.TestCase):

    def setUp(self):
        pass

    def testSphereMesh(self):
        for i in range(6,8):
            thread.start_new(createMesh,(10.0,(i+1)*20))
        time.sleep(10)

    def testLoadMesh(self):
        mesh=Mesh.createSphere(10.0,100) # a fine sphere
        name=tempfile.gettempdir() + os.sep + "mesh.stl"
        mesh.write(name)
        FreeCAD.Console.PrintMessage("Write mesh to %s\n"%(name))
        #lock=thread.allocate_lock()
        for i in range(2):
            thread.start_new(loadFile,(name,))
        time.sleep(1)

    def tearDown(self):
        pass
