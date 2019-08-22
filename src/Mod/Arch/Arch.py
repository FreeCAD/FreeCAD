#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *
#*   Yorik van Havre <yorik@uncreated.net>                                 *
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

__title__= "FreeCAD Arch API"
__author__ = "Yorik van Havre"
__url__ = "http://www.freecadweb.org"

## \defgroup ARCH Arch
#  \ingroup PYTHONWORKBENCHES
#  \brief Architecture and BIM tools
#
#  This module provides tools specialized in Building Information Modeling (BIM).
#  such as convenience tools to build walls, windows or structures, and
#  IFC import/export capabilities.

'''The Arch module provides tools specialized in BIM modeling.'''

import FreeCAD
if FreeCAD.GuiUp:
	import FreeCADGui
	FreeCADGui.updateLocale()

from ArchWall import *
from ArchFloor import *
from ArchFence import *
from ArchProject import *
from ArchSite import *
from ArchBuilding import *
from ArchStructure import *
from ArchProfile import *
from ArchCommands import *
from ArchSectionPlane import *
from ArchWindow import *
from ArchAxis import *
from ArchRoof import *
from ArchSpace import *
from ArchStairs import *
from ArchRebar import *
from ArchFrame import *
from ArchPanel import *
from ArchEquipment import *
from ArchCutPlane import *
from ArchMaterial import *
from ArchSchedule import *
from ArchPrecast import *
from ArchPipe import *
from ArchBuildingPart import *
from ArchReference import *
