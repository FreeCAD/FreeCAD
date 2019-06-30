# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
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
#***************************************************************************


"""
OfflineRenderingUtils - Utilities to help producing files with colors from FreeCAD in non-GUI mode


Example usage:

The examples below extract the colors from an existing file, but you can also create your own
while working, as a simple dictionary of "objectName":(r,g,b) pairs (r, g, b as floats)

import os
import sys
import FreeCAD
import OfflineRenderingUtils

# build full filepaths

freecadFile = os.path.join(os.getcwd(),"testfile.FCStd")
baseFileName = os.path.splitext(freecadFile)[0]

# open FreeCAD file

doc = FreeCAD.open(freecadFile)

# build color dict

# setting nodiffuse=True (optional) discards per-face colors, and only sets one color per object
# only the STEP exporter accepts per-face colors. The others would consider the first color in the per-face colors list as
# the object color, which might be not what we want, so it's best to turn it off here.

colors = OfflineRenderingUtils.getColors(freecadFile,nodiffuse=True) 

# get the camera data from the file (used in some functions below)

camera = OfflineRenderingUtils.getCamera(freecadFile)

# export to OBJ

import importOBJ
importOBJ.export(doc.Objects,baseFileName+".obj",colors=colors)

# export to DAE

import importDAE
importDAE.export(doc.Objects,baseFileName+".dae",colors=colors)

# export to IFC

import importIFC
importIFC.export(doc.Objects,baseFileName+".ifc",colors=colors)

# export to STEP

# The STEP exporter accepts different kind of data than the above
# exporters. Use the function below to reformat it the proper way

stepdata = OfflineRenderingUtils.getStepData(doc.Objects,colors)
import Import
Import.export(stepdata,baseFileName+".stp")

# export to PNG

scene = OfflineRenderingUtils.buildScene(doc.Objects,colors)
OfflineRenderingUtils.render(baseFileName+".png",scene,camera,width=800,height=600)

# view the scene in a standalone coin viewer

OfflineRenderingUtils.viewer(scene)

# file saving

OfflineRenderingUtils.save(doc,filename=baseFileName+"_exported.FCStd",colors=colors,camera=camera)
"""


import sys
import os
import xml.sax
import zipfile
import tempfile
import inspect
from pivy import coin



class FreeCADGuiHandler(xml.sax.ContentHandler):

    "A XML handler to process the FreeCAD GUI xml data, used by getGuiData()"

    # this creates a dictionary where each key is a FC object name,
    # and each value is a dictionary of property:value pairs,
    # plus a GuiCameraSettings key that contains an iv repr of a coin camera

    def __init__(self):

        self.guidata = {}
        self.current = None
        self.properties = {}
        self.currentprop = None
        self.currentval = None

    # Call when an element starts

    def startElement(self, tag, attributes):

        if tag == "ViewProvider":
            self.current = attributes["name"]
        elif tag == "Property":
            name = attributes["name"]
            if name in ["Visibility","ShapeColor","Transparency","DiffuseColor"]:
                self.currentprop = name
        elif tag == "Bool":
            if attributes["value"] == "true":
                self.currentval = True
            else:
                self.currentval = False
        elif tag == "PropertyColor":
            c = int(attributes["value"])
            r = float((c>>24)&0xFF)/255.0
            g = float((c>>16)&0xFF)/255.0
            b = float((c>>8)&0xFF)/255.0
            self.currentval = (r,g,b)
        elif tag == "Integer":
            self.currentval = int(attributes["value"])
        elif tag == "Float":
            self.currentval = float(attributes["value"])
        elif tag == "ColorList":
            self.currentval = attributes["file"]
        elif tag == "Camera":
            self.guidata["GuiCameraSettings"] = attributes["settings"]

    # Call when an elements ends

    def endElement(self, tag):

        if tag == "ViewProvider":
            if self.current and self.properties:
                self.guidata[self.current] = self.properties
                self.current = None
                self.properties = {}
        elif tag == "Property":
            if self.currentprop and (self.currentval != None):
                self.properties[self.currentprop] = self.currentval
                self.currentprop = None
                self.currentval = None



