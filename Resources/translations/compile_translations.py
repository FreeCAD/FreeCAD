#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#***************************************************************************
#*   compile_translations.py                                               *
#*   convert .ts files to .qm files for FreeCAD translations               *
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

import glob
import subprocess # nosec
import re
from pathlib import Path


# FreeCAD supports the following locales, but not all of them are fully translated.
# The list is generated using the following command: FreeCADGui.supportedLocales()
# and should be updated when new locales are added or removed from FreeCAD.
supported_locales = {'English': 'en', 'Afrikaans': 'af', 'Arabic': 'ar', 'Basque': 'eu', 
                     'Belarusian': 'be', 'Bulgarian': 'bg', 'Catalan': 'ca', 
                     'Chinese Simplified': 'zh-CN', 'Chinese Traditional': 'zh-TW', 
                     'Croatian': 'hr', 'Czech': 'cs', 'Danish': 'da', 'Dutch': 'nl', 
                     'Filipino': 'fil', 'Finnish': 'fi', 'French': 'fr', 'Galician': 'gl', 
                     'Georgian': 'ka', 'German': 'de', 'Greek': 'el', 'Hungarian': 'hu', 
                     'Indonesian': 'id', 'Italian': 'it', 'Japanese': 'ja', 'Kabyle': 'kab', 
                     'Korean': 'ko', 'Lithuanian': 'lt', 'Norwegian': 'no', 'Polish': 'pl', 
                     'Portuguese': 'pt-PT', 'Portuguese, Brazilian': 'pt-BR', 'Romanian': 'ro', 
                     'Russian': 'ru', 'Serbian': 'sr', 'Serbian, Latin': 'sr-CS', 'Slovak': 'sk', 
                     'Slovenian': 'sl', 'Spanish': 'es-ES', 'Spanish, Argentina': 'es-AR', 
                     'Swedish': 'sv-SE', 'Turkish': 'tr', 'Ukrainian': 'uk', 'Valencian': 'val-ES', 
                     'Vietnamese': 'vi'}
locale_values = supported_locales.values()
print(f"Supported locales: {', '.join(locale_values)}")
compiler = "lrelease"
workbench = "SheetMetal"
ts_files = [f"{workbench}/*.ts"]

files = []
for pattern in ts_files:
    files.extend(glob.glob(pattern))

# Crowdin naming scheme is a bit different than the one used by FreeCAD, so 
# we need to correct the names of the files before compiling them.
# For example, the file ending with _de-DE is not supported by FreeCAD, but the file ending with 
# _de is supported. So we try to match the locale code using the list above
def get_corrected_name(filename):
    """ Correct the name of a .ts file to match the expected format for FreeCAD."""
    lfile = Path(filename).stem
    res = re.search(r"(\S+)_(\w+)-(\w+)$", lfile)
    if res and len(res.groups()) == 3:
        base_name = res.group(1)
        localea = res.group(2)
        localeb = res.group(3)
        if localeb:
            locale = localea + "-" + localeb
        else:
            locale = localea
        if locale in locale_values:
            return f"{base_name}_{locale}.qm"
        elif localea in locale_values:
            return f"{base_name}_{localea}.qm"
    return "NOT_SUPPORTED"

def compile_ts_file(f):
    """Compile a .ts file using lrelease and handle the output."""
    compfile = get_corrected_name(f)
    if compfile == "NOT_SUPPORTED":
        print(f"{Path(f).stem} Not supported locale, skipping.")
        return
    cmd = [compiler, f, "-nounfinished", "-qm", compfile]
    result = subprocess.run( # nosec
        cmd,
        capture_output=True,
        text=True,
        check=False
    )
    if result.returncode != 0:
        print(f"Error compiling {f}: {result.stderr}")
        return
    res = re.search(r"\((\d+) finished and", result.stdout)
    if res:
        finished = int(res.group(1))
        ignored = 0
        res = re.search(r"Ignored (\d+)", result.stdout)
        if res:
            ignored = int(res.group(1))
        prec_finished = (finished / (finished + ignored)) * 100 if (finished + ignored) > 0 else 0
        if prec_finished > 10.0:
            print(f"{compfile}: {finished} finished ({prec_finished:.1f}%)")
        else:
            Path(compfile).unlink(missing_ok=True)
            print(f"{compfile}: less than 10% finished, skipping.")
    else:
        print(f"Could not parse output for {f}: {result.stdout}")

# Compile all the .ts files in the workbench directory
for fname in files:
    compile_ts_file(fname)
