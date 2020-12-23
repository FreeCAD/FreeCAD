# ***************************************************************************
# *   (c) sliptonic (shopinthewoods@gmail.com) 2014                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/
from __future__ import print_function

TOOLTIP='''
Dumper is an extremely simple postprocessor file for the Path workbench. It is used
to dump the command list from one or more Path objects for simple inspection. This post
doesn't do any manipulation of the path and doesn't write anything to disk.  It just
shows the dialog so you can see it.  Useful for debugging, but not much else.
'''
import datetime
from PathScripts import PostUtils

now = datetime.datetime.now()
SHOW_EDITOR = True

# to distinguish python built-in open function from the one declared below
if open.__module__ in ['__builtin__','io']:
    pythonopen = open


def export(objectslist, filename,argstring):
    "called when freecad exports a list of objects"
    # pylint: disable=unused-argument

    output = '''(This output produced with the dump post processor)
(Dump is useful for inspecting the raw commands in your paths)
(but is not useful for driving machines.)
(Consider setting a default postprocessor in your project or )
(exporting your paths using a specific post that matches your machine)

'''

    for obj in objectslist:

        if not hasattr(obj, "Path"):
            print("the object " + obj.Name + " is not a path. Please select only path and Compounds.")
            return
        print("postprocessing...")
        output += parse(obj)

    if SHOW_EDITOR:
        dia = PostUtils.GCodeEditorDialog()
        dia.editor.setText(output)
        result = dia.exec_()
        if result:
            final = dia.editor.toPlainText()
        else:
            final = output
    else:
        final = output

    print("done postprocessing.")
    return final


def parse(pathobj):
    out = ""

    if hasattr(pathobj, "Group"):  # We have a compound or project.
        out += "(Group: " + pathobj.Label + ")\n"
        for p in pathobj.Group:
            out += parse(p)
        return out
    else:  # parsing simple path

        if not hasattr(pathobj, "Path"):  # groups might contain non-path things like stock.
            return out

        out += "(Path: " + pathobj.Label + ")\n"

        for c in pathobj.Path.Commands:
            out += str(c) + "\n"
        return out

print(__name__ + " gcode postprocessor loaded.")
