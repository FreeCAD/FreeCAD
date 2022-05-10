# -*- coding: utf8 -*-
## @package importDWG
#  \ingroup DRAFT
#  \brief DWG file importer & exporter
'''
@package importDWG
ingroup DRAFT
\brief DWG file importer & exporter

This module provides support for importing and exporting Autodesk DWG files.
This module is only a thin layer that uses the ODA (formerly Teigha) File
Converter application to convert to/from DXF. Then the real work is done by
importDXF

The converter may be called
/usr/bin/TeighaFileConverter
/usr/bin/ODAFileConverter

Test files
https://knowledge.autodesk.com/support/autocad/downloads/
    caas/downloads/content/autocad-sample-files.html
'''
# Check code quality with
# flake8 --ignore=E226,E266,E401,W503

# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
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

import six
import FreeCAD
from FreeCAD import Console as FCC

if FreeCAD.GuiUp:
    from DraftTools import translate
else:
    def translate(context, txt):
        return txt

# Save the native open function to avoid collisions
if open.__module__ == '__builtin__':
    pythonopen = open


def open(filename):
    """Open filename and parse using importDXF.open().

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.

    Returns
    -------
    App::Document
        The new FreeCAD document object created, with the parsed information.
    """
    dxf = convertToDxf(filename)
    if dxf:
        import importDXF
        doc = importDXF.open(dxf)
        return doc
    return


def insert(filename, docname):
    """Imports a file using importDXF.insert().

    If no document exist, it is created.

    Parameters
    ----------
    filename : str
        The path to the filename to be opened.
    docname : str
        The name of the active App::Document if one exists, or
        of the new one created.

    Returns
    -------
    App::Document
        The active FreeCAD document, or the document created if none exists,
        with the parsed information.
    """
    dxf = convertToDxf(filename)
    if dxf:
        import importDXF
        # Warning: function doesn't return?
        doc = importDXF.insert(dxf, docname)
        return doc
    return


def export(objectslist, filename):
    """Export the DWG file with a given list of objects.

    The objects are exported with importDXF.export().
    Then the result is converted to DWG.

    Parameters
    ----------
    exportList : list
        List of document objects to export.
    filename : str
        Path to the new file.

    Returns
    -------
    str
        The same `filename` input.
    """
    import importDXF, os, tempfile
    outdir = tempfile.mkdtemp()
    _basename = os.path.splitext(os.path.basename(filename))[0]
    dxf = outdir + os.sep + _basename + ".dxf"
    importDXF.export(objectslist, dxf)
    convertToDwg(dxf, filename)
    return filename


def getTeighaConverter():
    """Find the ODA (formerly Teigha) executable.

    It searches the FreeCAD parameters database, then searches for common
    paths in Linux and Windows systems.

    Parameters
    ----------
    None

    Returns
    -------
    str
        The full path of the converter executable
        '/usr/bin/ODAFileConverter'
    """
    import FreeCAD, os, platform
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    path = p.GetString("TeighaFileConverter")
    if path:
        # path set manually
        return path
    elif platform.system() == "Linux":
        path = "/usr/bin/ODAFileConverter"
        if os.path.exists(path):
            return path
    elif platform.system() == "Windows":
        odadir = os.path.expandvars("%ProgramFiles%\ODA")
        if os.path.exists(odadir):
            for sub in os.listdir(odadir):
                path = os.path.join(odadir, sub, "ODAFileConverter.exe")
                if os.path.exists(path):
                    return path
    return None


def convertToDxf(dwgfilename):
    """Convert a DWG file to a DXF file.

    If the converter is found it is used, otherwise the conversion fails.

    Parameters
    ----------
    dwgfilename : str
        The input filename.

    Returns
    -------
    str
        The new file produced.
    """
    import os, tempfile, subprocess, sys

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    conv = p.GetInt("DWGConversion",0)
    path = p.GetString("TeighaFileConverter","")

    if conv in [0,1]: # LibreDWG dxf2dwg/dwg2dxf
        if not path:
            path = "dwg2dxf"
        if path.endswith("dxf2dwg"):
            path = os.path.join(os.path.dirname,"dwg2dxf")
        try:
            outdir = tempfile.mkdtemp()
            basename = os.path.basename(dwgfilename)
            result = outdir + os.sep + os.path.splitext(basename)[0] + ".dxf"
            proc = subprocess.Popen((path, dwgfilename, "-o", result))
            proc.communicate()
            return result
        except Exception:
            if conv != 0:
                FCC.PrintError(translate("draft", "LibreDWG error") + "\n")

    if conv in [0,2]: # ODA
        teigha = getTeighaConverter()
        if teigha:
            indir = os.path.dirname(dwgfilename)
            outdir = tempfile.mkdtemp()
            basename = os.path.basename(dwgfilename)
            cmdline = [teigha, indir, outdir, "ACAD2000", "DXF", "0", "1", basename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            result = outdir + os.sep + os.path.splitext(basename)[0] + ".dxf"
            if os.path.exists(result):
                FCC.PrintMessage(translate("draft", "Conversion successful") + "\n")
                return result
            else:
                FCC.PrintError(translate("draft","Error during DWG conversion. Try moving the DWG file to a directory path without spaces and non-english characters, or try saving to a lower DWG version.") + "\n")
        else:
            if conv != 0:
                    FCC.PrintError(translate("draft", "ODA File Converter not found") + "\n")

    if conv in [0,3]: # QCAD
        if not path:
            path = "dwg2dwg"
        try:
            outdir = tempfile.mkdtemp()
            basename = os.path.basename(dwgfilename)
            result = outdir + os.sep + os.path.splitext(basename)[0] + ".dxf"
            proc = subprocess.Popen((path, "-f", "-o", result, dwgfilename))
            proc.communicate()
            return result
        except Exception:
            FCC.PrintError(translate("draft", "QCAD error") + "\n")

    return None


def convertToDwg(dxffilename, dwgfilename):
    """Convert a DXF file to a DWG file.

    If the converter is found it is used, otherwise the conversion fails.

    Parameters
    ----------
    dxffilename : str
        The input DXF file
    dwgfilename : str
        The output DWG file

    Returns
    -------
    str
        The same `dwgfilename` file path.
    """
    import os, subprocess, shutil

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    conv = p.GetInt("DWGConversion",0)
    path = p.GetString("TeighaFileConverter","")

    if conv in [0,1]: # LibreDWG dxf2dwg/dwg2dxf
        if not path:
            path = "dxf2dwg"
        if path.endswith("dwg2dxf"):
            path = os.path.join(os.path.dirname,"dxf2dwg")
        try:
            proc = subprocess.Popen((path, dxffilename, "-y", "-o", dwgfilename))
            proc.communicate()
            return dwgfilename
        except Exception:
            if conv != 0:
                FCC.PrintError(translate("draft", "LibreDWG error") + "\n")

    if conv in [0,2]: # ODA
        teigha = getTeighaConverter()
        if teigha:
            indir = os.path.dirname(dxffilename)
            outdir = os.path.dirname(dwgfilename)
            basename = os.path.basename(dxffilename)
            cmdline = [teigha, indir, outdir, "ACAD2000", "DWG", "0", "1", basename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            return dwgfilename
        else:
            if conv != 0:
                FCC.PrintError(translate("draft", "ODA File Converter not found") + "\n")

    if conv in [0,3]: # QCAD
        if not path:
            path = "dwg2dwg"
        try:
            proc = subprocess.Popen((path, "-o", dwgfilename, dxffilename))
            proc.communicate()
            return dwgfilename
        except Exception:
            FCC.PrintError(translate("draft", "QCAD error") + "\n")

    return None
