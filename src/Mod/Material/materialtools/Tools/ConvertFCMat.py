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
import uuid
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

    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    f = open(filename, encoding="utf8")
    try:
        content = f.readlines()
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
        # line numbers are used for CardName and AuthorAndLicense
        # the use of line number is not smart for a data model
        # a wrong user edit could break the file

        # comment
        if line.startswith('#'):
            # a '#' is assumed to be a comment which is ignored
            continue
        # CardName
        if line.startswith(';') and ln == 0:
            pass

        # AuthorAndLicense
        elif line.startswith(';') and ln == 1:
            v = line.split(";")[1].strip()  # Line 2
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            d["General"]["AuthorAndLicense"] = v # Move the field to the general group

        # rest
        else:
            # ; is a Comment
            # [ is a Section
            if line[0] == '[':
                line = line[1:]
                k = line.split("]", 1)
                if len(k) >= 2:
                    v = k[0].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    section = v
            elif line[0] not in ";":
                # split once on first occurrence
                # a link could contain a '=' and thus would be split
                k = line.split("=", 1)
                if len(k) == 2:
                    v = k[1].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    d[section][k[0].strip()] = v
    return d

def yamGeneral(card):
    father = ""
    materialStandard = ""
    yamModels = ""
    yam = "# File created by ConvertFCMat.py\n"
    yam += "General:\n"

    # Add UUIDs
    yam += '  UUID: "{0}"\n'.format(uuid.uuid4())
    for param in card:
        if param in ["Name", "AuthorAndLicense", "Description", "ReferenceSource", "SourceURL"]:
            yam += '  {0}: "{1}"\n'.format(param, card[param])
        elif param  in ["Father"]:
            father += '    {0}: "{1}"\n'.format(param, card[param])
        elif param  in ["KindOfMaterial", "MaterialNumber", "Norm", "StandardCode"]:
            if param == "Norm": # Handle the name change
                materialStandard += '    {0}: "{1}"\n'.format("StandardCode", card[param])
            else:
                materialStandard += '    {0}: "{1}"\n'.format(param, card[param])

    if len(father) > 0:
        yamModels += "  {0}:\n".format('Father')
        yamModels += "    UUID: '{0}'\n".format('9cdda8b6-b606-4778-8f13-3934d8668e67')
        yamModels += father
    if len(materialStandard) > 0:
        yamModels += "  {0}:\n".format('MaterialStandard')
        yamModels += "    UUID: '{0}'\n".format('1e2c0088-904a-4537-925f-64064c07d700')
        yamModels += materialStandard

    return yam, yamModels

def yamSection(card, header, uuid):
    if len(card) > 0:
        yam = "  {0}:\n".format(header)
        yam += "    UUID: '{0}'\n".format(uuid)
        for param in card:
            yam += '    {0}: "{1}"\n'.format(param, card[param])
    else:
        yam = ""

    return yam

def yamMechanical(card):
    # Check which model we need
    useDensity = False
    useIso = False
    useLinearElastic = False
    for param in card:
        if param in ["Density"]:
            useDensity = True
        elif param in ["BulkModulus", "PoissonRatio", "ShearModulus", "YoungsModulus"]:
            useIso = True
        elif param in ["AngleOfFriction", "CompressiveStrength", "FractureToughness", 
                       "UltimateStrain", "UltimateTensileStrength", "YieldStrength", "Stiffness", "Hardness"]:
            useLinearElastic = True

    yam = ""
    if useLinearElastic:
        return yamSection(card, 'LinearElastic', '7b561d1d-fb9b-44f6-9da9-56a4f74d7536')
    if useIso:
        yam = yamSection(card, 'IsotropicLinearElastic', 'f6f9e48c-b116-4e82-ad7f-3659a9219c50')
    if useDensity:
        return yam + yamSection(card, 'Density', '454661e5-265b-4320-8e6f-fcf6223ac3af')

    # default mechanical model
    return ""

