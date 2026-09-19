# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for the property alias mechanism, doubling as usage examples.

Renaming a property breaks add-ons and macros that used the old name. An alias keeps the
old name working. In Python both steps happen at runtime, and the order matters:

    obj.addPropertyAlias("SurfaceArea", "Area", deprecated=True, since="1.1")
    obj.addProperty("App::PropertyArea", "SurfaceArea", "Geometry")

addPropertyAlias() must run first, while "SurfaceArea" does not yet exist, so it can find
and migrate a leftover "Area" from an older document. Calling addProperty() first creates a
fresh "SurfaceArea" with a default value, so addPropertyAlias() then finds the canonical
name already taken and declines to migrate, silently orphaning the user's value under the
old name. Both calls are otherwise safe to make unconditionally on every load. See
LegacyAreaFeature below for the complete pattern including document restore.
"""

import os
import tempfile
import unittest

import FreeCAD


class LegacyAreaFeature:
    """Renaming a Python property from 'Area' to 'SurfaceArea'.

    Documents saved by an older version stored 'Area'. Registering the alias migrates them
    in place and keeps 'Area' working for scripts that still use the old name.
    """

    def __init__(self, obj):
        obj.Proxy = self
        # Register the alias before adding the property: on a document saved before the
        # rename, obj may already carry a dynamic "Area" property, and addPropertyAlias()
        # only migrates it while "SurfaceArea" does not exist yet. In practice __init__ is
        # never called on restore (only onDocumentRestored() is, see below), so this
        # ordering never actually observes a leftover "Area" here -- but it follows the
        # documented recipe so the class stays a correct example, and so copying this
        # pattern elsewhere (e.g. into a constructor that *is* reachable on restore)
        # doesn't silently reintroduce the divergence bug.
        self.registerAliases(obj)
        obj.addProperty("App::PropertyArea", "SurfaceArea", "Geometry")

    def onDocumentRestored(self, obj):
        self.registerAliases(obj)

    def registerAliases(self, obj):
        # Aliases are runtime state and are not saved, so this must run on every load.
        obj.addPropertyAlias("SurfaceArea", "Area", deprecated=True, since="1.1")

    @classmethod
    def attachToLegacyObject(cls, obj):
        """Attach as Proxy without ever calling __init__, mirroring how FreeCAD actually
        restores a FeaturePython object: __init__ is not invoked on restore, only
        onDocumentRestored() is, via registerAliases(). This lets a test exercise the real
        migration path -- an object still carrying a dynamic "Area" property from before
        the rename -- without a full save/reload cycle.
        """
        self = cls.__new__(cls)
        obj.Proxy = self
        self.registerAliases(obj)
        return self


class PropertyAliasBasics(unittest.TestCase):
    """The alias API as an add-on author sees it."""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PropertyAliasBasics")
        self.obj = self.Doc.addObject("App::VarSet", "Vars")
        self.obj.addProperty("App::PropertyInteger", "NewName", "Variables")
        self.obj.NewName = 42
        self.obj.addPropertyAlias("NewName", "OldName", True, "1.1")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testAliasReadsAndWritesCanonical(self):
        self.assertEqual(self.obj.OldName, 42)

        self.obj.OldName = 7

        self.assertEqual(self.obj.NewName, 7)

    def testHasattrSeesAlias(self):
        self.assertTrue(hasattr(self.obj, "OldName"))

    def testPropertiesListIsCanonicalOnly(self):
        # Aliases are deliberately absent, so deprecated names do not appear in the
        # property editor or in dir().
        self.assertIn("NewName", self.obj.PropertiesList)
        self.assertNotIn("OldName", self.obj.PropertiesList)

    def testGetPropertyAliasesReportsCanonicalDeprecatedAndSince(self):
        aliases = self.obj.getPropertyAliases()

        self.assertIn("OldName", aliases)
        self.assertEqual(aliases["OldName"]["canonical"], "NewName")
        self.assertTrue(aliases["OldName"]["deprecated"])
        self.assertEqual(aliases["OldName"]["since"], "1.1")

    def testAddPropertyOnAliasNameIsNoOp(self):
        # The idiom add-ons use everywhere. Before aliases were tolerated this raised.
        if "OldName" not in self.obj.PropertiesList:
            self.obj.addProperty("App::PropertyInteger", "OldName", "Variables")

        self.assertEqual(self.obj.NewName, 42)
        self.assertNotIn("OldName", self.obj.PropertiesList)

    def testAddPropertyOnAliasNameWithWrongTypeRaises(self):
        with self.assertRaises(Exception):
            self.obj.addProperty("App::PropertyString", "OldName", "Variables")

    def testInstanceAliasOverridesClassAlias(self):
        # A runtime alias shadows a class-level one of the same name. Nothing tests this
        # precedence today, so a reversed merge order would go unnoticed.
        feature = self.Doc.addObject("App::FeatureTest", "Feature")
        feature.addProperty("App::PropertyInteger", "Other", "Variables")

        feature.addPropertyAlias("Other", "AliasPlain")

        aliases = feature.getPropertyAliases()
        self.assertEqual(aliases["AliasPlain"]["canonical"], "Other")


class PropertyAliasMigration(unittest.TestCase):
    """The rename recipe, including the document round trip it exists for."""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PropertyAliasMigration")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testRegisteringAliasMigratesExistingProperty(self):
        obj = self.Doc.addObject("App::VarSet", "Vars")
        obj.addProperty("App::PropertyInteger", "Area", "Geometry")
        obj.Area = 17

        obj.addPropertyAlias("SurfaceArea", "Area", True, "1.1")

        self.assertIn("SurfaceArea", obj.PropertiesList)
        self.assertNotIn("Area", obj.PropertiesList)
        self.assertEqual(obj.SurfaceArea, 17)
        self.assertEqual(obj.Area, 17)

    def testMigrationIsIdempotent(self):
        obj = self.Doc.addObject("App::VarSet", "Vars")
        obj.addProperty("App::PropertyInteger", "Area", "Geometry")
        obj.Area = 17

        obj.addPropertyAlias("SurfaceArea", "Area", True, "1.1")
        obj.addPropertyAlias("SurfaceArea", "Area", True, "1.1")

        self.assertEqual(obj.SurfaceArea, 17)
        self.assertIn("SurfaceArea", obj.PropertiesList)

    def testMigrationLeavesUnrelatedUserPropertyAlone(self):
        obj = self.Doc.addObject("App::VarSet", "Vars")
        obj.addProperty("App::PropertyInteger", "Area", "Geometry")
        obj.addProperty("App::PropertyInteger", "MyOwnThing", "Geometry")
        obj.MyOwnThing = 5

        obj.addPropertyAlias("SurfaceArea", "Area", True, "1.1")

        self.assertIn("MyOwnThing", obj.PropertiesList)
        self.assertEqual(obj.MyOwnThing, 5)

    def testMigrationSurvivesSaveAndReload(self):
        obj = self.Doc.addObject("App::FeaturePython", "Legacy")
        LegacyAreaFeature(obj)
        obj.SurfaceArea = 250.0

        saveName = os.path.join(tempfile.gettempdir(), "PropertyAliasMigration.FCStd")
        self.Doc.saveAs(saveName)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.open(saveName)

        restored = self.Doc.Legacy
        self.assertIn("SurfaceArea", restored.PropertiesList)
        self.assertAlmostEqual(restored.SurfaceArea.Value, 250.0)
        # The old name still works for scripts that have not migrated.
        self.assertAlmostEqual(restored.Area.Value, 250.0)

    def testMigrationFromDocumentSavedUnderOldName(self):
        # Unlike testMigrationSurvivesSaveAndReload above, this builds a document that
        # actually contains the pre-rename layout: a dynamic property literally named
        # "Area", with no "SurfaceArea" anywhere yet. This is what a document saved
        # before the rename shipped actually looks like on disk, and it is the case the
        # Python migration path exists for.
        obj = self.Doc.addObject("App::FeaturePython", "Legacy")
        obj.addProperty("App::PropertyArea", "Area", "Geometry")
        obj.Area = 250.0

        # Attach the Proxy the way restore actually does it: via onDocumentRestored(),
        # never __init__(), so "SurfaceArea" is never created ahead of the migration.
        LegacyAreaFeature.attachToLegacyObject(obj)

        self.assertIn("SurfaceArea", obj.PropertiesList)
        self.assertNotIn("Area", obj.PropertiesList)
        self.assertAlmostEqual(obj.SurfaceArea.Value, 250.0)
        # The old name still reads through the alias.
        self.assertAlmostEqual(obj.Area.Value, 250.0)


class PropertyAliasStatic(unittest.TestCase):
    """Aliases declared by a C++ class, seen from Python.

    App::FeatureTest declares AliasTarget with the aliases AliasPlain and AliasDeprecated.
    """

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PropertyAliasStatic")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testStaticAliasResolves(self):
        obj = self.Doc.addObject("App::FeatureTest", "Feature")
        obj.AliasTarget = 11

        self.assertEqual(obj.AliasPlain, 11)
        self.assertEqual(obj.AliasDeprecated, 11)

    def testStaticAliasAppliesToEveryInstance(self):
        first = self.Doc.addObject("App::FeatureTest", "First")
        second = self.Doc.addObject("App::FeatureTest", "Second")
        first.AliasTarget = 1
        second.AliasTarget = 2

        self.assertEqual(first.AliasPlain, 1)
        self.assertEqual(second.AliasPlain, 2)

    def testWriteThroughStaticAliasPersistsUnderCanonicalName(self):
        # Demonstrates the deliberate forward-compatibility break: the file records the new
        # name, so an older FreeCAD cannot read this property.
        obj = self.Doc.addObject("App::FeatureTest", "Feature")
        obj.AliasPlain = 33

        self.assertIn("AliasTarget", obj.Content)
        self.assertNotIn("AliasPlain", obj.Content)

    def testStaticAliasSurvivesSaveAndReload(self):
        obj = self.Doc.addObject("App::FeatureTest", "Feature")
        obj.AliasPlain = 44

        saveName = os.path.join(tempfile.gettempdir(), "PropertyAliasStatic.FCStd")
        self.Doc.saveAs(saveName)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.open(saveName)

        self.assertEqual(self.Doc.Feature.AliasTarget, 44)
        self.assertEqual(self.Doc.Feature.AliasPlain, 44)


class PropertyAliasExpressions(unittest.TestCase):
    """Expressions referencing an alias, and how they are healed on load."""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PropertyAliasExpressions")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testExpressionUsingAliasEvaluates(self):
        source = self.Doc.addObject("App::FeatureTest", "Source")
        target = self.Doc.addObject("App::FeatureTest", "Target")
        source.AliasTarget = 9
        target.setExpression("Integer", "Source.AliasDeprecated")

        self.Doc.recompute()

        self.assertEqual(target.Integer, 9)

    def testExpressionIsCanonicalizedOnReload(self):
        source = self.Doc.addObject("App::FeatureTest", "Source")
        target = self.Doc.addObject("App::FeatureTest", "Target")
        source.AliasTarget = 9
        target.setExpression("Integer", "Source.AliasDeprecated")
        self.Doc.recompute()

        saveName = os.path.join(tempfile.gettempdir(), "PropertyAliasExpressions.FCStd")
        self.Doc.saveAs(saveName)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.open(saveName)

        expressions = dict(self.Doc.Target.ExpressionEngine)
        self.assertIn("AliasTarget", expressions["Integer"])
        self.assertNotIn("AliasDeprecated", expressions["Integer"])

    def testReloadDoesNotTouchDocument(self):
        source = self.Doc.addObject("App::FeatureTest", "Source")
        target = self.Doc.addObject("App::FeatureTest", "Target")
        source.AliasTarget = 9
        target.setExpression("Integer", "Source.AliasDeprecated")
        self.Doc.recompute()

        saveName = os.path.join(tempfile.gettempdir(), "PropertyAliasReload.FCStd")
        self.Doc.saveAs(saveName)
        FreeCAD.closeDocument(self.Doc.Name)
        self.Doc = FreeCAD.open(saveName)

        # Opening a document must never mark it as modified.
        self.assertFalse(self.Doc.isTouched())


class PropertyAliasDocument(unittest.TestCase):
    """App::Document is a PropertyContainer too."""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PropertyAliasDocument")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def testDocumentPropertyAlias(self):
        self.Doc.Comment = "hello"
        self.Doc.addPropertyAlias("Comment", "OldComment", True, "1.1")

        self.assertEqual(self.Doc.OldComment, "hello")
        self.assertIn("OldComment", self.Doc.getPropertyAliases())
