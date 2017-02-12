#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *  
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

import FreeCAD, Material
import os

__title__="FreeCAD material card importer"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

# file structure - this affects how files are saved
FileStructure = [
    [ "Meta",            ["CardName","AuthorAndLicense","Source"] ],
    [ "General",         ["Name","Father","Description","Density","Vendor","ProductURL","SpecificPrice"] ],
    [ "Mechanical",      ["YoungsModulus","UltimateTensileStrength","CompressiveStrength","Elasticity","FractureToughness"] ],
    [ "FEM",             ["PoissonRatio"] ],
    [ "Architectural",   ["Model","ExecutionInstructions","FireResistanceClass","StandardCode","ThermalConductivity","SoundTransmissionClass","Color","Finish","UnitsPerQuantity","EnvironmentalEfficiencyClass"] ],
    [ "Rendering",       ["DiffuseColor","AmbientColor","SpecularColor","Shininess","EmissiveColor","Transparency","VertexShader","FragmentShader","TexturePath","TextureScaling"] ],
    [ "Vector rendering",["ViewColor","ViewFillPattern","SectionFillPattern","ViewLinewidth","SectionLinewidth"] ],
    [ "User defined",    [] ]
]

# to distinguish python built-in open function from the one declared below
if open.__module__ == '__builtin__':
    pythonopen = open

def open(filename):
    "called when freecad wants to open a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc

def insert(filename,docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc
    
def export(exportList,filename):
    "called when freecad exports a file"
    return

def decode(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            FreeCAD.Console.PrintError("Error: Couldn't determine character encoding")
            decodedName = name
    return decodedName

def read(filename):
    "reads a FCMat file and returns a dictionary from it"
    if isinstance(filename,unicode): 
        import sys
        filename = filename.encode(sys.getfilesystemencoding())
    f = pythonopen(filename)
    d = {}
    l = 0
    for line in f:
        if l == 0:
            d["CardName"] = line.split(";")[1].strip()
        elif l == 1:
            d["AuthorAndLicense"] = line.split(";")[1].strip()
        else:
            if not line[0] in ";#[":
                k = line.split("=")
                if len(k) == 2:
                    d[k[0].strip()] = k[1].strip().decode('utf-8')
        l += 1
    return d
    
def write(filename,dictionary):
    "writes the given dictionary to the given file"
    # sort the data into sections
    contents = []
    for key in FileStructure:
        contents.append({"keyname":key[0]})
        if key[0] == "Meta":
            header = contents[-1]
        elif key[0] == "User defined":
            user = contents[-1]
        for p in key[1]:
            contents[-1][p] = ""
    for k,i in dictionary.iteritems():
        found = False
        for group in contents:
            if not found:
                if k in group.keys():
                    group[k] = i
                    found = True
        if not found:
            user[k] = i
    # write header
    rev = FreeCAD.ConfigGet("BuildVersionMajor")+"."+FreeCAD.ConfigGet("BuildVersionMinor")+" "+FreeCAD.ConfigGet("BuildRevision")
    filename = filename[0]
    if isinstance(filename,unicode): 
        import sys
        filename = filename.encode(sys.getfilesystemencoding())
    print(filename)
    f = pythonopen(filename,"wb")
    f.write("; " + header["CardName"].encode("utf8") + "\n")
    f.write("; " + header["AuthorAndLicense"].encode("utf8") + "\n")
    f.write("; file produced by FreeCAD " + rev + "\n")
    f.write("; information about the content of this card can be found here:\n")
    f.write("; http://www.freecadweb.org/wiki/index.php?title=Material\n")
    f.write("\n")
    if header["Source"]:
        f.write("; source of the data provided in this card:\n")
        f.write("; " + header["Source"].encode("utf8") + "\n")
        f.write("\n")
    # write sections
    for s in contents:
        if s["keyname"] != "Meta":
            if len(s) > 1:
                # if the section has no contents, we don't write it
                f.write("[" + s["keyname"] + "]\n")
                for k,i in s.iteritems():
                    if k != "keyname":
                        f.write(k + "=" + i.encode('utf-8') + "\n")
                f.write("\n")
    f.close()
