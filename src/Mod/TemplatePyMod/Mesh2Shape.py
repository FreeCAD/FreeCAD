# FreeCAD TemplatePyMod module
# (c) 2010 Werner Mayer LGPL


import Mesh,Part,MeshPart

faces = []
mesh = App.ActiveDocument.ActiveObject.Mesh
segments = mesh.getPlanarSegments(0.00001) # use rather strict tolerance here

for i in segments:
   if len(i) > 0:
      # a segment can have inner holes
      wires = MeshPart.wireFromSegment(mesh, i)
      # we assume that the exterior boundary is that one with the biggest bounding box
      if len(wires) > 0:
         ext=None
         max_length=0
         for i in wires:
            if i.BoundBox.DiagonalLength > max_length:
               max_length = i.BoundBox.DiagonalLength
               ext = i

         wires.remove(ext)
         # all interior wires mark a hole and must reverse their orientation, otherwise Part.Face fails
         for i in wires:
            i.reverse()

         # make sure that the exterior wires comes as first in the list
         wires.insert(0, ext)
         faces.append(Part.Face(wires))


shell=Part.Compound(faces)
Part.show(shell)
#solid = Part.Solid(Part.Shell(faces))
#Part.show(solid)