def getGuiData(filename):

    """getGuiData(filename): Extract visual data from a saved FreeCAD file.
    Returns a dictionary ["objectName:dict] where dict contains properties
    keys  like ShapeColor, Transparency, DiffuseColor or Visibility. If found,
    also contains a GuiCameraSettings key with an iv repr of a coin camera"""

    guidata = {}
    zdoc = zipfile.ZipFile(filename)
    if zdoc:
        if "GuiDocument.xml" in zdoc.namelist():
            gf = zdoc.open("GuiDocument.xml")
            guidata = gf.read()
            gf.close()
            Handler = FreeCADGuiHandler()
            xml.sax.parseString(guidata, Handler)
            guidata = Handler.guidata
            for key,properties in guidata.items():
                # open each diffusecolor files and retrieve values
                # first 4 bytes are the array length, then each group of 4 bytes is abgr
                if "DiffuseColor" in properties:
                    #print ("opening:",guidata[key]["DiffuseColor"])
                    df = zdoc.open(guidata[key]["DiffuseColor"])
                    buf = df.read()
                    #print (buf," length ",len(buf))
                    df.close()
                    cols = []
                    for i in range(1,int(len(buf)/4)):
                        cols.append((buf[i*4+3]/255.0,buf[i*4+2]/255.0,buf[i*4+1]/255.0,buf[i*4]/255.0))
                    guidata[key]["DiffuseColor"] = cols
        zdoc.close()
        #print ("guidata:",guidata)
    return guidata



def getColors(filename,nodiffuse=False):

    """getColors(filename,nodiffuse): Extracts the colors saved in a FreeCAD file
    Returns a dictionary containing ["objectName":colors] pairs.
    colrs can be either a 3-element tuple representing an RGB color, if
    the object has no per-face colors (DiffuseColor) defined, or a list
    of tuples if per-face colors are available. In case of DiffuseColors,
    tuples can have 4 values (RGBT) (T = transparency, inverse of alpha)
    This is a reduced version of getGuiData(), which returns more information.
    If nodiffuse = True, DiffuseColor info is discarded, only ShapeColor is read."""

    data = getGuiData(filename)
    colors = {}
    for k,v in data.items():
        if ("DiffuseColor" in v) and (not nodiffuse):
            if len(v["DiffuseColor"]) == 1:
                # only one color in DiffuseColor: used for the whole object
                 colors[k] = v["DiffuseColor"][0]
            else:
                colors[k] = v["DiffuseColor"]
        elif "ShapeColor" in v:
            colors[k] = v["ShapeColor"]
    return colors



def getStepData(objects,colors):

    """getStepData(objects,colors): transforms the given list of objects and
    colors dictionary into a list of tuples acceptable by the STEP exporter of
    FreeCAD's Import module"""

    # The STEP exporter accepts two kinds of data: [obj1,obj2,...] list, or
    # [(obj1,DiffuseColor),(obj2,DiffuseColor),...] list.

    # we need to reformat a bit the data we send to the exporter
    # since, differently than the others, it wants (object,DiffuseColor) tuples
    data = []
    for obj in objects:
        if obj.Name in colors:
            color = colors[obj.Name]
            if isinstance(color,tuple):
                # this is a ShapeColor. We reformat as a list so it works as a DiffuseColor,
                # which is what the exporter expects. DiffuseColor can have either one color,
                # or the same number of colors as the number of faces
                color = [color]
            data.append((obj,color))
        else:
            # no color information. This object will be exported without colors
            data.append(obj)
    return data


