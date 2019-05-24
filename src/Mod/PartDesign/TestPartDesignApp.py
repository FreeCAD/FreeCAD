#   (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2011      LGPL        *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD PartDesign module
#---------------------------------------------------------------------------

# datum tools
from PartDesignTests.TestDatum import TestDatumPoint, TestDatumLine, TestDatumPlane
from PartDesignTests.TestShapeBinder import TestShapeBinder

# additive/subtractive features & primitives
from PartDesignTests.TestPad import TestPad
from PartDesignTests.TestPocket import TestPocket
from PartDesignTests.TestHole import TestHole
from PartDesignTests.TestRevolve import TestRevolve
from PartDesignTests.TestPipe import TestPipe
#from PartDesignTests.TestLoft import TestLoft
from PartDesignTests.TestPrimitive import TestPrimitive

# transformations and boolean
from PartDesignTests.TestMirrored import TestMirrored
from PartDesignTests.TestLinearPattern import TestLinearPattern
from PartDesignTests.TestPolarPattern import TestPolarPattern
from PartDesignTests.TestMultiTransform import TestMultiTransform
from PartDesignTests.TestBoolean import TestBoolean

# dressup features
from PartDesignTests.TestFillet import TestFillet
from PartDesignTests.TestChamfer import TestChamfer
from PartDesignTests.TestDraft import TestDraft
from PartDesignTests.TestThickness import TestThickness
