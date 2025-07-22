# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2019 Dion Moult <dion@thinkmoult.com>                   *
# *   Copyright (c) 2019 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2019 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Provides the IFC schema data as dicts, by loading the JSON schema files.

Provides the data as IfcContexts, IfcProducts and IfcTypes.
"""

import json
import os

import FreeCAD

from draftutils import params

ifcVersions = ["IFC4", "IFC2X3"]
IfcVersion = ifcVersions[params.get_param_arch("IfcVersion")]

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM", "Presets",
"ifc_contexts_" + IfcVersion + ".json")) as f:
    IfcContexts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM", "Presets",
"ifc_products_" + IfcVersion + ".json")) as f:
    IfcProducts = json.load(f)

with open(os.path.join(FreeCAD.getResourceDir(), "Mod", "BIM", "Presets",
"ifc_types_" + IfcVersion + ".json")) as f:
    IfcTypes = json.load(f)
