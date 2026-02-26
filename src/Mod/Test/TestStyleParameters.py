# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                          *
# *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                    *
# *                                                                          *
# *   This file is part of FreeCAD.                                          *
# *                                                                          *
# *   FreeCAD is free software: you can redistribute it and/or modify it     *
# *   under the terms of the GNU Lesser General Public License as            *
# *   published by the Free Software Foundation, either version 2.1 of the   *
# *   License, or (at your option) any later version.                        *
# *                                                                          *
# *   FreeCAD is distributed in the hope that it will be useful, but         *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
# *   Lesser General Public License for more details.                        *
# *                                                                          *
# *   You should have received a copy of the GNU Lesser General Public       *
# *   License along with FreeCAD. If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                       *
# *                                                                          *
# ***************************************************************************

"""Python integration tests for FreeCADGui.StyleParameters and FreeCADGui.Color."""

import unittest

import FreeCAD
import FreeCADGui


class TestStyleParametersModule(unittest.TestCase):
    """Tests for the FreeCADGui.StyleParameters module structure."""

    def testModuleExists(self):
        self.assertTrue(hasattr(FreeCADGui, "StyleParameters"))

    def testModuleHasResolve(self):
        self.assertTrue(callable(FreeCADGui.StyleParameters.resolve))

    def testModuleHasEvaluate(self):
        self.assertTrue(callable(FreeCADGui.StyleParameters.evaluate))

    def testModuleHasNumericType(self):
        self.assertTrue(hasattr(FreeCADGui.StyleParameters, "Numeric"))

    def testModuleHasTupleType(self):
        self.assertTrue(hasattr(FreeCADGui.StyleParameters, "Tuple"))

    def testColorRegisteredOnGui(self):
        self.assertTrue(hasattr(FreeCADGui, "Color"))


class TestNumericEvaluation(unittest.TestCase):
    """Tests for Numeric values returned by FreeCADGui.StyleParameters.evaluate()."""

    def setUp(self):
        self.sp = FreeCADGui.StyleParameters

    def testEvaluateNumericWithUnit(self):
        result = self.sp.evaluate("10px")
        self.assertIsInstance(result, self.sp.Numeric)
        self.assertAlmostEqual(result.value, 10.0)
        self.assertEqual(result.unit, "px")

    def testEvaluateNumericAddition(self):
        result = self.sp.evaluate("2px + 3px")
        self.assertAlmostEqual(result.value, 5.0)
        self.assertEqual(result.unit, "px")

    def testEvaluateDimensionless(self):
        result = self.sp.evaluate("42")
        self.assertAlmostEqual(result.value, 42.0)
        self.assertEqual(result.unit, "")

    def testEvaluateScalarMultiplication(self):
        result = self.sp.evaluate("10px * 3")
        self.assertAlmostEqual(result.value, 30.0)
        self.assertEqual(result.unit, "px")

    def testFloatConversion(self):
        result = self.sp.evaluate("7.5px")
        self.assertAlmostEqual(float(result), 7.5)

    def testNumericReprWithUnit(self):
        result = self.sp.evaluate("10px")
        text = repr(result)
        self.assertIn("10", text)
        self.assertIn("px", text)

    def testNumericReprWithoutUnit(self):
        result = self.sp.evaluate("5")
        text = repr(result)
        self.assertIn("5", text)
        self.assertNotIn("px", text)


class TestColorEvaluation(unittest.TestCase):
    """Tests for Color values returned by FreeCADGui.StyleParameters.evaluate()."""

    def setUp(self):
        self.sp = FreeCADGui.StyleParameters

    def testEvaluateRed(self):
        result = self.sp.evaluate("#ff0000")
        self.assertIsInstance(result, FreeCADGui.Color)
        self.assertAlmostEqual(result.r, 1.0, places=5)
        self.assertAlmostEqual(result.g, 0.0, places=5)
        self.assertAlmostEqual(result.b, 0.0, places=5)

    def testEvaluateBlack(self):
        result = self.sp.evaluate("#000000")
        self.assertAlmostEqual(result.r, 0.0, places=5)
        self.assertAlmostEqual(result.g, 0.0, places=5)
        self.assertAlmostEqual(result.b, 0.0, places=5)

    def testEvaluateWhite(self):
        result = self.sp.evaluate("#ffffff")
        self.assertAlmostEqual(result.r, 1.0, places=5)
        self.assertAlmostEqual(result.g, 1.0, places=5)
        self.assertAlmostEqual(result.b, 1.0, places=5)

    def testColorAlphaDefaultsToOne(self):
        result = self.sp.evaluate("#00ff00")
        self.assertAlmostEqual(result.a, 1.0, places=5)

    def testColorRepr(self):
        result = self.sp.evaluate("#ff0000")
        text = repr(result)
        self.assertIn("Color", text)


