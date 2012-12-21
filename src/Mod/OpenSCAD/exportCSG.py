
#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2012 Keith Sloan <keith@sloan-home.co.uk>               *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         * 
#*   Acknowledgements :                                                    *
#*                                                                         *
#*     Thanks to shoogen on the FreeCAD forum for programming advice       *
#*     and some code.                                                      *
#*                                                                         *
#***************************************************************************
__title__="FreeCAD OpenSCAD Workbench - CSG exporter Version 0.01c"
__author__ = "Keith Sloan <keith@sloan-home.co.uk>"
__url__ = ["http://www.sloan-home.co.uk/Export/Export.html"]

import FreeCAD, os, Part, math
from FreeCAD import Vector

try: import FreeCADGui
except ValueError: gui = False
else: gui = True

#***************************************************************************
# Tailor following to your requirements ( Should all be strings )          *
global fafs
#fafs = '$fa = 12, $fs = 2'
#convexity = 'convexity = 10'
params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/OpenSCAD")
fa = params.GetFloat('exportFa',12.0)
fs = params.GetFloat('exportFs',2.0)
conv = params.GetInt('exportConvexity',10)
fafs = '$fa = %f, $fs = %f' % (fa,fs)
convexity = 'convexity = %d' % conv
#***************************************************************************
if open.__module__ == '__builtin__':
        pythonopen = open

def check_center(ob):
    # Only say center = false if no rotation and no displacement
    if ob.Placement.isNull():
        return 'false'
    return 'true'

def center(b):
    if b == 0 :
        return 'false'
    return 'true'

def check_multmatrix(csg,ob,x,y,z):
    v = FreeCAD.Vector(0,0,1)
    b = FreeCAD.Vector(x,y,z)
    if ( ob.Placement.Base == FreeCAD.Vector(0,0,0)):
        return 0 # center = false no mm
    elif not ob.Placement.isNull():
        print "Output Multmatrix"
        m = ob.Placement.toMatrix()
        # adjust position for center displacments
        csg.write("multmatrix([["+str(m.A11)+", "+str(m.A12)+", "+str(m.A13)+", "+str(m.A14)+"], ["\
                                 +str(m.A21)+", "+str(m.A22)+", "+str(m.A23)+", "+str(m.A24)+"], ["\
                                 +str(m.A31)+", "+str(m.A32)+", "+str(m.A33)+", "+str(m.A34)+"], [ 0, 0, 0, 1]]){\n")          
        return 1 # center = true and mm
    return 2 # center = true and no mm            

def mesh2polyhedron(mesh):
    pointstr=','.join(['[%f,%f,%f]' % tuple(vec) for vec in mesh.Topology[0]])
    trianglestr=','.join(['[%d,%d,%d]' % tuple(tri) for tri in mesh.Topology[1]])
    return 'polyhedron ( points = [%s], triangles = [%s]);' % (pointstr,trianglestr)

def vector2d(v):
    return [v[0],v[1]]

def vertexs2polygon(vertex):
    pointstr=','.join(['[%f, %f]'  % tuple(vector2d(v.Point)) for v in vertex])
    return 'polygon ( points = [%s], paths = undef, convexity = 1);}' % pointstr

def shape2polyhedron(shape):
    import MeshPart
    fa = params.GetFloat('exportFa',12.0)
    return mesh2polyhedron(MeshPart.meshFromShape(shape,params.GetFloat(\
        'meshmaxlength',1.0), params.GetFloat('meshmaxarea',0.0),\
         params.GetFloat('meshlocallen',0.0),\
         params.GetFloat('meshdeflection',0.0)))
     
