#***************************************************************************
#*   Copyright (c) 2019 Dion Moult <dion@thinkmoult.com>                   *
#*   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
#*   Copyright (c) 2019 FreeCAD Developers                                 *
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

"""Provides the IFC schema data as dicts, by loading the JSON schema files.

Provides the data as IfcContexts, IfcProducts and IfcTypes.
"""

import os
import json

import FreeCAD

ifcVersions = ["IFC4", "IFC2X3"]
IfcVersion = ifcVersions[FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Arch").GetInt("IfcVersion",0)]

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_contexts_" + IfcVersion + ".json")) as f:
    IfcContexts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_products_" + IfcVersion + ".json")) as f:
    IfcProducts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "Arch", "Presets",
"ifc_types_" + IfcVersion + ".json")) as f:
    IfcTypes = json.load(f)
