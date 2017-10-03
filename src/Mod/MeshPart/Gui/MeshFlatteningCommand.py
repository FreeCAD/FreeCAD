import FreeCADGui as gui
import Mesh
import FreeCAD as App
import FreeCADGui as gui

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
        return {'MenuText': 'Unwrap Mesh', 'ToolTip': 'find a flat representation of a mesh'}

    def Activated(self):
        import numpy as np
        import flatmesh
        import Part
        obj = gui.Selection.getSelection()[0] # obj must be a Mesh (Mesh-Design->Meshes->Create-Mesh)
        mesh = Mesh.Mesh(obj.Mesh) # copy of the mesh to set new vertices later on
        points = np.array([[i.x, i.y, i.z] for i in obj.Mesh.Points])
        faces = np.array([list(i) for i in  obj.Mesh.Topology[1]])
        print(faces)
        flattener = flatmesh.FaceUnwrapper(points, faces)
        flattener.findFlatNodes()
        boundaries = flattener.getFlatBoundaryNodes()
        wires = []
        for edge in boundaries:
            pi = Part.makePolygon([App.Vector(*node) for node in edge])
            Part.show(Part.Wire(pi))

    def IsActive(self):
        assert(super(CreateFlatMesh, self).IsActive())
        assert(isinstance(gui.Selection.getSelection()[0].Mesh, Mesh.Mesh))
        return True

class CreateFlatFace(BaseCommand):
    """create a flat face from a single face
       only full faces are supported right now"""
       
    def GetResources(self):
        return {'MenuText': 'Unwrap Face', 'ToolTip': 'find a flat representation of a mesh'}
    
    def IsActive(self):
        assert(super(CreateFlatMesh, self).IsActive())
        assert(isinstance(gui.Selection.getSelection()[0], Part.Face))
        return True
    
    

gui.addCommand('CreateFlatMesh', CreateFlatMesh())
