#!/usr/bin/env python
# coding: utf-8

###
# Copyright (c) 2005 Øystein Handegard <handegar@sim.no>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

##
## A simple Inventor-2-POV converter  
##
## TODO:
##  * Handle transformation of light position
##  * Handle textures
##  * Better lightsource converter (esp. for spotlights)
##  * Native support for POV primitives (Spheres/Cones/Cylinder)
##  * Better camera support
##  * Search graph for lights or cameras BEFORE processing. Add
##    if missing.
##  * Better material handling
##  * Make it into a library?
##


from __future__ import print_function
from pivy.coin import *
from pivy.sogui import *

##############################################################

lightfound = False

##############################################################
    
def printMaterial(action, idx=0):
    (ambient, diffuse, specular, emissive, shininess, transparency) = action.getMaterial(idx)
    print(" texture { // Material")
    if transparency != 0:
        print("  pigment { color rgbt <%f, %f, %f, %f> }" % (diffuse[0], diffuse[1], diffuse[2], transparency))
    else:
        print("  pigment { color rgb <%f, %f, %f> }" % (diffuse[0], diffuse[1], diffuse[2]))
    ambientfactor = SbVec3f(ambient[0], ambient[1], ambient[2]).length() / 3.0
    diffusefactor = SbVec3f(specular[0], specular[1], specular[2]).length() / 3.0                                
    print("  finish { diffuse 0.8 ambient %f specular %f reflection %f }" % (ambientfactor, diffusefactor, shininess))
    print(" }")

##############################################################

def cameraCallback(userdata, action, camera):
    print("\ncamera {")
    print(" perspective")
    print(" up <0, 1, 0>")
    ## FIXME: This is not really needed (?)  (20050819 handegar)
    ##print " right <%f, 0, 0>" % camera.aspectRatio.getValue()
    print(" direction <0, 0, -1>")
    campos = camera.position.getValue()
    print(" location <%f, %f, %f>" % (campos[0], campos[1], campos[2]))
    print(" angle %f" % (camera.heightAngle.getValue() * (360.0 / (2*M_PI))))                      
    lookat = camera.getViewVolume().getSightPoint(10)
    print(" look_at <%f, %f, %f>" % (lookat[0], lookat[1], lookat[2]))
    print("}\n")



def lightCallback(userdata, action, light):
    global lightfound
    lightfound = True    
    print("\nlight_source {")
    position = SbVec3f()
    if light.isOfType(SoDirectionalLight.getClassTypeId()) :
        position = light.direction.getValue() ## Not exactly correct    
    else :
        position = light.location.getValue()
    print(" <%f, %f, %f>" % (position[0], position[1], position[2]))
    color = light.color.getValue()
    print(" rgb <%f, %f, %f>" % (color[0], color[1], color[2]))
    if light.isOfType(SoDirectionalLight.getClassTypeId()) :
        print(" parallel")
        print(" point_at <0, 0, 0>") ## I'm not sure if this is correct (but it looks OK)
    if light.isOfType(SoSpotLight.getClassTypeId()):
        target = position + light.direction.getValue()
        print(" spotlight")
        print(" radius %f" % (2*(light.cutOffAngle.getValue() * 360) / (2*M_PI)))
        print(" point_at <%f, %f, %f>" % (target[0], target[1], target[2]))        
    print("}\n")



def preShapeCallback(userdata, action, node):
    print("\n// Mesh start")
    if node.getName().getString() != "":
        print("// name: '%s'" % node.getName().getString())    
    print("mesh {")
    return SoCallbackAction.CONTINUE



def postShapeCallback(userdata, action, node):
    printMaterial(action)
    print("} // Mesh end\n")
    return SoCallbackAction.CONTINUE



