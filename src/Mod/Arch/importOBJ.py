import FreeCAD
from draftlibs import fcgeo

pythonopen = open

def findVert(aVertex,aList):
    "finds aVertex in aList, returns index"
    for i in range(len(aList)):
        if (aVertex.X == aList[i].X) and (aVertex.Y == aList[i].Y) and (aVertex.Z == aList[i].Z):
            return i

def getIndices(shape,offset):
    "returns a list with 2 lists: vertices and face indexes, offsetted with the given amount"
    vlist = []
    flist = []
    for v in shape.Vertexes:
        vlist.append(" "+str(round(v.X,4))+" "+str(round(v.Z,4))+" "+str(round(v.Y,4)))
    for f in shape.Faces:
        fi = ""
        # OCC vertices are unsorted. We need to sort in the right order...
        edges = fcgeo.sortEdges(f.Wire.Edges)
        print edges
        for e in edges:
            print e.Vertexes[0].Point,e.Vertexes[1].Point
            v = e.Vertexes[0]
            fi+=" "+str(findVert(v,shape.Vertexes)+offset)
        flist.append(fi)
    return vlist,flist

def export(exportList,filename):
    "called when freecad exports a file"
    outfile = pythonopen(filename,"wb")
    ver = FreeCAD.Version()
    outfile.write("# FreeCAD v" + ver[0] + "." + ver[1] + " build" + ver[2] + " Arch module\n")
    outfile.write("# http://free-cad.sf.net\n")
    offset = 1
    for obj in exportList:
        if obj.isDerivedFrom("Part::Feature"):
            vlist,flist = getIndices(obj.Shape,offset)
            offset += len(vlist)
            outfile.write("o " + obj.Name + "\n")
            for v in vlist:
                outfile.write("v" + v + "\n")
            for f in flist:
                outfile.write("f" + f + "\n")
    outfile.close()
    FreeCAD.Console.PrintMessage("successfully written "+filename)
            
            
            