def render(outputfile,scene=None,camera=None,zoom=False,width=400,height=300,background=(1.0,1.0,1.0)):

    """render(outputfile,scene=None,camera=None,zoom=False,width=400,height=300,background=(1.0,1.0,1.0)):
    Renders a PNG image of given width and height and background color from the given coin scene, using
    the given coin camera (ortho or perspective). If zoom is True the camera will be resized to fit all
    objects. The outputfile must be a file path to save a png image."""

    # On Linux, the X server must have indirect rendering enabled in order to be able to do offline
    # PNG rendering. Unfortunatley, this is turned off by default on most recent distros. The easiest
    # way I found is to edit (or create if inexistant) /etc/X11/xorg.conf and add this:
    #
    # Section "ServerFlags"
    #    Option "AllowIndirectGLX" "on"
    #    Option "IndirectGLX" "on"
    # EndSection
    #
    # But there are other ways, google of GLX indirect rendering

    if isinstance(camera,str):
        camera = getCoinCamera(camera)

    print("Starting offline renderer")
    # build an offline scene root separator
    root = coin.SoSeparator()
    # add one light (mandatory)
    light = coin.SoDirectionalLight()
    root.addChild(light)
    if not camera:
        # create a default camera if none was given
        camera = coin.SoPerspectiveCamera()
        cameraRotation = coin.SbRotation.identity()
        cameraRotation *= coin.SbRotation(coin.SbVec3f(1,0,0),-0.4)
        cameraRotation *= coin.SbRotation(coin.SbVec3f(0,1,0), 0.4)
        camera.orientation = cameraRotation
        # make sure all objects get in the view later
        zoom = True
    root.addChild(camera)
    if scene:
        root.addChild(scene)
    else:
        # no scene was given, add a simple cube
        cube = coin.SoCube()
        root.addChild(cube)
    vpRegion = coin.SbViewportRegion(width,height)
    if zoom:
        camera.viewAll(root,vpRegion)
    print("Creating viewport")
    offscreenRenderer = coin.SoOffscreenRenderer(vpRegion)
    offscreenRenderer.setBackgroundColor(coin.SbColor(background[0],background[1],background[2]))
    print("Ready to render")
    # ref ensures that the node will not be garbage-collected during rendering
    root.ref()
    ok = offscreenRenderer.render(root)
    root.unref()
    if ok:
        offscreenRenderer.writeToFile(outputfile,"PNG")
        print("Rendering",outputfile,"done")
    else:
        print("Error rendering image")



def buildScene(objects,colors=None):

    """buildScene(objects,colors=None): builds a coin node from a given list of FreeCAD
    objects. Optional colors argument can be a dicionary of objName:ShapeColorTuple
    or obj:DiffuseColorList pairs."""

    root = coin.SoSeparator()
    for o in objects:
        buf = None
        if o.isDerivedFrom("Part::Feature"):
            # writeInventor of shapes needs tessellation values
            buf = o.Shape.writeInventor(2,0.01)
        elif o.isDerivedFrom("Mesh::Feature"):
            buf = o.Mesh.writeInventor()
        if buf:
            # 3 lines below are the standard way to produce a coin node from a string
            # Part and Mesh objects always have a containing SoSeparator
            inp = coin.SoInput()
            inp.setBuffer(buf)
            node = coin.SoDB.readAll(inp)
            if colors:
                if o.Name in colors:
                    # insert a material node at 1st position, before the geometry
                    color = colors[o.Name]
                    if isinstance(color,list):
                        # DiffuseColor, not supported here
                        color = color[0]
                    mat = coin.SoMaterial()
                    mat.diffuseColor = color
                    node.insertChild(mat,0)
            root.addChild(node)
    return root



def getCamera(filepath):

    """getCamera(filepath): Returns a string representing a coin camera node from a given FreeCAD
    file, or None if none was found inside"""

    guidata = getGuiData(filepath)
    if "GuiCameraSettings" in guidata:
        return guidata["GuiCameraSettings"].strip()
    print("no camera found in file")
    return None