def triangleCallback(userdata, action, v1, v2, v3):
    matrix = action.getModelMatrix()
    revmatrix = matrix.inverse().transpose()
    p1 = matrix.multVecMatrix(v1.getPoint())
    p2 = matrix.multVecMatrix(v2.getPoint())
    p3 = matrix.multVecMatrix(v3.getPoint())
    n1 = revmatrix.multVecMatrix(v1.getNormal())
    n2 = revmatrix.multVecMatrix(v2.getNormal())
    n3 = revmatrix.multVecMatrix(v3.getNormal())

    ## FIXME: There must be a better way to detect if normals
    ## are invalid than this (20050819 handegar)
    if n1[0] < 2.0: ## Substitute for +NaN as the normal is always normalized
        print(" smooth_triangle {")
        print("  <%f, %f, %f>,<%f, %f, %f>, <%f, %f, %f>,<%f, %f, %f>, <%f, %f, %f>,<%f, %f, %f>" % \
              (p1[0], p1[1], p1[2], n1[0], n1[1], n1[2], p2[0], p2[1], p2[2], n2[0], n2[1], n2[2], p3[0], p3[1], p3[2], n3[0], n3[1], n3[2]))
    else:
        print(" triangle {")
        print("  <%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>" % (p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], p3[0], p3[1], p3[2]))
    if (action.getMaterialBinding() == SoMaterialBinding.PER_FACE) or \
       (action.getMaterialBinding() == SoMaterialBinding.PER_FACE_INDEXED):
        print("  texture { T%d }" % (v1.getMaterialIndex()))
    print(" }") 
    return SoCallbackAction.CONTINUE



def materialCallback(userdata, action, node):
    print("// Material declarations")
    nr = node.diffuseColor.getNum()    
    for i in range(0, nr):
        d = node.diffuseColor[i]
        ambientfactor = SbVec3f(node.ambientColor[i][0], node.ambientColor[i][1], node.ambientColor[i][2]).length() / 3.0
        specularfactor = SbVec3f(node.specularColor[i][0], node.specularColor[i][1], node.specularColor[i][2]).length() / 3.0
        transparency = node.transparency[i]
        shininess = node.shininess[i]

        if node.ambientColor.getNum() < nr: ambientfactor = 0.2
        if node.specularColor.getNum() < nr: specularfactor = 0
        if node.transparency.getNum() < nr: transparency = 0
        if node.shininess.getNum() < nr: shininess = 0

        print("#declare T%d=" % (i))
        print("texture { pigment { color rgbt <%f, %f, %f, %f> }" % (d[0], d[1], d[2], transparency))
        print("          finish { diffuse 0.8 ambient %f specular %f reflection %f } }" \
              % (ambientfactor, specularfactor, shininess))
        print("// end")



def convert(root):
    print("""
/*
   iv2pov.py  -  An Inventor to Persistence of Vision converter
   Version 0.01 alpha      
   Øystein Handegard, <handegar@sim.no>
*/""")
    callbackAction = SoCallbackAction()
    callbackAction.addPreCallback(SoPerspectiveCamera.getClassTypeId(), cameraCallback, None)
    callbackAction.addPreCallback(SoMaterial.getClassTypeId(), materialCallback, None)    
    callbackAction.addPreCallback(SoLight.getClassTypeId(), lightCallback, None)
    callbackAction.addPreCallback(SoShape.getClassTypeId(), preShapeCallback, None)
    callbackAction.addPostCallback(SoShape.getClassTypeId(), postShapeCallback, None)
    callbackAction.addTriangleCallback(SoShape.getClassTypeId(), triangleCallback, None)
    callbackAction.apply(root)
    
##############################################################

def main():
    myWindow = SoGui.init(sys.argv[0])
    if myWindow == None: sys.exit(1)

    sys.stderr.write("Inventor => POV converter - Version 0.01 alpha\n")
    if len(sys.argv) != 2:
        sys.stderr.write(" Usage: iv2pov.py [iv file] > out.pov\n")
        sys.exit(1)
        
    myInput = SoInput()
    if not myInput.openFile(sys.argv[1]):
        sys.stderr.write(" Could not open specified file.\n")
        sys.exit(1)
    root = SoDB.readAll(myInput)

    sys.stderr.write("* Select a camera angle and press 'q'\n")
    
    # setup viewer
    myViewer = SoGuiExaminerViewer(myWindow)
    myViewer.setSceneGraph(root)
    myViewer.setTitle("Inventor to POV converter")
    myViewer.viewAll()
    myViewer.show()

    SoGui.show(myWindow)
    SoGui.mainLoop()

    sys.stderr.write("* Processing...\n")
    cam = myViewer.getCamera()
    root.insertChild(cam, 0)
    convert(root)
    sys.stderr.write("* ...finished\n")


    # Add a default headlight if no light were processed (or else
    # the scene gets completely dark...)
    global lightfound    
    if lightfound != True:
        sys.stderr.write("* Scene contained no lights. Added a headlight to the camera.\n")
        pos = cam.position.getValue()
        print("// Default headlight")
        print("light_source { <%f, %f, %f>, rgb <1, 1, 1> }" % (pos[0], pos[1], pos[2]))
        
    return 0

if __name__ == "__main__":
    sys.exit(main())