def process_object(csg,ob):
    
    print "Placement"
    print "Pos   : "+str(ob.Placement.Base)
    print "axis  : "+str(ob.Placement.Rotation.Axis)
    print "angle : "+str(ob.Placement.Rotation.Angle)
    
    if ob.Type == "Part::Sphere" :
        print "Sphere Radius : "+str(ob.Radius)
        check_multmatrix(csg,ob,0,0,0)
        csg.write("sphere($fn = 0, "+fafs+", r = "+str(ob.Radius)+");\n")
           
    elif ob.Type == "Part::Box" :
        print "cube : ("+ str(ob.Length)+","+str(ob.Width)+","+str(ob.Height)+")"
        mm = check_multmatrix(csg,ob,-ob.Length/2,-ob.Width/2,-ob.Height/2)        
        csg.write("cube (size = ["+str(ob.Length)+", "+str(ob.Width)+", "+str(ob.Height)+"], center = "+center(mm)+");\n")
        if mm == 1 : csg.write("}\n")       

    elif ob.Type == "Part::Cylinder" :
        print "cylinder : Height "+str(ob.Height)+ " Radius "+str(ob.Radius)        
        mm = check_multmatrix(csg,ob,0,0,-ob.Height/2)
        csg.write("cylinder($fn = 0, "+fafs+", h = "+str(ob.Height)+ ", r1 = "+str(ob.Radius)+\
                  ", r2 = " + str(ob.Radius) + ", center = "+center(mm)+");\n")
        if mm == 1 : csg.write("}\n")           
            
    elif ob.Type == "Part::Cone" :
        print "cone : Height "+str(ob.Height)+ " Radius1 "+str(ob.Radius1)+" Radius2 "+str(ob.Radius2)
        mm = check_multmatrix(csg,ob,0,0,-ob.Height/2)
        csg.write("cylinder($fn = 0, "+fafs+", h = "+str(ob.Height)+ ", r1 = "+str(ob.Radius1)+\
                  ", r2 = "+str(ob.Radius2)+", center = "+center(mm)+");\n")
        if mm == 1 : csg.write("}\n")

    elif ob.Type == "Part::Torus" :
        print "Torus"
        print ob.Radius1
        print ob.Radius2
        if ob.Angle3 == 360.00 :
            mm = check_multmatrix(csg,ob,0,0,0)
            csg.write("rotate_extrude("+convexity+", $fn = 0, "+fafs+")\n")
            csg.write("multmatrix([[1, 0, 0, "+str(ob.Radius1)+"], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]])\n")
            csg.write("circle($fn = 0, "+fafs+", r = "+str(ob.Radius2)+");\n")          
            if mm == 1 : csg.write("}\n")
        else : # Cannot convert to rotate extrude so best effort is polyhedron
            csg.write('%s\n' % shape2polyhedron(ob.Shape)) 

    elif ob.Type == "Part::Extrusion" :
        print "Extrusion"
        print ob.Base
        print ob.Base.Name
        #if ( ob.Base == "Part::FeaturePython" and ob.Base.Name == "Polygon") :
        if ob.Base.Name.startswith("Polygon") :
            f = str(ob.Base.FacesNumber)
            r = str(ob.Base.Radius)
            h = str(ob.Dir[2])
            print "Faces : " + f
            print "Radius : " + r
            print "Height : " + h
            mm = check_multmatrix(csg,ob,0,0,-float(h)/2)
            csg.write("cylinder($fn = "+f+", "+fafs+", h = "+h+", r1 = "+r+\
                      ", r2 = "+r+", center = "+center(mm)+");\n")
            if mm == 1: csg.write("}\n")

        elif ob.Base.Name.startswith("circle") :
            r = str(ob.Base.Radius)
            h = str(ob.Dir[2])
            print "Radius : " + r
            print "Height : " + h
            mm = check_multmatrix(csg,ob,0,0,-float(h)/2)
            csg.write("cylinder($fn = 0, "+fafs+", h = "+h+", r1 = "+r+\
                      ", r2 = "+r+", center = "+center(mm)+");\n")
            if mm == 1: csg.write("}\n")    
            
        elif ob.Base.Name.startswith("Wire") :
            print "Wire extrusion"
            print ob.Base
            mm = check_multmatrix(csg,ob,0,0,0)
            csg.write("linear_extrude(height = "+str(ob.Dir[2])+", center = "+center(mm)+", "+convexity+", twist = 0, slices = 2, $fn = 0, "+fafs+")\n{\n")
            csg.write(vertexs2polygon(ob.Base.Shape.Vertexes))
            if mm == 1: csg.write("}\n")

        elif ob.Base.Name.startswith("square") :
            mm = check_multmatrix(csg,ob,0,0,0)
            csg.write("linear_extrude(height = "+str(ob.Dir[2])+", center = true, "+convexity+", twist = 0, slices = 2, $fn = 0, "+fafs+")\n{\n")
            csg.write("square (size = ["+str(ob.Base.Length)+", "+str(ob.Base.Width)+"],center = "+center(mm)+";\n}\n")
            if mm == 1: csg.write("}\n")     
                      
    elif ob.Type == "Part::Cut" :
        print "Cut"
        csg.write("difference() {\n")
        process_object(csg,ob.Base)
        process_object(csg,ob.Tool)
        csg.write("}\n")

    elif ob.Type == "Part::Fuse" :
        print "union"
        csg.write("union() {\n")
        process_object(csg,ob.Base)
        process_object(csg,ob.Tool)
        csg.write("}\n")

    elif ob.Type == "Part::Common" :
        print "intersection"
        csg.write("intersection() {\n")
        process_object(csg,ob.Base)
        process_object(csg,ob.Tool)
        csg.write("}\n")

    elif ob.Type == "Part::MultiFuse" :
        print "Multi Fuse / union"
        csg.write("union() {\n")
        for subobj in ob.Shapes:
            process_object(csg,subobj)
        csg.write("}\n")
        
    elif ob.Type == "Part::Common" :
        print "Multi Common / intersection"
        csg.write("intersection() {\n")
        for subobj in ob.Shapes:
            process_object(csg,subobj)
        csg.write("}\n")

    elif ob.isDerivedFrom('Part::Feature') :
        print "Part::Feature"
        mm = check_multmatrix(csg,ob,0,0,0)
        csg.write('%s\n' % shape2polyhedron(ob.Shape))
        if mm == 1 : csg.write("}\n")    
                    


def export(exportList,filename):
    "called when freecad exports a file"
    
    # process Objects
    print "\nStart Export 0.1c\n"
    print "Open Output File"
    csg = pythonopen(filename,'w')
    print "Write Inital Output"
    # Not sure if comments as per scad are allowed in csg file              
    csg.write("// CSG file generated from FreeCAD Export 0.1c\n")
    #write initial group statements - not sure if required              
    csg.write("group() {\n group(){\n")
    for ob in exportList:
        print ob
        print "Name : "+ob.Name
        print "Type : "+ob.Type
        print "Shape : "
        print ob.Shape
        process_object(csg,ob)
   
    # write closing group braces
    csg.write("}\n}\n")
    # close file              
    csg.close()
    FreeCAD.Console.PrintMessage("successfully exported "+filename)
