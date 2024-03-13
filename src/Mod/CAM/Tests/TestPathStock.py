# -*- coding: utf-8 -*-
# ***************************************************************************
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

import FreeCAD
import Path.Main.Stock as PathStock

from Tests.PathTestUtils import PathTestBase


class FakeJobProxy:
    def baseObject(self, obj):
        return obj.Base


R = 223.606798 / 2


class TestPathStock(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathStock")
        self.base = self.doc.addObject("Part::Box", "Box")
        self.base.Length = 100
        self.base.Width = 200
        self.base.Height = 300
        self.job = self.doc.addObject("App::FeaturePython", "Job")
        self.job.addProperty("App::PropertyLink", "Model")
        model = FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup", "Model")
        model.addObject(self.base)
        self.job.Model = model
        self.job.Proxy = FakeJobProxy()

    def tearDown(self):
        FreeCAD.closeDocument("TestPathStock")

    def test00(self):
        """Test CreateBox"""

        stock = PathStock.CreateBox(self.job)
        self.assertTrue(hasattr(stock, "Length"))
        self.assertTrue(hasattr(stock, "Width"))
        self.assertTrue(hasattr(stock, "Height"))
        self.assertEqual(100, stock.Length)
        self.assertEqual(200, stock.Width)
        self.assertEqual(300, stock.Height)

        extent = FreeCAD.Vector(17, 13, 77)
        stock = PathStock.CreateBox(self.job, extent)
        self.assertEqual(17, stock.Length)
        self.assertEqual(13, stock.Width)
        self.assertEqual(77, stock.Height)

        placement = FreeCAD.Placement(
            FreeCAD.Vector(-3, 88, 4), FreeCAD.Vector(0, 0, 1), 180
        )
        stock = PathStock.CreateBox(self.job, extent, placement)
        self.assertEqual(17, stock.Length)
        self.assertEqual(13, stock.Width)
        self.assertEqual(77, stock.Height)
        self.assertPlacement(placement, stock.Placement)

    def test01(self):
        """Test CreateCylinder"""

        stock = PathStock.CreateCylinder(self.job)
        self.assertTrue(hasattr(stock, "Radius"))
        self.assertTrue(hasattr(stock, "Height"))
        self.assertRoughly(R, stock.Radius.Value)
        self.assertEqual(300, stock.Height)

        stock = PathStock.CreateCylinder(self.job, 37, 24)
        self.assertEqual(37, stock.Radius)
        self.assertEqual(24, stock.Height)

        placement = FreeCAD.Placement(
            FreeCAD.Vector(3, 8, -4), FreeCAD.Vector(0, 0, 1), -90
        )
        stock = PathStock.CreateCylinder(self.job, 1, 88, placement)
        self.assertEqual(1, stock.Radius)
        self.assertEqual(88, stock.Height)
        self.assertPlacement(placement, stock.Placement)

    def test10(self):
        """Verify FromTemplate box creation."""

        extent = FreeCAD.Vector(17, 13, 77)
        placement = FreeCAD.Placement(
            FreeCAD.Vector(3, 8, -4), FreeCAD.Vector(0, 0, 1), -90
        )
        orig = PathStock.CreateBox(self.job, extent, placement)

        # collect full template
        template = PathStock.TemplateAttributes(orig)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateBox, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.Length, stock.Length)
        self.assertEqual(orig.Width, stock.Width)
        self.assertEqual(orig.Height, stock.Height)
        self.assertPlacement(orig.Placement, stock.Placement)

        # don't store extent in template
        template = PathStock.TemplateAttributes(orig, False, True)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateBox, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(100, stock.Length)
        self.assertEqual(200, stock.Width)
        self.assertEqual(300, stock.Height)
        self.assertPlacement(orig.Placement, stock.Placement)

        # don't store placement in template
        template = PathStock.TemplateAttributes(orig, True, False)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateBox, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.Length, stock.Length)
        self.assertEqual(orig.Width, stock.Width)
        self.assertEqual(orig.Height, stock.Height)
        self.assertPlacement(FreeCAD.Placement(), stock.Placement)

    def test11(self):
        """Verify FromTemplate cylinder creation."""
        radius = 7
        height = 12
        placement = FreeCAD.Placement(
            FreeCAD.Vector(99, 88, 77), FreeCAD.Vector(1, 1, 1), 123
        )
        orig = PathStock.CreateCylinder(self.job, radius, height, placement)

        # full template
        template = PathStock.TemplateAttributes(orig)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateCylinder, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.Radius, stock.Radius)
        self.assertEqual(orig.Height, stock.Height)
        self.assertPlacement(orig.Placement, stock.Placement)

        # no extent in template
        template = PathStock.TemplateAttributes(orig, False, True)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateCylinder, PathStock.StockType.FromStock(stock)
        )
        self.assertRoughly(R, stock.Radius.Value)
        self.assertEqual(300, stock.Height)
        self.assertPlacement(orig.Placement, stock.Placement)

        # no placement template - and no base
        template = PathStock.TemplateAttributes(orig, True, False)
        stock = PathStock.CreateFromTemplate(None, template)

        self.assertEqual(
            PathStock.StockType.CreateCylinder, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.Radius, stock.Radius)
        self.assertEqual(orig.Height, stock.Height)
        self.assertPlacement(FreeCAD.Placement(), stock.Placement)

        # no placement template - but base
        template = PathStock.TemplateAttributes(orig, True, False)
        stock = PathStock.CreateFromTemplate(self.job, template)

        self.assertEqual(
            PathStock.StockType.CreateCylinder, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.Radius, stock.Radius)
        self.assertEqual(orig.Height, stock.Height)
        self.assertPlacement(
            FreeCAD.Placement(FreeCAD.Vector(50, 100, 0), FreeCAD.Rotation()),
            stock.Placement,
        )

    def test12(self):
        """Verify FromTemplate from Base creation."""
        neg = FreeCAD.Vector(1, 2, 3)
        pos = FreeCAD.Vector(9, 8, 7)
        orig = PathStock.CreateFromBase(self.job, neg, pos)

        # Make sure we have a different base object for the creation
        self.base.Length = 11
        self.base.Width = 12
        self.base.Height = 13

        # full template
        template = PathStock.TemplateAttributes(orig)
        stock = PathStock.CreateFromTemplate(self.job, template)
        self.assertEqual(
            PathStock.StockType.FromBase, PathStock.StockType.FromStock(stock)
        )
        self.assertEqual(orig.ExtXneg, stock.ExtXneg)
        self.assertEqual(orig.ExtXpos, stock.ExtXpos)
        self.assertEqual(orig.ExtYneg, stock.ExtYneg)
        self.assertEqual(orig.ExtYpos, stock.ExtYpos)
        self.assertEqual(orig.ExtZneg, stock.ExtZneg)
        self.assertEqual(orig.ExtZpos, stock.ExtZpos)

        bb = stock.Shape.BoundBox
        self.assertEqual(neg.x + pos.x + 11, bb.XLength)
        self.assertEqual(neg.y + pos.y + 12, bb.YLength)
        self.assertEqual(neg.z + pos.z + 13, bb.ZLength)
