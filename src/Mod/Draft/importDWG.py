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

import FreeCAD
from FreeCAD import Console as FCC

if FreeCAD.GuiUp:
    from draftutils.translate import translate
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
    import importDXF
    import os
    import tempfile
    outdir = tempfile.mkdtemp()
    _basename = os.path.splitext(os.path.basename(filename))[0]
    dxf = outdir + os.sep + _basename + ".dxf"
    importDXF.export(objectslist, dxf)
    convertToDwg(dxf, filename)
    return filename


def get_libredwg_converter(typ):
    """Find the LibreDWG converter.

    It searches the FreeCAD parameters database, then searches the OS search path.
    There are no standard installation paths.

    `typ` is required because LibreDWG uses two converters and we store only one.

    Parameters
    ----------
    typ : str
        "dwg2dxf" or "dxf2dwg".

    Returns
    -------
    str
        The full path of the converter.
    """
    import os
    import platform

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    path = p.GetString("TeighaFileConverter")

    if "dwg2dxf" in path or "dxf2dwg" in path: # path set manually
        if typ not in path:
            path = os.path.dirname(path) + "/" + typ + os.path.splitext(path)[1]
        if os.path.exists(path) and os.path.isfile(path):
            return path
    elif platform.system() == "Windows":
        for sub in os.getenv("PATH").split(os.pathsep):
            path = sub.replace("\\", "/") + "/" + typ + ".exe"
            if os.path.exists(path) and os.path.isfile(path):
                return path
    else: # for Linux and macOS
        for sub in os.getenv("PATH").split(os.pathsep):
            path = sub + "/" + typ
            if os.path.exists(path) and os.path.isfile(path):
                return path

    return None


def get_oda_converter():
    """Find the ODA converter.

    It searches the FreeCAD parameters database, then searches for common paths.

    Parameters
    ----------
    None

    Returns
    -------
    str
        The full path of the converter.
    """
    import os
    import platform

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    path = p.GetString("TeighaFileConverter")

    if "ODAFileConverter" in path: # path set manually
        if os.path.exists(path) and os.path.isfile(path):
            return path
    elif platform.system() == "Windows":
        odadir = os.path.expandvars("%ProgramFiles%\\ODA").replace("\\", "/")
        if os.path.exists(odadir):
            for sub in os.listdir(odadir):
                path = odadir + "/" + sub + "/" + "ODAFileConverter.exe"
                if os.path.exists(path) and os.path.isfile(path):
                    return path
    elif platform.system() == "Linux":
        path = "/usr/bin/ODAFileConverter"
        if os.path.exists(path) and os.path.isfile(path):
            return path
    else: # for macOS
        path = "/Applications/ODAFileConverter.app/Contents/MacOS/ODAFileConverter"
        if os.path.exists(path) and os.path.isfile(path):
            return path

    return None


