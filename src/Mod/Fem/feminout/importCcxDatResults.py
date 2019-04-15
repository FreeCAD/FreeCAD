# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
# *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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

__title__ = "importCcxDatResults"
__author__ = "Przemo Firszt, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importCcxDatResults
#  \ingroup FEM
#  \brief FreeCAD Calculix DAT reader for FEM workbench

import FreeCAD
import os


EIGENVALUE_OUTPUT_SECTION = "     E I G E N V A L U E   O U T P U T"


# ********* generic FreeCAD import and export methods *********
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_dat(filename)


# ********* module specific methods *********
def import_dat(filename, Analysis=None):
    r = readResult(filename)
    # print("Results {}".format(r))
    return r


# read a calculix result file and extract the data
def readResult(dat_input):
    FreeCAD.Console.PrintMessage('Read ccx results from dat file: {}\n'.format(dat_input))
    dat_file = pyopen(dat_input, "r")
    eigenvalue_output_section_found = False
    mode_reading = False
    results = []

    for line in dat_file:
        if EIGENVALUE_OUTPUT_SECTION in line:
            # Found EIGENVALUE_OUTPUT_SECTION
            eigenvalue_output_section_found = True
        if eigenvalue_output_section_found:
            try:
                mode = int(line[0:7])
                mode_frequency = float(line[39:55])
                m = {}
                m['eigenmode'] = mode
                m['frequency'] = mode_frequency
                results.append(m)
                mode_reading = True
            except:
                if mode_reading:
                    # Conversion error after mode reading started, so it's the end of section
                    eigenvalue_output_section_found = False
                    mode_reading = False

    dat_file.close()
    return results
