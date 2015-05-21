#! python
# -*- coding: utf-8 -*-

__title__="FreeCAD Computatilnal elctrodynamics Workbench"
__author__ = "Robert Rehammar"
__url__ = "http://www.freecadweb.org"

'''
General description:

    The CEM module is a FreeCAD module for doing computatilnal electrodynamics.
    It currently works by providing a new export format to the MEEP ctl file
    format used med MIT MEEP FDTD solver.

User manual:

    http://www.freecadweb.org/FIXME
    http://ab-initio.mit.edu/wiki/index.php/Meep

'''

import FreeCAD
from datetime import datetime


# Import the special CEM features:
#import PML
#import Source
import lattice
#import Flux_region
#import Far_field


def export(objectslist,filename,nospline=False,lwPoly=False):
    "called when freecad exports a file. If nospline=True, bsplines are exported as straight segs lwPoly=True for OpenSCAD DXF"
    file = open(filename, "w")
    file.write("""; Exported ctl-file from FreeCAD version %s.%s.%s.
; This FreeCAD module was written by Robert Rehammar, robert@rehammar.se
; File generated %s.
; File units are FIXME

"""%(FreeCAD.Version()[0], FreeCAD.Version()[1], FreeCAD.Version()[2], datetime.now().strftime('%c')))
    
    
    # Export the geometry:
    file.write("""
; Exported geometry starts here: 

(set! geometry
    (list
""")
    for ob in objectslist:
        if ob.isDerivedFrom("Part::Sphere"):
            print_sphere(file, ob)
        elif ob.isDerivedFrom("Part::Box"):
            print_block(file, ob)
        elif ob.isDerivedFrom("Part::Cylinder"):
            #print_cylinder(file, ob)
            pass
    file.write("""
    ) ; list
) ; set! gemoetry
; Exported geometry ends here.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
""")
    # Export the PML-layers:
    # And so on...
    
    file.close()

def print_sphere(file, so):
    """
    Prints a sphere object to file, assumes so is a sphere object.
    """
    file.write("        ; Sphere: "+so.Label+"\n")
    file.write("""        (make
            sphere
            (center %f %f %f)
            (radius %f)
        )\n"""%(so.Placement.Base[0],so.Placement.Base[1],so.Placement.Base[2],so.Radius))

def print_block(file, so):
    """
    Prints a box object to file, assumes so is a box object.
    A MEEP block is the same thing as a Part::Box in FreeCAD.
    All MEEP objects have the center coordinate specified, but FreeCAD gives the
    lower right corner, so needs to compute.
    """
    x = float(so.Placement.Base[0]) + float(so.Length)/2.0
    y = float(so.Placement.Base[1]) + float(so.Width)/2.0
    z = float(so.Placement.Base[2]) + float(so.Height)/2.0
    file.write("        ; Block: "+so.Label+"\n")
    file.write("""        (make
            block
            (center %f %f %f)
            (size %f %f %f)
        )\n"""%(x, y, z,so.Length,so.Width,so.Height))

def print_cylinder(file, so):
    """
    Prints a cylinder object to file, assumes so is a cylinder object.
    """
    file.write("        ; Cylinder: "+so.Label+"\n")
    if so.Angle != 360:
        file.write("; WARNING: Angle != 360 deg property is not supported in MEEP, and thus not used!")
    file.write("""        (make
            cylinder
            (center %f %f %f)
            (axis %f %f %f)
            (radius %f)
            (height %f)
        )\n"""%(so.Placement.Base[0],so.Placement.Base[1],so.Placement.Base[2],so.Radius,so.Height))