class TestTupleEvaluation(unittest.TestCase):
    """Tests for Tuple values returned by FreeCADGui.StyleParameters.evaluate()."""

    def setUp(self):
        self.sp = FreeCADGui.StyleParameters

    def testEvaluateUnnamedTuple(self):
        result = self.sp.evaluate("(10px, 20px)")
        self.assertIsInstance(result, self.sp.Tuple)
        self.assertEqual(result.kind, "Generic")
        self.assertEqual(len(result), 2)

    def testUnnamedTuplePositionalAccess(self):
        result = self.sp.evaluate("(10px, 20px)")
        first = result[0]
        self.assertAlmostEqual(first.value, 10.0)
        self.assertEqual(first.unit, "px")
        second = result[1]
        self.assertAlmostEqual(second.value, 20.0)

    def testUnnamedTupleIndexOutOfBounds(self):
        result = self.sp.evaluate("(10px, 20px)")
        with self.assertRaises(IndexError):
            _ = result[10]

    def testEvaluateNamedTuple(self):
        result = self.sp.evaluate("(top: 10px, right: 20px, bottom: 5px, left: 15px)")
        self.assertIsInstance(result, self.sp.Tuple)
        self.assertEqual(len(result), 4)

    def testNamedTupleStringSubscript(self):
        result = self.sp.evaluate("(top: 10px, right: 20px, bottom: 5px, left: 15px)")
        top = result["top"]
        self.assertAlmostEqual(top.value, 10.0)
        self.assertEqual(top.unit, "px")

    def testNamedTupleStringSubscriptMissing(self):
        result = self.sp.evaluate("(top: 10px)")
        with self.assertRaises(KeyError):
            _ = result["nonexistent"]

    def testNamedTupleAttributeAccess(self):
        result = self.sp.evaluate("(top: 10px, right: 20px)")
        self.assertAlmostEqual(result.top.value, 10.0)
        self.assertAlmostEqual(result.right.value, 20.0)

    def testNamedTupleAttributeMissing(self):
        result = self.sp.evaluate("(top: 10px)")
        with self.assertRaises(AttributeError):
            _ = result.nonexistent_attribute

    def testTupleFindReturnsValueWhenFound(self):
        result = self.sp.evaluate("(top: 10px, right: 20px)")
        found = result.find("top")
        self.assertIsNotNone(found)
        self.assertAlmostEqual(found.value, 10.0)

    def testTupleFindReturnsNoneWhenMissing(self):
        result = self.sp.evaluate("(top: 10px)")
        self.assertIsNone(result.find("nonexistent"))

    def testTupleKindAttribute(self):
        result = self.sp.evaluate("(10px, 20px)")
        self.assertEqual(result.kind, "Generic")


class TestErrorHandling(unittest.TestCase):
    """Tests for error handling in FreeCADGui.StyleParameters."""

    def setUp(self):
        self.sp = FreeCADGui.StyleParameters

    def testEvaluateIncompleteExpression(self):
        with self.assertRaises(RuntimeError):
            self.sp.evaluate("10px +")

    def testEvaluateMixedUnits(self):
        with self.assertRaises(RuntimeError):
            self.sp.evaluate("10px + 20rem")

    def testResolveUnknownParameterReturnsNone(self):
        result = self.sp.resolve("__NonExistentParameter__xyz__")
        self.assertIsNone(result)

    def testResolveTakesExactlyOneArgument(self):
        with self.assertRaises(TypeError):
            self.sp.resolve()

    def testEvaluateTakesExactlyOneArgument(self):
        with self.assertRaises(TypeError):
            self.sp.evaluate()
