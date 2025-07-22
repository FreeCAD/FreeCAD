# FreeCAD TemplatePyMod module
# (c) 2013 Werner Mayer LGPL

# http://de.wikipedia.org/wiki/Non-Uniform_Rational_B-Spline
# len(knot_u) := nNodes_u + degree_u + 1
# len(knot_v) := nNodes_v + degree_v + 1

import FreeCAD

degree_u=2
degree_v=2
nNodes_u=5
nNodes_v=5

#knot_u=[0,0,0,0.3333,0.6666,1,1,1]
#knot_v=[0,0,0,0.3333,0.6666,1,1,1]
knot_u=[0,0,0,0.2,0.7,1,1,1]
knot_v=[0,0,0,0.2,0.7,1,1,1]
#knot_u=[0,0,0.2,0.4,0.6,0.8,1,1]
#knot_v=[0,0,0.2,0.4,0.6,0.8,1,1]
coor=[[0,0,1],[1,0,2],[2,0,0],[3,0,1],[4,0,2],\
      [0,1,2],[1,1,0],[2,1,0],[3,1,0],[4,1,0],\
      [0,2,0],[1,2,0],[2,2,0],[3,2,0],[4,2,0],\
      [0,3,1],[1,3,0],[2,3,0],[3,3,3],[4,3,0],\
      [0,4,2],[1,4,0],[2,4,0],[3,4,0],[4,4,0]]

bs=Part.BSplineSurface()
bs.increaseDegree(degree_u,degree_v)

id=1
for i in range(0,len(knot_u)-1):
        if knot_u[i+1] > knot_u[i]:
                bs.insertUKnot(knot_u[i],id,0.0000001)

id=1
for i in range(0,len(knot_v)-1):
        if knot_v[i+1] > knot_v[i]:
                bs.insertVKnot(knot_v[i],id,0.0000001)

i=0
for jj in range(0,nNodes_v):
        for ii in range(0,nNodes_u):
                bs.setPole(ii+1,jj+1,FreeCAD.Vector((coor[i][0],coor[i][1],coor[i][2])),1);
                i=i+1;

s=bs.toShape()
Part.show(s)
