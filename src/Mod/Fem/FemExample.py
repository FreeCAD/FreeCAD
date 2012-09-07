# Example hwo to use the basic fem classes. The Fem module is  dependend on Part but nor on other Modules.
# So we need only:
from Fem import *
from Part import *
from FreeCAD import *

m = FemMesh()
m.NodeCount

b = makeBox(10,10,10)
m.setShape(b)

m.NodeCount