def getCoinCamera(camerastring):

    """getCoinCamera(camerastring): Returns a coin camera node from a string"""

    if camerastring:
        inp = coin.SoInput()
        inp.setBuffer(camerastring)
        node = coin.SoDB.readAll(inp)
        # this produces a SoSeparator containing the camera
        for child in node.getChildren():
            if ("SoOrthographicCamera" in str(child)) or ("SoPerspectiveCamera" in str(child)):
                return child
    print("unnable to build a camera node from string:",camerastring)
    return None



def viewer(scene=None,background=(1.0,1.0,1.0)):

    """viewer(scene=None,background=(1.0,1.0,1.0)): starts a standalone coin viewer with the contents of the given scene"""

    # Initialize Coin. This returns a main window to use
    from pivy import sogui
    win = sogui.SoGui.init()
    if win == None:
        print("Unable to create a SoGui window")
        return

    win.setBackgroundColor(coin.SbColor(background[0],background[1],background[2]))

    if not scene:
        # build a quick default scene
        mat = coin.SoMaterial()
        mat.diffuseColor = (1.0, 0.0, 0.0)
        # Make a scene containing a red cone
        scene = coin.SoSeparator()
        scene.addChild(mat)
        scene.addChild(coin.SoCone())

    # ref the scene so it doesn't get garbage-collected
    scene.ref()

    # Create a viewer in which to see our scene graph
    viewer = sogui.SoGuiExaminerViewer(win)

    # Put our scene into viewer, change the title
    viewer.setSceneGraph(scene)
    viewer.setTitle("Coin viewer")
    viewer.show()

    sogui.SoGui.show(win) # Display main window
    sogui.SoGui.mainLoop()     # Main Coin event loop



def save(document,filename=None,colors=None,camera=None):

    """save(document,filename=None,colors=None,camera=None): Saves the current document. If no filename 
       is given, the filename stored in the document (document.FileName) is used. A color dictionary of 
       objName:ShapeColorTuple or obj:DiffuseColorList pairs.can be provided, in that case the objects 
       will keep their colors when opened in the FreeCAD GUI. If given, camera is a string representing
       a coin camera node."""

    if filename:
        print("Saving as",filename)
        document.saveAs(filename)
    else:
        if document.FileName:
            filename = document.FileName
            document.save()
        else:
            print("Unable to save this document. Please provide a file name")
            return

    if colors:
        zf = zipfile.ZipFile(filename, mode='a')

        guidoc = buildGuiDocument(document,colors,camera)
        if guidoc:
            zf.write(guidoc,'GuiDocument.xml')
        zf.close()
        # delete the temp file
        os.remove(guidoc)



def getunsigned(color):

    """getunsigned(color): returns an unsigned int from a (r,g,b) color tuple"""

    if (color[0] <= 1) and (color[1] <= 1) and (color[2] <= 1):
        # 0->1 float colors, convert to 0->255
        color = (color[0]*255.0,color[1]*255.0,color[2]*255.0)

    # ensure evertything is int otherwise bit ops below dont work
    color = (int(color[0]),int(color[1]),int(color[2]))

    # https://forum.freecadweb.org/viewtopic.php?t=19074
    return str(color[0] << 24 | color[1] << 16 | color[2] << 8)



