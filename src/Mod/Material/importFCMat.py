# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
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


import FreeCAD
import Material
import os
import sys
if sys.version_info.major >= 3:
    unicode = str


__title__ = "FreeCAD material card importer"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"


# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__', 'io']:
    pythonopen = open


def open(filename):
    "called when freecad wants to open a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    doc = FreeCAD.newDocument(docname)
    doc.Label = docname
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    read(filename)
    return doc


def export(exportList, filename):
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


# the reader and writer do not use some Library to read and write the ini file format, they are implemented here
# thus non standard ini files will be read and written too
# in standard ini file format a = in the value without any encapsulation or string quotes is not allowed (AFAIK)
# https://en.wikipedia.org/wiki/INI_file
# http://www.docuxplorer.com/WebHelp/INI_File_Format.htm
# mainly this parser here is used in FreeCAD
# in the module Material.py is another implementaion of reading and writing FCMat files which uses the module ConfigParser
# in ViewProviderFemMaterial in add_cards_from_a_dir() the parser from Material.py is used
# since this mixture seams to be there for ages it should not be changed for 0.18
# TODO: get rid of this mixture in FreeCAD 0.19

# Metainformations
# first five lines are the same in any card file
# Line1: card name
# Line2: author and licence
# Line3: information string
# Line4: information link
# Line5: FreeCAD version info or empty
def read(filename):
    "reads a FCMat file and returns a dictionary from it"
    if isinstance(filename, unicode):
        if sys.version_info.major < 3:
            filename = filename.encode(sys.getfilesystemencoding())
    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    if sys.version_info.major >= 3:
        f = pythonopen(filename, encoding="utf8")
    else:
        f = pythonopen(filename)
    d = {}
    d["CardName"] = card_name_file  # CardName is the MatCard file name
    ln = 0
    for line in f:
        if ln == 0:
            v = line.split(";")[1].strip()  # Line 1
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            card_name_content = v
            if card_name_content != d["CardName"]:
                FreeCAD.Console.PrintError("File CardName (" + card_name_file + ") is not content CardName (" + card_name_content + ")\n")
        elif ln == 1:
            v = line.split(";")[1].strip()  # Line 2
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            d["AuthorAndLicense"] = v
        else:
            # ; is a Commend
            # # might be a comment too ?
            # [ is a Section
            if line[0] not in ";#[":
                k = line.split("=", 1)  # only split once on first occurence, a link could contain a = and thus would be splitted
                if len(k) == 2:
                    v = k[1].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    d[k[0].strip()] = v
        ln += 1
    return d


def write(filename, dictionary):
    "writes the given dictionary to the given file"

    # sort the data into sections
    contents = []
    tree = Material.getMaterialAttributeStructure()
    MatPropDict = tree.getroot()
    for group in MatPropDict.getchildren():
        groupName = group.attrib['Name']
        contents.append({"keyname": groupName})
        if groupName == "Meta":
            header = contents[-1]
        elif groupName == "User defined":
            user = contents[-1]
        for proper in group.getchildren():
            properName = proper.attrib['Name']
            contents[-1][properName] = ''
    for k, i in dictionary.items():
        found = False
        for group in contents:
            if not found:
                if k in group.keys():
                    group[k] = i
                    found = True
        if not found:
            user[k] = i
    # delete empty properties
    for group in contents:
        for k in list(group.keys()):  # iterating over a dict and changing it is not allowed, thus we iterate over a list of the keys
            if group[k] == '':
                del group[k]

    # card writer
    rev = FreeCAD.ConfigGet("BuildVersionMajor") + "." + FreeCAD.ConfigGet("BuildVersionMinor") + "." + FreeCAD.ConfigGet("BuildRevision")
    if isinstance(filename, unicode):
        if sys.version_info.major < 3:
            filename = filename.encode(sys.getfilesystemencoding())
    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    # print(card_name_file)
    f = pythonopen(filename, "w")
    # write header
    # first five lines are the same in any card file, see comment above read def
    if header["CardName"] != card_name_file:
        FreeCAD.Console.PrintMessage("File CardName is used: " + card_name_file + " \n")  # CardName is the MatCard file name
    if sys.version_info.major >= 3:
        f.write("; " + card_name_file + "\n")
        f.write("; " + header["AuthorAndLicense"] + "\n")
    else:
        f.write("; " + header["CardName"].encode("utf8") + "\n")
        f.write("; " + header["AuthorAndLicense"].encode("utf8") + "\n")
    f.write("; information about the content of such cards you can find here:\n")
    f.write("; http://www.freecadweb.org/wiki/index.php?title=Material\n")
    f.write("; file produced by FreeCAD" + rev + "\n")
    f.write("\n")
    # write sections
    for s in contents:
        if s["keyname"] != "Meta":
            if len(s) > 1:
                # if the section has no contents, we don't write it
                f.write("[" + s["keyname"] + "]\n")
                for k, i in s.items():
                    if (k != "keyname" and i != '') or k == "Name":
                        # use only keys which are not empty and the name even if empty
                        if sys.version_info.major >= 3:
                            f.write(k + " = " + i + "\n")
                        else:
                            f.write(k + " = " + i.encode('utf-8') + "\n")
                f.write("\n")
    f.close()
