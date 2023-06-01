# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


__title__ = "FreeCAD material card importer"
__author__ = "Juergen Riegel"
__url__ = "https://www.freecad.org"


import os
import glob
from pathlib import Path
import xml.etree.ElementTree as ET

def decode(name):
    "decodes encoded strings"
    try:
        decodedName = (name.decode("utf8"))
    except UnicodeDecodeError:
        try:
            decodedName = (name.decode("latin1"))
        except UnicodeDecodeError:
            print("Error: Couldn't determine character encoding")
            decodedName = name
    return decodedName

def read(filename):
    "reads a FCMat file and returns a dictionary from it"

    # the reader returns a dictionary in any case even if the file  has problems
    # an empty dict is returned in such case

    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    f = open(filename, encoding="utf8")
    try:
        content = f.readlines()
        # print(len(content))
        # print(type(content))
        # print(content)
    except Exception:
        # https://forum.freecad.org/viewtopic.php?f=18&t=56912#p489721
        # older FreeCAD do not write utf-8 for special character on windows
        # I have seen "ISO-8859-15" or "windows-1252"
        # explicit utf-8 writing, https://github.com/FreeCAD/FreeCAD/commit/9a564dd906f
        print("Error on card loading. File might not utf-8.")
        error_message = "Error on loading. Material file '{}' might not utf-8.".format(filename)
        print("{}\n".format(error_message))
        return {}
    d = {}
    d["Meta"] = {}
    d["General"] = {}
    d["Mechanical"] = {}
    d["Fluidic"] = {}
    d["Thermal"] = {}
    d["Electromagnetic"] = {}
    d["Architectural"] = {}
    d["Rendering"] = {}
    d["VectorRendering"] = {}
    d["Cost"] = {}
    d["UserDefined"] = {}
    d["Meta"]["CardName"] = card_name_file  # CardName is the MatCard file name
    section = ''
    for ln, line in enumerate(content):
        # print(line)
        # enumerate starts with 0

        # line numbers are used for CardName and AuthorAndLicense
        # the use of line number is not smart for a data model
        # a wrong user edit could break the file

        # comment
        if line.startswith('#'):
            # a '#' is assumed to be a comment which is ignored
            continue
        # CardName
        if line.startswith(';') and ln == 0:
            # print("Line CardName: {}".format(line))
            # v = line.split(";")[1].strip()  # Line 1
            # if hasattr(v, "decode"):
            #     v = v.decode('utf-8')
            # card_name_content = v
            pass

        # AuthorAndLicense
        elif line.startswith(';') and ln == 1:
            # print("Line AuthorAndLicense: {}".format(line))
            v = line.split(";")[1].strip()  # Line 2
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            d["General"]["AuthorAndLicense"] = v # Move the field to the general group

        # rest
        else:
            # ; is a Comment
            # [ is a Section
            if line[0] == '[':
                # print("parse section '{0}'".format(line))
                line = line[1:]
                # print("\tline '{0}'".format(line))
                k = line.split("]", 1)
                if len(k) >= 2:
                    v = k[0].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    section = v
                    # print("Section '{0}'".format(section))
            elif line[0] not in ";":
                # split once on first occurrence
                # a link could contain a '=' and thus would be split
                k = line.split("=", 1)
                if len(k) == 2:
                    v = k[1].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    # print("key '{0}', value '{1}'".format(k[0].strip(), v))
                    d[section][k[0].strip()] = v
    return d

def saveXML(card, output):
    root = ET.Element("FCMat")
    for group in card:
        print(group)
        groupNode = ET.SubElement(root, group)
        if group in ["Mechanical"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default mechanical model")
        elif group in ["Fluidic"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default fluidic model")
        elif group in ["Thermal"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default thermal model")
        elif group in ["Electromagnetic"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default electromagnetic model")
        elif group in ["Architectural"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default architecturaal model")
        elif group in ["Cost"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default cost model")
        elif group in ["Rendering"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default rendering model")
        elif group in ["VectorRendering"]:
            parentNode = ET.SubElement(groupNode, "Model", name="default", description="Default vector rendering model")
        else:
            parentNode = groupNode
        for parameter, value in card[group].items():
            print(parameter)
            paramNode = ET.SubElement(parentNode, parameter)
            paramNode.text = value

    # Write the XML
    tree = ET.ElementTree(root)
    tree.write(output, encoding="unicode", method="xml")

def convert(infolder, outfolder):
    a_path = infolder + '/**/*.FCMat'
    # print("path = '{0}'".format(a_path))
    dir_path_list = glob.glob(a_path, recursive=True)

    for a_path in dir_path_list:
        p = Path(a_path)
        relative = p.relative_to(infolder)
        out = Path(outfolder) / relative
        print("('{0}', '{1}') -> {2}".format(infolder, relative, out))

        try:
            card = read(p)
            print(card)
        except Exception:
            print("Error converting card '{0}'. Skipped.")
            continue

        out.parent.mkdir(parents=True, exist_ok=True)
        saveXML(card, out)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("infolder", help="Input folder containing older material cards")
parser.add_argument("outfolder", help="Output folder to place the converted material cards")
args = parser.parse_args()

print("Input folder '{0}'".format(args.infolder))
print("Output folder '{0}'".format(args.outfolder))

convert(args.infolder, args.outfolder)