def buildGuiDocument(document,colors,camera=None):

    """buildGuiDocument(document,colors,camera=None): Returns the path to a temporary GuiDocument.xml for the given document.
    Colors is a color dictionary of objName:ShapeColorTuple or obj:DiffuseColorList. Camera, if given, is a string representing
    a coin camera. You must delete the temporary file after using it."""

    if not camera:
        camera = "OrthographicCamera {   viewportMapping ADJUST_CAMERA   position 0 -0 20000   orientation 0.0, 0.8939966636005564, 0.0, -0.44807361612917324   nearDistance 7561.228   farDistance 63175.688   aspectRatio 1   focalDistance 35368.102   height 2883.365  }"

    guidoc =  "<?xml version='1.0' encoding='utf-8'?>\n"
    guidoc += "<Document SchemaVersion=\"1\">\n"

    vps = [obj for obj in document.Objects if obj.Name in colors]
    if not vps:
        return None
    guidoc += "    <ViewProviderData Count=\""+str(len(vps))+"\">\n"
    for vp in vps:
        guidoc += "        <ViewProvider name=\""+vp.Name+"\" expanded=\"0\">\n"
        guidoc += "            <Properties Count=\"2\">\n"
        vpcol = colors[vp.Name]
        if isinstance(vpcol[0],tuple):
            # Diffuse colors not yet supported (need to create extra files...) for now simply use the first color...
            #guidoc += "                <Property name=\"DiffuseColor\" type=\"App::PropertyColorList\">\n"
            #guidoc += "                    <ColorList file=\"DiffuseColor\"/>\n"
            #guidoc += "                </Property>\n"
            guidoc += "                <Property name=\"ShapeColor\" type=\"App::PropertyColor\">\n"
            guidoc += "                    <PropertyColor value=\""+getunsigned(vpcol[0])+"\"/>\n"
            guidoc += "                </Property>\n"
        else:
            guidoc += "                <Property name=\"ShapeColor\" type=\"App::PropertyColor\">\n"
            guidoc += "                    <PropertyColor value=\""+getunsigned(vpcol)+"\"/>\n"
            guidoc += "                </Property>\n"
        guidoc += "                <Property name=\"Visibility\" type=\"App::PropertyBool\">\n"
        guidoc += "                    <Bool value=\"true\"/>\n"
        guidoc += "                </Property>\n"
        if hasattr(vp,"Proxy"):
            # if this is a python feature, store the view provider class if possible
            m = getViewProviderClass(vp)
            if m:
                guidoc += "                <Property name=\"Proxy\" type=\"App::PropertyPythonObject\">\n"
                guidoc += "                    <Python value=\"bnVsbA==\" encoded=\"yes\" module=\""+m[0]+"\" class=\""+m[1]+"\"/>\n"
                guidoc += "                </Property>\n"
        guidoc += "            </Properties>\n"
        guidoc += "        </ViewProvider>\n"

    guidoc +="    </ViewProviderData>\n"
    guidoc +="    <Camera settings=\"  " + camera + " \"/>\n"
    guidoc += "</Document>"

    # although the zipfile module has a writestr() function that should allow us to write the
    # string above directly to the zip file, I couldn't manage to make it work.. So we rather
    # use a temp file here, which works.
    
    #print(guidoc)

    tempxml = tempfile.mkstemp(suffix=".xml")[-1]
    f = open(tempxml,"w")
    f.write(guidoc)
    f.close()

    return tempxml


def getViewProviderClass(obj):

    """getViewProviderClass(obj): tries to identify the associated view provider for a 
       given python object. Returns a (modulename,classname) tuple if found, or None"""

    if not hasattr(obj,"Proxy"):
        return None
    if not obj.Proxy:
        return None
    mod = obj.Proxy.__module__
    objclass = obj.Proxy.__class__.__name__
    classes = []
    for name, mem in inspect.getmembers(sys.modules[mod]):
        if inspect.isclass(mem):
            classes.append(mem.__name__)
    # try to find a matching ViewProvider class
    if objclass.startswith("_"):
        wantedname = "_ViewProvider"+objclass[1:]
    else:
        wantedname = "ViewProvider"+objclass
    if wantedname in classes:
        #print("Found matching view provider for",mod,objclass,wantedname)
        return (mod,wantedname,)
    # use the default Draft VP if this is a Draft object
    if mod == "Draft":
        return(mod,"_ViewProviderDraft")
    print("Found no matching view provider for",mod,objclass)
    return None
