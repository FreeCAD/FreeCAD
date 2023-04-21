#!/usr/bin/python3
# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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
#
# Script for manipulating toolbit shapes in a batch. This is handy for making
# attributes consistent over multiple or all shape file.
#
# Most commands are straight forward, except for --set which can be used to
# set the actual value of a property, or, in the case of enumerations it can
# also be used to set the enum values. This might be particularly useful when
# adding more materials.
#
# The following example moves all properties from the "Extra" group into the
# "Attributes" group. Note that the Attributes group might or might not
# already exist. If it does exist the specified group gets merged in.
#
#   ./toolbit-attributes.py --move 'Extra:Attributes' src/Mod/Path/Tools/Shape/*.fcstd
#
# This example sets the Flutes value of all Shapes to 0:
#
#   ./toolbit-attributes.py --set Flutes=0 src/Mod/Path/Tools/Shape/*.fcstd
#
# Finally, this example sets the enumerations of the Material attribute:
#
#   ./toolbit-attributes.py --set 'Material=[HSS,Carbide,Tool Steel,Titanium]' src/Mod/Path/Tools/Shape/*.fcstd
#
# After running this tool it might be necessary to open the shape files
# manually and make sure they are visible and the thumbprint image is
# saved.
#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!! Final note: this is a dangerous tool and should not be shipped with FC. !!!!
# !!! It's sole purpose is to make life of the developers easier and ensure   !!!!
# !!! consistent attributes across all toobit shapes.                         !!!!
# !!! A single typo can ruin a lot of toolbit shapes - make sure to only use  !!!!
# !!! it if those shape files are under a version control system and you can  !!!!
# !!! back out the changes easily.                                            !!!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

import argparse
import os
import sys

parser = argparse.ArgumentParser()
parser.add_argument("path", nargs="+", help="Shape file to process")
parser.add_argument(
    "--move",
    metavar="<group1>:<group2>",
    help="Move attributes from group 1 into group 2",
)
parser.add_argument("--delete", metavar="prop", help="Delete the given attribute")
parser.add_argument("--set", metavar="prop=value", help="Set property value")
parser.add_argument(
    "--print", action="store_true", help="If set attributes are printed as discovered"
)
parser.add_argument(
    "--print-all", action="store_true", help="If set Shape attributes are also printed"
)
parser.add_argument(
    "--print-groups",
    action="store_true",
    help="If set all custom property groups are printed",
)
parser.add_argument(
    "--save-changes", action="store_true", help="Unless specified the file is not saved"
)
parser.add_argument(
    "--freecad", help="Directory FreeCAD binaries (libFreeCAD.so) if not installed"
)
args = parser.parse_args()

if args.freecad:
    sys.path.append(args.freecad)

import FreeCAD
import Path
import Path.Base.PropertyBag as PathPropertyBag
import Path.Base.Util as PathUtil

set_var = None
set_val = None

GroupMap = {}
if args.move:
    g = args.move.split(":")
    if len(g) != 2:
        print("ERROR: {} not a valid group mapping".format(args.move))
        sys.exit(1)
    GroupMap[g[0]] = g[1]

if args.set:
    s = args.set.split("=")
    if len(s) != 2:
        print("ERROR: {} not a valid group mapping".format(args.move))
        sys.exit(1)
    set_var = s[0]
    set_val = s[1]

for i, fname in enumerate(args.path):
    # print(fname)
    doc = FreeCAD.openDocument(fname, False)
    print("{}:".format(doc.Name))
    for o in doc.Objects:
        if PathPropertyBag.IsPropertyBag(o):
            if args.print_groups:
                print("  {}:  {}".format(o.Label, sorted(o.CustomPropertyGroups)))
            else:
                print("  {}:".format(o.Label))
            for p in o.Proxy.getCustomProperties():
                grp = o.getGroupOfProperty(p)
                typ = o.getTypeIdOfProperty(p)
                ttp = PathPropertyBag.getPropertyTypeName(typ)
                val = PathUtil.getProperty(o, p)
                dsc = o.getDocumentationOfProperty(p)
                enm = ""
                enum = []
                if ttp == "Enumeration":
                    enum = o.getEnumerationsOfProperty(p)
                    enm = "{}".format(",".join(enum))
                if GroupMap.get(grp):
                    group = GroupMap.get(grp)
                    print("move: {}.{} -> {}".format(grp, p, group))
                    o.removeProperty(p)
                    o.Proxy.addCustomProperty(typ, p, group, dsc)
                    if enum:
                        print("enum {}.{}: {}".format(group, p, enum))
                        setattr(o, p, enum)
                    PathUtil.setProperty(o, p, val)
                if p == set_var:
                    print("set {}.{} = {}".format(grp, p, set_val))
                    if ttp == "Enumeration" and set_val[0] == "[":
                        enum = set_val[1:-1].split(",")
                        setattr(o, p, enum)
                    else:
                        PathUtil.setProperty(o, p, set_val)
                if p == args.delete:
                    print("delete {}.{}".format(grp, p))
                    o.removeProperty(p)
                if not args.print_all and grp == "Shape":
                    continue
                if args.print or args.print_all:
                    print(
                        "    {:10} {:20} {:20} {:10} {}".format(
                            grp, p, ttp, str(val), enm
                        )
                    )
            o.Proxy.refreshCustomPropertyGroups()
    if args.save_changes:
        doc.recompute()
        doc.save()
    FreeCAD.closeDocument(doc.Name)

print("-done-")
