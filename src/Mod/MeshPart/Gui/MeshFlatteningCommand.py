import numpy as np
import Mesh
import FreeCAD as app
import flatMesh
import Part



class BaseCommand(object):
    def __init__(self):
        pass

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        else:
            return True

class CreateFlatMesh(BaseCommand):
    """create an involute gear"""

    def GetResources(self):
        return {'Pixmap': 'not_yet.svg', 'MenuText': 'unwrap mesh', 'ToolTip': 'find a flat representation of a mesh'}

    def Activated(self):
        obj = Gui.Selection.getSelection()[0] # obj must be a Mesh (Mesh-Design->Meshes->Create-Mesh)
        mesh = Mesh.Mesh(obj.Mesh) # copy of the mesh to set new vertices later on
        points = np.array([[i.x, i.y, i.z] for i in obj.Mesh.Points])
        faces = np.array([list(i) for i in  obj.Mesh.Topology[1]])
        flattener = flatMesh.FaceUnwrapper(points, faces)
        flattener.findFlatNodes()
        boundaries = flattener.getFlatBoundaryNodes()
        wires = []
        for edge in boundaries:
            pi = Part.makePolygon([app.Vector(*node) for node in edge])
            wires.append(pi)
        Part.show(Part.Wire(wires))


def initialize():
    Gui.addCommand('CreateFlatMesh', CreateFlatMesh())
    return ["CreateFlatMesh"]