def get_qcad_converter():
    """Find the QCAD converter.

    It searches the FreeCAD parameters database, then searches for common paths.

    Parameters
    ----------
    None

    Returns
    -------
    str
        The full path of the converter.
    """
    import os
    import platform

    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    path = p.GetString("TeighaFileConverter")

    if "dwg2dwg" in path: # path set manually
        pass
    elif platform.system() == "Windows":
        path = os.path.expandvars("%ProgramFiles%\\QCAD\\dwg2dwg.bat").replace("\\", "/")
    elif platform.system() == "Linux":
        # /home/$USER/opt/qcad-3.28.1-trial-linux-qt5.14-x86_64/dwg2dwg
        path = os.path.expandvars("/home/$USER/opt")
        if os.path.exists(path) and os.path.isdir(path):
            for sub in os.listdir(path):
                if "qcad" in sub:
                    path = path + "/" + sub + "/" + "dwg2dwg"
                    break
    else: # for macOS
        path = "/Applications/QCAD.app/Contents/Resources/dwg2dwg"

    if os.path.exists(path) and os.path.isfile(path):
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
    import os
    import subprocess
    import tempfile

    dwgfilename = dwgfilename.replace("\\", "/")
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    conv = p.GetInt("DWGConversion", 0)
    error_msg = translate("draft", """Error during DWG conversion.
Try moving the DWG file to a directory path without spaces and non-english characters,
or try saving to a lower DWG version.""") + "\n"

    if conv in [0, 1]: # LibreDWG
        libredwg = get_libredwg_converter("dwg2dxf")
        if libredwg is not None:
            outdir = tempfile.mkdtemp().replace("\\", "/")
            basename = os.path.basename(dwgfilename)
            result = outdir + "/" + os.path.splitext(basename)[0] + ".dxf"
            cmdline = [libredwg, dwgfilename, "-o", result]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            if os.path.exists(result):
                FCC.PrintMessage(translate("draft", "Conversion successful") + "\n")
                return result
            else:
                FCC.PrintError(error_msg)
        elif conv != 0:
            FCC.PrintError(translate("draft", "LibreDWG converter not found") + "\n")

    if conv in [0, 2]: # ODA
        oda = get_oda_converter()
        if oda is not None:
            indir = os.path.dirname(dwgfilename)
            outdir = tempfile.mkdtemp().replace("\\", "/")
            basename = os.path.basename(dwgfilename)
            cmdline = [oda, indir, outdir, "ACAD2000", "DXF", "0", "1", basename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            result = outdir + "/" + os.path.splitext(basename)[0] + ".dxf"
            if os.path.exists(result):
                FCC.PrintMessage(translate("draft", "Conversion successful") + "\n")
                return result
            else:
                FCC.PrintError(error_msg)
        elif conv != 0:
            FCC.PrintError(translate("draft", "ODA converter not found") + "\n")

    if conv in [0, 3]: # QCAD
        qcad = get_qcad_converter()
        if qcad is not None:
            outdir = tempfile.mkdtemp().replace("\\", "/")
            basename = os.path.basename(dwgfilename)
            result = outdir + "/" + os.path.splitext(basename)[0] + ".dxf"
            cmdline = [qcad, "-f", "-o", result, dwgfilename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline, cwd=os.path.dirname(qcad)) # cwd required for Windows
            proc.communicate()
            if os.path.exists(result):
                FCC.PrintMessage(translate("draft", "Conversion successful") + "\n")
                return result
            else:
                FCC.PrintError(error_msg)
        elif conv != 0:
            FCC.PrintError(translate("draft", "QCAD converter not found") + "\n")

    FCC.PrintError(translate("draft", """No suitable external DWG converter has been found.
Please set one manually under menu Edit -> Preferences -> Import/Export -> DWG
For more information see:
https://wiki.freecad.org/Import_Export_Preferences""") + "\n")
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
    import os
    import subprocess

    dxffilename = dxffilename.replace("\\", "/")
    dwgfilename = dwgfilename.replace("\\", "/")
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    conv = p.GetInt("DWGConversion", 0)

    if conv in [0, 1]: # LibreDWG
        libredwg = get_libredwg_converter("dxf2dwg")
        if libredwg is not None:
            cmdline = [libredwg, dxffilename, "-y", "-o", dwgfilename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            return dwgfilename
        elif conv != 0:
            FCC.PrintError(translate("draft", "LibreDWG converter not found") + "\n")

    if conv in [0, 2]: # ODA
        oda = get_oda_converter()
        if oda is not None:
            indir = os.path.dirname(dxffilename)
            outdir = os.path.dirname(dwgfilename)
            basename = os.path.basename(dxffilename)
            cmdline = [oda, indir, outdir, "ACAD2000", "DWG", "0", "1", basename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline)
            proc.communicate()
            return dwgfilename
        elif conv != 0:
            FCC.PrintError(translate("draft", "ODA converter not found") + "\n")

    if conv in [0, 3]: # QCAD
        qcad = get_qcad_converter()
        if qcad is not None:
            cmdline = [qcad, "-f", "-o", dwgfilename, dxffilename]
            FCC.PrintMessage(translate("draft", "Converting:") + " " + str(cmdline) + "\n")
            proc = subprocess.Popen(cmdline, cwd=os.path.dirname(qcad)) # cwd required for Windows
            proc.communicate()
            return dwgfilename
        elif conv != 0:
            FCC.PrintError(translate("draft", "QCAD converter not found") + "\n")

    FCC.PrintError(translate("draft", """No suitable external DWG converter has been found.
Please set one manually under menu Edit -> Preferences -> Import/Export -> DWG
For more information see:
https://wiki.freecad.org/Import_Export_Preferences""") + "\n")
    return None
