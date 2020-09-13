import Mesh
import FreeCAD as App
import FreeCADGui as Gui
import Part
import MeshPartGui

class BaseCommand(object):
    def __init__(self):
        pass

    def IsActive(self):
        if App.ActiveDocument is None:
            return False
        else:
            return True


class CreateFlatMesh(BaseCommand):
    """create flat wires from a meshed face"""

    def GetResources(self):
        return {'Pixmap': 'MeshPart_Create_Flat_Mesh.svg', 'MenuText': 'Unwrap Mesh', 'ToolTip': 'find a flat representation of a mesh'}

    def Activated(self):
        import numpy as np
        import flatmesh
        obj = Gui.Selection.getSelection()[0] # obj must be a Mesh (Mesh-Design->Meshes->Create-Mesh)
        points = np.array([[i.x, i.y, i.z] for i in obj.Mesh.Points])
        faces = np.array([list(i) for i in  obj.Mesh.Topology[1]])
        flattener = flatmesh.FaceUnwrapper(points, faces)
        flattener.findFlatNodes(5, 0.95)
        boundaries = flattener.getFlatBoundaryNodes()
        #print('number of nodes: {}'.format(len(flattener.ze_nodes)))
        #print('number of faces: {}'.format(len(flattener.tris)))

        wires = []
        for edge in boundaries:
            pi = Part.makePolygon([App.Vector(*node) for node in edge])
            Part.show(Part.Wire(pi))

    def IsActive(self):
        assert(super(CreateFlatMesh, self).IsActive())
        assert(isinstance(Gui.Selection.getSelection()[0].Mesh, Mesh.Mesh))
        return True


class CreateFlatFace(BaseCommand):
    """create a flat face from a single face
       only full faces are supported right now"""
       
    def GetResources(self):
        return {'Pixmap': 'MeshPart_Create_Flat_Face.svg', 'MenuText': 'Unwrap Face', 'ToolTip': 'find a flat representation of a mesh'}
    
    def Activated(self):
        import numpy as np
        import flatmesh
        face = Gui.Selection.getSelectionEx()[0].SubObjects[0]
        shape = face.toNurbs()
        face = shape.Faces[0]
        nurbs = face.Surface
        nurbs.setUNotPeriodic()
        nurbs.setVNotPeriodic()
        bs = nurbs.toBSpline(1, "C0", "C0", 3, 3, 10)
        face = bs.toShape()
        face.tessellate(0.01)
        flattener = flatmesh.FaceUnwrapper(face)
        flattener.findFlatNodes(5, 0.99)
        poles = flattener.interpolateFlatFace(face)
        num_u_poles = len(bs.getPoles())
        num_v_poles = len(bs.getPoles()[0])
        i = 0
        for u in range(num_u_poles):
            for v in range(num_v_poles):
                bs.setPole(u + 1, v + 1, App.Vector(poles[i]))
                i += 1
        Part.show(bs.toShape())
        
    def IsActive(self):
        assert(super(CreateFlatFace, self).IsActive())
        assert(isinstance(Gui.Selection.getSelectionEx()[0].SubObjects[0], Part.Face))
        return True

try:
    import flatmesh
    Gui.addCommand('MeshPart_CreateFlatMesh', CreateFlatMesh())
    Gui.addCommand('MeshPart_CreateFlatFace', CreateFlatFace())
except ImportError:
    App.Console.PrintLog("flatmesh-commands are not available\n")
    App.Console.PrintLog("flatmesh needs pybind11 as build dependency\n")
