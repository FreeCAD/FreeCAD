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

import FreeCAD

from materialtools.cardutils import get_material_template
import Material

if FreeCAD.GuiUp:
    from PySide import QtGui


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


# the reader and writer do not use some Library to read and write the ini file format
# they are implemented here
# thus non standard ini files will be read and written too
# in standard ini file format:
# a = in the value without any encapsulation or string quotes is not allowed (AFAIK)
# https://en.wikipedia.org/wiki/INI_file
# http://www.docuxplorer.com/WebHelp/INI_File_Format.htm
# mainly this parser here is used in FreeCAD
# in the module Material.py is another implementation of reading and writing FCMat files
# the implementation in Material.py uses Pythons ConfigParser module
# in ViewProviderFemMaterial in add_cards_from_a_dir() the parser from Material.py is used
# since this mixture seems to have be there for ages it should not be changed for 0.18
# TODO and FIXME:
# get rid of this mixture
# best might be to switch to a more robust file schema like YAML
# as we had and we might will have problems again and again
# https://github.com/berndhahnebach/FreeCAD_bhb/commits/materialdev

def read(filename):
    materialManager = Material.MaterialManager()
    material = materialManager.getMaterialByPath(filename)
    return material.Properties

# Metainformation
# first two lines HAVE, REALLY HAVE to be the same (no comment) in any card file !!!!!
# first five lines are the same in any card file
# Line1: card name
# Line2: author and licence
# Line3: information string
# Line4: information link
# Line5: FreeCAD version info or empty
def read_old(filename):
    "reads a FCMat file and returns a dictionary from it"

    # the reader returns a dictionary in any case even if the file  has problems
    # an empty dict is returned in such case

    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    f = pythonopen(filename, encoding="utf8")
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
        FreeCAD.Console.PrintError("Error on card loading. File might not utf-8.")
        error_message = "Error on loading. Material file '{}' might not utf-8.".format(filename)
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(None, "Error on card reading", error_message)
        return {}
    d = {}
    d["CardName"] = card_name_file  # CardName is the MatCard file name
    for ln, line in enumerate(content):
        # print(line)
        ln += 1  # enumerate starts with 0, but we would like to have the real line number

        # line numbers are used for CardName and AuthorAndLicense
        # the use of line number is not smart for a data model
        # a wrong user edit could break the file

        # comment
        if line.startswith('#'):
            # a '#' is assumed to be a comment which is ignored
            continue
        # CardName
        if line.startswith(';') and ln == 1:
            # print("Line CardName: {}".format(line))
            v = line.split(";")[1].strip()  # Line 1
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            card_name_content = v
            if card_name_content != d["CardName"]:
                FreeCAD.Console.PrintLog(
                    "File CardName ( {} ) is not content CardName ( {} )\n"
                    .format(card_name_file, card_name_content)
                )

        # AuthorAndLicense
        elif line.startswith(';') and ln == 2:
            # print("Line AuthorAndLicense: {}".format(line))
            v = line.split(";")[1].strip()  # Line 2
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            d["AuthorAndLicense"] = v

        # rest
        else:
            # ; is a Comment
            # [ is a Section
            if line[0] not in ";[":
                # split once on first occurrence
                # a link could contain a '=' and thus would be split
                k = line.split("=", 1)
                if len(k) == 2:
                    v = k[1].strip()
                    if hasattr(v, "decode"):
                        v = v.decode('utf-8')
                    d[k[0].strip()] = v
    return d

def read2(filename):
    "reads a FCMat file and returns a dictionary from it"

    # the reader returns a dictionary in any case even if the file  has problems
    # an empty dict is returned in such case

    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    f = pythonopen(filename, encoding="utf8")
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
        FreeCAD.Console.PrintError("Error on card loading. File might not utf-8.")
        error_message = "Error on loading. Material file '{}' might not utf-8.".format(filename)
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(None, "Error on card reading", error_message)
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
            v = line.split(";")[1].strip()  # Line 1
            if hasattr(v, "decode"):
                v = v.decode('utf-8')
            card_name_content = v
            if card_name_content != d["Meta"]["CardName"]:
                FreeCAD.Console.PrintLog(
                    "File CardName ( {} ) is not content CardName ( {} )\n"
                    .format(card_name_file, card_name_content)
                )

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


def write(filename, dictionary, write_group_section=True):
    "writes the given dictionary to the given file"

    # sort the data into sections
    contents = []
    user = {}
    template_data = get_material_template()
    for group in template_data:
        groupName = list(group)[0]  # group dict has only one key
        contents.append({"keyname": groupName})
        if groupName == "Meta":
            header = contents[-1]
        elif groupName == 'UserDefined':
            user = contents[-1]
        for properName in group[groupName]:
            contents[-1][properName] = ''
    for k, i in dictionary.items():
        found = False
        for group in contents:
            if not found:
                if k in group:
                    group[k] = i
                    found = True
        if not found:
            user[k] = i
    # delete empty properties
    for group in contents:
        # iterating over a dict and changing it is not allowed
        # thus it is iterated over a list of the keys
        for k in list(group):
            if group[k] == '':
                del group[k]

    # card writer
    rev = "{}.{}.{}".format(
        FreeCAD.ConfigGet("BuildVersionMajor"),
        FreeCAD.ConfigGet("BuildVersionMinor"),
        FreeCAD.ConfigGet("BuildRevision")
    )
    # print(filename)
    card_name_file = os.path.splitext(os.path.basename(filename))[0]
    # print(card_name_file)
    if "CardName" not in header:
        print(header)
        error_message = "No card name provided. Card could not be written.".format(header)
        FreeCAD.Console.PrintError("{}\n".format(error_message))
        if FreeCAD.GuiUp:
            QtGui.QMessageBox.critical(None, "No card name", error_message)
        return
    f = pythonopen(filename, "w", encoding="utf-8")
    # write header
    # first five lines are the same in any card file, see comment above read def
    if header["CardName"] != card_name_file:
        # CardName is the MatCard file name
        FreeCAD.Console.PrintWarning(
            "The file name {} is not equal to the card name {}. The file name is used."
            .format(card_name_file, header["CardName"])
        )
    f.write("; " + card_name_file + "\n")
    # f.write("; " + header["AuthorAndLicense"] + "\n")
    f.write("; " + header.get("AuthorAndLicense", "no author") + "\n")
    f.write("; information about the content of such cards can be found on the wiki:\n")
    f.write("; https://www.freecad.org/wiki/Material\n")
    f.write("; file created by FreeCAD " + rev + "\n")
    # write sections
    # write standard FCMat section if write group section parameter is set to False
    if write_group_section is False:
        f.write("\n[FCMat]\n")
    for s in contents:
        if s["keyname"] != "Meta":
            # if the section has no contents, we don't write it
            if len(s) > 1:
                # only write group section if write group section parameter is set to True
                if write_group_section is True:
                    f.write("\n[" + s["keyname"] + "]\n")
                for k, i in s.items():
                    if (k != "keyname" and i != '') or k == "Name":
                        # use only keys which are not empty and the name, even if empty
                        f.write(k + " = " + i + "\n")
    f.close()


# ***** some code examples ***********************************************************************
'''
from materialtools.cardutils import get_source_path as getsrc
from importFCMat import read, write
readmatfile = getsrc() + '/src/Mod/Material/StandardMaterial/Concrete-Generic.FCMat'
writematfile = '/tmp/Concrete-Generic.FCMat'
matdict = read(readmatfile)
matdict
write(writematfile, matdict)

'''