def yamFluid(card):
    # Split out density
    for param in card:
        if param not in ["Density"]:
            return yamSection(card, 'Fluid', '1ae66d8c-1ba1-4211-ad12-b9917573b202')
        
    return yamSection(card, 'Density', '454661e5-265b-4320-8e6f-fcf6223ac3af')

def yamThermal(card):
    return yamSection(card, 'Thermal', '9959d007-a970-4ea7-bae4-3eb1b8b883c7')

def yamElectromagnetic(card):
    return yamSection(card, 'Electromagnetic', 'b2eb5f48-74b3-4193-9fbb-948674f427f3')

def yamArchitectural(card):
    return yamSection(card, 'Architectural', '32439c3b-262f-4b7b-99a8-f7f44e5894c8')

def yamCost(card):
    return yamSection(card, 'Costs', '881df808-8726-4c2e-be38-688bb6cce466')

def yamRendering(card):
    # Check which model we need
    useTexture = False
    useAdvanced = False
    for param in card:
        if param in ["TexturePath", "TextureScaling"]:
            useTexture = True
        elif param in ["FragmentShader", "VertexShader"]:
            useAdvanced = True

    if useAdvanced:
        return yamSection(card, 'AdvancedRendering', 'c880f092-cdae-43d6-a24b-55e884aacbbf')
    if useTexture:
        return yamSection(card, 'TextureRendering', 'bbdcc65b-67ca-489c-bd5c-a36e33d1c160')

    # default rendering model
    return yamSection(card, 'BasicRendering', 'f006c7e4-35b7-43d5-bbf9-c5d572309e6e')

def yamVectorRendering(card):
    return yamSection(card, 'VectorRendering', 'fdf5a80e-de50-4157-b2e5-b6e5f88b680e')

def saveYaml(card, output):
    yam, yamModels = yamGeneral(card["General"])
    if len(card["Mechanical"]) > 0 or \
        len(card["Fluidic"]) > 0 or \
        len(card["Thermal"]) > 0 or \
        len(card["Electromagnetic"]) > 0 or \
        len(card["Architectural"]) > 0 or \
        len(card["Cost"]) > 0 or \
        len(yamModels) > 0:
        yam += "Models:\n"
        yam += yamModels
        if "Mechanical" in card:
            yam += yamMechanical(card["Mechanical"])
        if "Fluidic" in card:
            yam += yamFluid(card["Fluidic"])
        if "Thermal" in card:
            yam += yamThermal(card["Thermal"])
        if "Electromagnetic" in card:
            yam += yamElectromagnetic(card["Electromagnetic"])
        if "Architectural" in card:
            yam += yamArchitectural(card["Architectural"])
        if "Cost" in card:
            yam += yamCost(card["Cost"])
    if len(card["Rendering"]) > 0 or len(card["VectorRendering"]) > 0:
        yam += "AppearanceModels:\n"
        if "Rendering" in card:
            yam += yamRendering(card["Rendering"])
        if "VectorRendering" in card:
            yam += yamVectorRendering(card["VectorRendering"])

    file = open(output, "w", encoding="utf-8")
    file.write(yam)
    file.close()

def convert(infolder, outfolder):
    a_path = infolder + '/**/*.FCMat'
    dir_path_list = glob.glob(a_path, recursive=True)

    for a_path in dir_path_list:
        p = Path(a_path)
        relative = p.relative_to(infolder)
        out = Path(outfolder) / relative
        print("('{0}', '{1}') -> {2}".format(infolder, relative, out))

        try:
            card = read(p)
        except Exception:
            print("Error converting card '{0}'. Skipped.")
            continue

        out.parent.mkdir(parents=True, exist_ok=True)
        saveYaml(card, out)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("infolder", help="Input folder containing older material cards")
parser.add_argument("outfolder", help="Output folder to place the converted material cards")
args = parser.parse_args()

print("Input folder '{0}'".format(args.infolder))
print("Output folder '{0}'".format(args.outfolder))

convert(args.infolder, args.outfolder)