# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

import Path

from PathTests.PathTestUtils import PathTestBase

class TestPathTool(PathTestBase):

    def test00(self):
        '''Verify templateAttrs'''

        name = 'tool 1'
        mat  = 'Carbide'
        typ  = 'EndMill'
        dia    = 1.7
        flat   = 7.2
        offset = 3.2
        corner = 4
        height = 45.3
        angle  = 118

        tool = Path.Tool()
        tool.Name = name
        tool.ToolType = typ
        tool.Material = mat
        tool.Diameter = dia
        tool.LengthOffset = offset
        tool.FlatRadius = flat
        tool.CornerRadius = corner
        tool.CuttingEdgeAngle = angle
        tool.CuttingEdgeHeight = height

        attrs = tool.templateAttrs()
        self.assertEqual(attrs['name'], name)
        self.assertEqual(attrs['diameter'], dia)
        self.assertEqual(attrs['material'], mat)
        self.assertEqual(attrs['tooltype'], typ)
        self.assertEqual(attrs['lengthOffset'], offset)
        self.assertEqual(attrs['flatRadius'], flat)
        self.assertEqual(attrs['cornerRadius'], corner)
        self.assertEqual(attrs['cuttingEdgeAngle'], angle)
        self.assertEqual(attrs['cuttingEdgeHeight'], height)
        return tool


    def test01(self):
        '''Verify template roundtrip'''

        t0 = self.test00()
        t1 = Path.Tool()
        t1.setFromTemplate(t0.templateAttrs())

        self.assertEqual(t0.Name, t1.Name)
        self.assertEqual(t0.ToolType, t1.ToolType)
        self.assertEqual(t0.Material, t1.Material)
        self.assertEqual(t0.Diameter, t1.Diameter)
        self.assertEqual(t0.LengthOffset, t1.LengthOffset)
        self.assertEqual(t0.FlatRadius, t1.FlatRadius)
        self.assertEqual(t0.CornerRadius, t1.CornerRadius)
        self.assertEqual(t0.CuttingEdgeAngle, t1.CuttingEdgeAngle)
        self.assertEqual(t0.CuttingEdgeHeight, t1.CuttingEdgeHeight)

    def test02(self):
        '''Verify template dictionary construction'''

        t0 = self.test00()
        t1 = Path.Tool(t0.templateAttrs())

        self.assertEqual(t0.Name, t1.Name)
        self.assertEqual(t0.ToolType, t1.ToolType)
        self.assertEqual(t0.Material, t1.Material)
        self.assertEqual(t0.Diameter, t1.Diameter)
        self.assertEqual(t0.LengthOffset, t1.LengthOffset)
        self.assertEqual(t0.FlatRadius, t1.FlatRadius)
        self.assertEqual(t0.CornerRadius, t1.CornerRadius)
        self.assertEqual(t0.CuttingEdgeAngle, t1.CuttingEdgeAngle)
        self.assertEqual(t0.CuttingEdgeHeight, t1.CuttingEdgeHeight)

