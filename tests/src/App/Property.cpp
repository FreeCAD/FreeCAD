// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>      *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <gtest/gtest.h>

#include <FCConfig.h>

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Interpreter.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/Link.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>

#include <src/App/InitApplication.h>

#include <xercesc/util/PlatformUtils.hpp>

#include "Property.h"

TEST(PropertyLink, TestSetValues)
{
    App::PropertyLinkSubList prop;
    std::vector<App::DocumentObject*> objs {nullptr, nullptr};
    std::vector<const char*> subs {"Sub1", "Sub2"};
    prop.setValues(objs, subs);
    const auto& sub = prop.getSubValues();
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], "Sub1");
    EXPECT_EQ(sub[1], "Sub2");
}

class PropertyFloatTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
    }
};

TEST_F(PropertyFloatTest, testWriteRead)
{
#if defined(FC_OS_LINUX) || defined(FC_OS_BSD)
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");  // avoid rounding of floating point numbers
#endif
    double value = 1.2345;
    App::PropertyFloat prop;
    prop.setValue(value);
    Base::StringWriter writer;
    prop.Save(writer);

    std::string str = "<?xml version='1.0' encoding='utf-8'?>\n";
    str.append("<Property name='Length' type='App::PropertyFloat'>\n");
    str.append(writer.getString());
    str.append("</Property>\n");

    std::stringstream data(str);
    Base::XMLReader reader("Document.xml", data);
    App::PropertyFloat prop2;
    prop2.Restore(reader);
    EXPECT_DOUBLE_EQ(prop2.getValue(), value);
}

PROPERTY_SOURCE(tests::HookRecordingContainer, App::PropertyContainer)

App::Document* PropertyAliasStatic::doc {nullptr};

App::Document* PropertyAlias::doc {nullptr};

App::Document* RenameProperty::doc {nullptr};

// Tests that a class-level alias is visible on every instance of the class, which is only
// true if it is stored in PropertyData rather than in a per-instance map.
TEST_F(PropertyAliasStatic, staticAliasResolvesOnEveryInstance)
{
    EXPECT_EQ(first->getPropertyByName("AliasPlain"), &first->AliasTarget);
    EXPECT_EQ(second->getPropertyByName("AliasPlain"), &second->AliasTarget);
}

// Tests that an alias declared on a base class is reachable through a derived class, via
// the parentPropertyData chain. App::FeatureTestException derives from App::FeatureTest.
TEST_F(PropertyAliasStatic, staticAliasInheritedBySubclass)
{
    auto* derived = freecad_cast<App::FeatureTestException*>(
        doc->addObject("App::FeatureTestException", "Derived")
    );
    ASSERT_NE(derived, nullptr);
    EXPECT_EQ(derived->getPropertyByName("AliasPlain"), &derived->AliasTarget);
    doc->removeObject(derived->getNameInDocument());
}

// Tests that an alias resolves to the same property as the canonical name.
TEST_F(PropertyAlias, aliasResolvesToCanonicalProperty)
{
    varSet->addPropertyAlias("NewName", "OldName");

    App::Property* byCanonical = varSet->getPropertyByName("NewName");
    App::Property* byAlias = varSet->getPropertyByName("OldName");

    ASSERT_NE(byCanonical, nullptr);
    EXPECT_EQ(byAlias, byCanonical);
}

// Tests that a non-deprecated alias emits no warning.
TEST_F(PropertyAlias, nonDeprecatedAliasEmitsNoWarning)
{
    varSet->addPropertyAlias("NewName", "OldName");

    WarningCapture capture;
    varSet->getPropertyByName("OldName");

    EXPECT_TRUE(capture.warnings.empty());
}

// Tests that a deprecated alias still resolves but emits a warning.
TEST_F(PropertyAlias, deprecatedAliasEmitsWarningAndResolves)
{
    varSet->addPropertyAlias("NewName", "OldDeprecated", App::PropertyAliasType::Deprecated);

    WarningCapture capture;
    App::Property* prop = varSet->getPropertyByName("OldDeprecated");

    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop, varSet->getPropertyByName("NewName"));
    ASSERT_EQ(capture.warnings.size(), 1u);
    EXPECT_NE(capture.warnings[0].find("OldDeprecated"), std::string::npos);
    EXPECT_NE(capture.warnings[0].find("NewName"), std::string::npos);
}

// Tests that an unknown name still returns nullptr (no regression).
TEST_F(PropertyAlias, unknownNameReturnsNullptr)
{
    App::Property* prop = varSet->getPropertyByName("DoesNotExist");

    EXPECT_EQ(prop, nullptr);
}

// Tests that an alias works for a static property (Label is inherited from DocumentObject).
TEST_F(PropertyAlias, aliasForStaticProperty)
{
    varSet->addPropertyAlias("Label", "OldLabel");

    App::Property* byCanonical = varSet->getPropertyByName("Label");
    App::Property* byAlias = varSet->getPropertyByName("OldLabel");

    ASSERT_NE(byCanonical, nullptr);
    EXPECT_EQ(byAlias, byCanonical);
}

// Tests that Python attribute access via an alias returns the correct value.
TEST_F(PropertyAlias, pythonAttributeAccessViaAlias)
{
    varSet->addPropertyAlias("NewName", "OldName");

    std::string cmd = std::string("vs = App.getDocument('") + doc->getName() + "').getObject('"
        + varSet->getNameInDocument()
        + "')\n"
          "val = vs.OldName\n"
          "assert val == 42, f'Expected 42, got {val}'";
    Base::Interpreter().runString(cmd.c_str());
}

// Tests that Python getPropertyByName() resolves aliases.
TEST_F(PropertyAlias, pythonGetPropertyByNameViaAlias)
{
    varSet->addPropertyAlias("NewName", "OldName");

    std::string cmd = std::string("vs = App.getDocument('") + doc->getName() + "').getObject('"
        + varSet->getNameInDocument()
        + "')\n"
          "p1 = vs.getPropertyByName('NewName')\n"
          "p2 = vs.getPropertyByName('OldName')\n"
          "assert p1 == p2, f'Alias must resolve to same value, got {p1} vs {p2}'";
    Base::Interpreter().runString(cmd.c_str());
}

// Tests that addPropertyAlias is callable from Python.
TEST_F(PropertyAlias, pythonAddPropertyAlias)
{
    std::string cmd = std::string("vs = App.getDocument('") + doc->getName() + "').getObject('"
        + varSet->getNameInDocument()
        + "')\n"
          "vs.addPropertyAlias('NewName', 'PyAlias')\n"
          "p = vs.getPropertyByName('PyAlias')\n"
          "assert p is not None, 'Alias registered from Python must resolve'";
    Base::Interpreter().runString(cmd.c_str());

    EXPECT_EQ(varSet->getPropertyByName("PyAlias"), varSet->getPropertyByName("NewName"));
}

// Tests that a real property is never shadowed by an alias of the same name.
TEST_F(PropertyAlias, realPropertyTakesPrecedenceOverAlias)
{
    varSet->addDynamicProperty("App::PropertyInteger", "OldName", "Variables");
    varSet->addPropertyAlias("NewName", "OldName");

    App::Property* byName = varSet->getPropertyByName("OldName");

    ASSERT_NE(byName, nullptr);
    EXPECT_NE(byName, dynProp);
    EXPECT_STREQ(byName->getName(), "OldName");
}

// Tests that writing through an alias updates the canonical property.
TEST_F(PropertyAlias, settingValueThroughAliasUpdatesCanonical)
{
    varSet->addPropertyAlias("NewName", "OldName");

    auto* viaAlias = freecad_cast<App::PropertyInteger*>(varSet->getPropertyByName("OldName"));
    ASSERT_NE(viaAlias, nullptr);
    viaAlias->setValue(99);

    EXPECT_EQ(dynProp->getValue(), 99);
}

// Tests that a cyclic alias pair terminates instead of recursing forever.
TEST_F(PropertyAlias, aliasChainDoesNotRecurse)
{
    varSet->addPropertyAlias("Missing", "AlsoMissing");
    varSet->addPropertyAlias("AlsoMissing", "Missing");

    EXPECT_EQ(varSet->getPropertyByName("Missing"), nullptr);
    EXPECT_EQ(varSet->getPropertyByName("AlsoMissing"), nullptr);
}

// Tests that a runtime alias shadows a class-level alias of the same name.
TEST_F(PropertyAliasStatic, instanceAliasOverridesClassAlias)
{
    first->addDynamicProperty("App::PropertyInteger", "Other", "Variables");
    first->addPropertyAlias("Other", "AliasPlain");

    App::Property* resolved = first->getPropertyByName("AliasPlain");

    ASSERT_NE(resolved, nullptr);
    EXPECT_STREQ(resolved->getName(), "Other");
    // The other instance is unaffected — the override is per object.
    EXPECT_EQ(second->getPropertyByName("AliasPlain"), &second->AliasTarget);
}

// Tests that the deprecation warning names the version the alias was introduced in.
TEST_F(PropertyAliasStatic, warningIncludesSinceVersion)
{
    WarningCapture capture;
    first->getPropertyByName("AliasDeprecated");

    ASSERT_EQ(capture.warnings.size(), 1U);
    EXPECT_NE(capture.warnings[0].find("since 1.1"), std::string::npos);
    EXPECT_NE(capture.warnings[0].find("AliasTarget"), std::string::npos);
}

// Tests that a deprecated alias warns only once per container, so an alias referenced from an
// expression does not flood the report view on every recompute.
TEST_F(PropertyAliasStatic, warningEmittedOncePerContainerPerAlias)
{
    WarningCapture capture;
    first->getPropertyByName("AliasDeprecated");
    first->getPropertyByName("AliasDeprecated");
    first->getPropertyByName("AliasDeprecated");

    EXPECT_EQ(capture.warnings.size(), 1U);
}

// Tests that the once-per-container budget is not shared between objects, so every call site
// the developer needs to fix is reported.
TEST_F(PropertyAliasStatic, warningEmittedForEachContainerSeparately)
{
    WarningCapture capture;
    first->getPropertyByName("AliasDeprecated");
    second->getPropertyByName("AliasDeprecated");

    EXPECT_EQ(capture.warnings.size(), 2U);
}

// Tests that a non-deprecated alias is silent no matter how often it is used.
TEST_F(PropertyAliasStatic, nonDeprecatedAliasNeverWarns)
{
    WarningCapture capture;
    first->getPropertyByName("AliasPlain");
    first->getPropertyByName("AliasPlain");

    EXPECT_TRUE(capture.warnings.empty());
}

// Tests that the property reports its canonical name even when reached through an alias, so
// code that round-trips through getName() does not resurrect the old name.
TEST_F(PropertyAlias, getPropertyNameReturnsCanonicalAfterAliasLookup)
{
    varSet->addPropertyAlias("NewName", "OldName");

    App::Property* byAlias = varSet->getPropertyByName("OldName");

    ASSERT_NE(byAlias, nullptr);
    EXPECT_STREQ(byAlias->getName(), "NewName");
    EXPECT_STREQ(varSet->getPropertyName(byAlias), "NewName");
}

// Tests that aliases stay out of the property map, so the property editor and dir() are
// unaffected by them.
TEST_F(PropertyAlias, getPropertyMapExcludesAliases)
{
    varSet->addPropertyAlias("NewName", "OldName");

    std::map<std::string, App::Property*> propertyMap;
    varSet->getPropertyMap(propertyMap);

    EXPECT_TRUE(propertyMap.contains("NewName"));
    EXPECT_FALSE(propertyMap.contains("OldName"));
}

// Tests that resolving a class-level alias leaves the per-instance overlay empty. It must
// still fail if a regression made class aliases get copied into the per-instance map.
TEST_F(PropertyAliasStatic, classAliasDoesNotPopulateInstanceOverlay)
{
    App::Property* resolved = first->getPropertyByName("AliasPlain");

    ASSERT_EQ(resolved, &first->AliasTarget);
    EXPECT_FALSE(first->hasInstanceAliases());
}

// Tests that aliases work on App::Document, which is a PropertyContainer too.
TEST_F(PropertyAliasDocument, documentContainerSupportsAliases)
{
    doc->addPropertyAlias("Comment", "OldComment");

    App::Property* byAlias = doc->getPropertyByName("OldComment");

    ASSERT_NE(byAlias, nullptr);
    EXPECT_EQ(byAlias, &doc->Comment);
}

// Tests that an alias can point at a property provided by an extension rather than by the
// container itself. Support -> AttachmentSupport in AttachExtension is exactly this shape.
TEST_F(PropertyAliasExtension, aliasResolvesToExtensionProperty)
{
    App::Property* canonical = group->getPropertyByName("Group");
    ASSERT_NE(canonical, nullptr);

    group->addPropertyAlias("Group", "OldGroup");

    EXPECT_EQ(group->getPropertyByName("OldGroup"), canonical);
}

// Tests that a deprecated alias onto an extension property still warns, so the diagnostic is
// not silently lost for the case that motivated this fix.
TEST_F(PropertyAliasExtension, deprecatedAliasToExtensionPropertyWarns)
{
    group->addPropertyAlias("Group", "OldGroup", App::PropertyAliasType::Deprecated, "1.1");

    WarningCapture capture;
    App::Property* resolved = group->getPropertyByName("OldGroup");

    ASSERT_NE(resolved, nullptr);
    ASSERT_EQ(capture.warnings.size(), 1U);
    EXPECT_NE(capture.warnings[0].find("since 1.1"), std::string::npos);
}

// Tests that PropertyLookupMode::WithoutAliases is honoured when the property is provided by an
// extension (LinkBaseExtension::extensionGetPropertyByName), not just when it lives directly on
// the container. Alias resolution internally re-looks-up the canonical property using
// WithoutAliases to guarantee that a chained or cyclic alias cannot recurse forever; if the mode
// is silently dropped somewhere along the extension chain, that guarantee is lost.
TEST_F(PropertyAliasExtension, extensionLookupRespectsWithoutAliases)
{
    auto* link = freecad_cast<App::Link*>(doc->addObject("App::Link", "LinkObject"));
    ASSERT_NE(link, nullptr);

    App::Property* canonical = link->addDynamicProperty("App::PropertyInteger", "Canonical", "Base");
    ASSERT_NE(canonical, nullptr);

    link->addPropertyAlias("Canonical", "Old");

    EXPECT_EQ(link->getPropertyByName("Old", App::PropertyLookupMode::WithAliases), canonical);
    EXPECT_EQ(link->getPropertyByName("Old", App::PropertyLookupMode::WithoutAliases), nullptr);
}

// Tests that a property saved under its old name restores into the renamed property with no
// handleChangedPropertyName override — the central claim of the alias mechanism.
TEST_F(PropertyAliasRestore, restoresOldNameWithNoHandleChangedPropertyNameOverride)
{
    tests::HookRecordingContainer container;
    restoreInto(container, makeDocument("OldName", "App::PropertyInteger", "<Integer value='7'/>\n"));

    EXPECT_EQ(container.Renamed.getValue(), 7);
    EXPECT_TRUE(container.nameHookCalls.empty());
}

// Tests that restoring through an alias emits no deprecation warning. The file was written by
// an older version; the user did nothing to deprecate.
TEST_F(PropertyAliasRestore, restoreThroughAliasIsSilent)
{
    tests::HookRecordingContainer container;

    WarningCapture capture;
    restoreInto(container, makeDocument("OldName", "App::PropertyInteger", "<Integer value='7'/>\n"));

    EXPECT_TRUE(capture.warnings.empty());
}

// Tests that an alias whose saved type disagrees with the canonical property reaches the type
// hook rather than being force-read into the wrong property.
TEST_F(PropertyAliasRestore, mismatchedTypeFallsBackToHandleChangedPropertyType)
{
    tests::HookRecordingContainer container;
    restoreInto(container, makeDocument("OldName", "App::PropertyString", "<String value='seven'/>\n"));

    ASSERT_EQ(container.typeHookCalls.size(), 1U);
    EXPECT_EQ(container.typeHookCalls[0], "App::PropertyString");
    EXPECT_TRUE(container.nameHookCalls.empty());
}

// Tests that a name matching no property and no alias still reaches the legacy hook, so
// subclasses with existing migration logic do not regress.
TEST_F(PropertyAliasRestore, unmatchedNameStillReachesHandleChangedPropertyName)
{
    tests::HookRecordingContainer container;
    restoreInto(container, makeDocument("Unrelated", "App::PropertyInteger", "<Integer value='7'/>\n"));

    ASSERT_EQ(container.nameHookCalls.size(), 1U);
    EXPECT_EQ(container.nameHookCalls[0], "Unrelated");
}

// Tests whether we can rename a property
TEST_F(RenameProperty, simple)
{
    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}

// Tests whether we can rename a property from Python
TEST_F(RenameProperty, fromPython)
{
    // Act
    Base::Interpreter().runString(
        "App.ActiveDocument.getObject('VarSet').renameProperty('Variable', 'NewName')"
    );

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}

// Tests whether we can rename a property in a chain
TEST_F(RenameProperty, chain)
{
    // Act 1
    bool isRenamed = varSet->renameDynamicProperty(prop, "Name1");

    // Assert 1
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Name1");
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Name1"), prop);

    // Act 2
    auto prop1 = freecad_cast<App::PropertyInteger*>(varSet->getDynamicPropertyByName("Name1"));
    isRenamed = varSet->renameDynamicProperty(prop1, "Name2");

    // Assert 2
    EXPECT_TRUE(isRenamed);
    EXPECT_EQ(prop, prop1);
    EXPECT_STREQ(varSet->getPropertyName(prop1), "Name2");
    EXPECT_EQ(varSet->getDynamicPropertyByName("Name1"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Name2"), prop1);
}

// Tests whether we can rename a static property
TEST_F(RenameProperty, staticProperty)
{
    // Arrange
    App::Property* prop = varSet->getPropertyByName("Label");

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "MyLabel");

    // Assert
    EXPECT_FALSE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Label");
    EXPECT_EQ(varSet->getDynamicPropertyByName("MyLabel"), nullptr);
}

// Tests whether we can rename a static property from Python
TEST_F(RenameProperty, staticPropertyFromPython)
{
    // Arrange
    App::Property* prop = varSet->getPropertyByName("Label");

    // Act / Assert
    EXPECT_THROW(
        Base::Interpreter().runString(
            "App.ActiveDocument.getObject('VarSet006').renameProperty('Label', 'NewName')"
        ),
        Base::Exception
    );

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Label");
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}

// Tests whether we can rename a locked property
TEST_F(RenameProperty, lockedProperty)
{
    // Arrange
    prop->setStatus(App::Property::LockDynamic, true);

    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop, "NewName"), Base::RuntimeError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}

// Tests that the Skip policy neither renames a locked property nor throws
TEST_F(RenameProperty, lockedPropertySkipPolicyDoesNotRename)
{
    // Arrange
    prop->setStatus(App::Property::LockDynamic, true);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName", App::RenameLockedPolicy::Skip);

    // Assert
    EXPECT_FALSE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}

// Tests that RenameLockedPolicy::Skip on a locked property with a bound expression leaves
// the binding intact under the original name, rather than destroying it. The rename itself
// is declined, so a binding re-created under the (never taken) new name would point at
// nothing.
TEST_F(RenameProperty, skipPolicyPreservesBoundExpressions)
{
    // Arrange
    prop->setStatus(App::Property::LockDynamic, true);
    App::ObjectIdentifier path(*prop);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "1 + 1"));
    varSet->setExpression(path, expr);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName", App::RenameLockedPolicy::Skip);

    // Assert
    EXPECT_FALSE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");

    auto expressions = varSet->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1U);
    EXPECT_EQ(expressions.begin()->first.getProperty(), prop);
}

// Tests whether we can rename to a property that already exists
TEST_F(RenameProperty, toExistingProperty)
{
    // Arrange
    App::Property* prop2 = varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables");

    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop2, "Variable"), Base::NameError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_STREQ(varSet->getPropertyName(prop2), "Variable2");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable2"), prop2);
}

// Tests whether we can rename to a property that is invalid
TEST_F(RenameProperty, toInvalidProperty)
{
    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop, "0Variable"), Base::NameError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("0Variable"), nullptr);
}

// Tests whether we can rename a property that is used in an expression in the same container
TEST_F(RenameProperty, updateExpressionSameContainer)
{
    // Arrange
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "Variable"));
    varSet->setExpression(path, expr);
    varSet->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(prop2->getValue(), value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), value);
}

// Tests whether we can rename a property that is used in an expression in a different container
TEST_F(RenameProperty, updateExpressionDifferentContainer)
{
    // Arrange
    auto* varSet2 = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet2"));
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "VarSet.Variable"));
    varSet2->setExpression(path, expr);
    varSet2->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(prop2->getValue(), value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet2->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), value);

    // Tear down
    doc->removeObject(varSet2->getNameInDocument());
}

// Tests whether we can rename a property that is used in an expression in a different document
TEST_F(RenameProperty, updateExpressionDifferentDocument)
{
    // Arrange
    std::string docName = App::GetApplication().getUniqueDocumentName("test2");
    App::Document* doc2 = App::GetApplication().newDocument(docName.c_str(), "testUser");

    auto* varSet2 = freecad_cast<App::VarSet*>(doc2->addObject("App::VarSet", "VarSet2"));
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "test#VarSet.Variable"));
    doc->saveAs("test.FCStd");
    doc2->saveAs("test2.FCStd");
    varSet2->setExpression(path, expr);
    varSet2->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(prop2->getValue(), value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet2->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), value);

    // Tear down
    doc2->removeObject(varSet2->getNameInDocument());
    App::GetApplication().closeDocument(doc2->getName());
}

// Test if we can rename a property which value is the result of an expression
TEST_F(RenameProperty, withExpression)
{
    // Arrange
    auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );
    prop2->setValue(value);

    App::ObjectIdentifier path(*prop);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "Variable2"));
    varSet->setExpression(path, expr);
    varSet->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop2->getValue(), value);
    EXPECT_EQ(prop->getValue(), value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act
    prop2->setValue(value + 1);
    varSet->ExpressionEngine.execute();

    // Assert
    EXPECT_EQ(prop2->getValue(), value + 1);
    EXPECT_EQ(prop->getValue(), value + 1);
}

// Tests whether we can rename a property and undo it
TEST_F(RenameProperty, undo)
{
    // Act
    bool isRenamed = false;
    {
        doc->openTransaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
        doc->commitTransaction();
    }

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act: Undo the rename
    bool undone = doc->undo();

    // Assert: The property should be back to its original name and value
    EXPECT_TRUE(undone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}


// Tests whether we can rename a property, undo, and redo it
TEST_F(RenameProperty, redo)
{
    // Act
    bool isRenamed = false;
    {
        doc->openTransaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
        doc->commitTransaction();
    }

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act: Undo the rename
    bool undone = doc->undo();

    // Assert: The property should be back to its original name and value
    EXPECT_TRUE(undone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);

    // Act: Redo the rename
    bool redone = doc->redo();
    EXPECT_TRUE(redone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}

/*
 * For these tests we have the following variables that correspond to the
 * following names:
 *
 * The documents:
 * - doc1: "test"
 * - doc2: "test1"
 *
 * The VarSet objects in doc1 are:
 * - varSet1Doc1: "VarSet"
 * - varSet2Doc1: "VarSet001"
 *
 * The VarSet object in doc2 is:
 * - varSetDoc2: "VarSet"
 *
 * The property to move:
 * - prop: "Variable" and is an integer with value 123 and is initially in varSet1Doc1.
 */

void MoveProperty::assertMovedProperty(App::Property* property, App::DocumentObject* target)

{
    ASSERT_TRUE(property != nullptr);
    EXPECT_EQ(property->getContainer(), target);
    EXPECT_EQ(varSet1Doc1->getDynamicPropertyByName("Variable"), nullptr);

    auto* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        target->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), value);
}

// Helper function to test moving a property
void MoveProperty::testMoveProperty(App::DocumentObject* target)
{
    // Act
    App::Property* movedProp = varSet1Doc1->moveDynamicProperty(prop, target);

    assertMovedProperty(movedProp, target);
    // Assert
}

// Tests whether we can move a property to a different container
// test#VarSet.Variable -> test#VarSet001.Variable
TEST_F(MoveProperty, simple)
{
    testMoveProperty(varSet2Doc1);
}

// Tests whether we can move a property to a container in a different document
// test#VarSet.Variable -> test1#VarSet.Variable
TEST_F(MoveProperty, otherDoc)
{
    testMoveProperty(varSetDoc2);
}

// Tests whether we can move a static property
// test#Cube.Length -> FAIL
TEST_F(MoveProperty, staticProperty)
{
    // Arrange
    App::DocumentObject* cube = doc1->addObject("Part::Box", "Cube");
    App::Property* prop = cube->getPropertyByName("Length");

    // Act
    EXPECT_THROW(varSet1Doc1->moveDynamicProperty(prop, varSet2Doc1), Base::RuntimeError);

    // Assert
    EXPECT_EQ(cube->getPropertyByName("Length"), prop);
    EXPECT_EQ(varSet2Doc1->getDynamicPropertyByName("Length"), nullptr);

    // Tear down
    doc1->removeObject(cube->getNameInDocument());
}

// Tests whether we can move a static property
// test#VarSet.Variable (locked) -> FAIL
TEST_F(MoveProperty, lockedProperty)
{
    // Arrange
    prop->setStatus(App::Property::LockDynamic, true);

    // Act / Assert
    EXPECT_THROW(varSet1Doc1->moveDynamicProperty(prop, varSet2Doc1), Base::RuntimeError);
    EXPECT_EQ(varSet1Doc1->getPropertyByName("Variable"), prop);
}

// Tests whether we can move to a property that already exists
// test#VarSet.Variable -> test#VarSet001.Variable (already existing): FAIL
TEST_F(MoveProperty, toExistingProperty)
{
    // Arrange
    App::Property* prop2
        = varSet2Doc1->addDynamicProperty("App::PropertyInteger", "Variable", "Variables");

    // Act / Assert
    EXPECT_THROW(varSet1Doc1->moveDynamicProperty(prop, varSet2Doc1), Base::NameError);

    EXPECT_EQ(varSet1Doc1->getPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet2Doc1->getPropertyByName("Variable"), prop2);
}

void MoveProperty::testMovePropertyExpressionWithAct(
    App::DocumentObject* sourceProp2,
    App::DocumentObject* target,
    const char* exprString,
    const std::function<App::Property*()>& act
)
{
    // Arrange
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        sourceProp2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet1Doc1, "Variable"));
    doc1->saveAs("test.FCStd");
    doc2->saveAs("test1.FCStd");

    sourceProp2->setExpression(path, expr);
    sourceProp2->ExpressionEngine.execute();

    // Assert before the move
    EXPECT_EQ(prop->getValue(), value);
    EXPECT_EQ(prop2->getValue(), value);

    // Act
    App::Property* movedProp = act();
    sourceProp2->ExpressionEngine.execute();

    // Assert after the move
    ASSERT_TRUE(movedProp != nullptr);
    EXPECT_EQ(varSet1Doc1->getPropertyByName("Variable"), nullptr);

    auto* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        target->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), value);
    EXPECT_STREQ(
        sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
        exprString
    );
}

void MoveProperty::testMovePropertyExpression(
    App::DocumentObject* sourceProp2,
    App::DocumentObject* target,
    const char* exprString
)
{
    auto act = [this, target]() -> App::Property* {
        return varSet1Doc1->moveDynamicProperty(prop, target);
    };
    testMovePropertyExpressionWithAct(sourceProp2, target, exprString, act);
}

// Tests whether we can move a property that is used in an expression in the
// originating container
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = VarSet001.Variable
TEST_F(MoveProperty, updateExpressionOriginatingContainer)
{
    testMovePropertyExpression(varSet1Doc1, varSet2Doc1, "VarSet001.Variable");
}

// Tests whether we can move a property that is used in an expression in the
// target container
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet001.Variable2 = VarSet.Variable -> test#VarSet001.Variable2 = Variable
TEST_F(MoveProperty, updateExpressionTargetContainer)
{
    testMovePropertyExpression(varSet2Doc1, varSet2Doc1, "Variable");
}

// Tests whether we can move a property to another document that is used in an
// expression in the originating container
// test#VarSet.Variable -> test1#VarSet.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = test1#VarSet.Variable
TEST_F(MoveProperty, updateExpressionOriginatingContainerOtherDoc)
{
    testMovePropertyExpression(varSet1Doc1, varSetDoc2, "test1#VarSet.Variable");
}

// Tests whether we can move a property to another document that is used in an
// expression in the target container
// test#VarSet.Variable -> test1#VarSet.Variable where
// test1#VarSet.Variable2 = test#VarSet.Variable -> test1#VarSet.Variable2 = Variable
TEST_F(MoveProperty, updateExpressionTargetContainerOtherDoc)
{
    testMovePropertyExpression(varSetDoc2, varSetDoc2, "Variable");
}

// Tests whether we can move a property that obtains its value from an expression.
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet.Variable = Variable2 -> test#VarSet001.Variable = VarSet.Variable2
TEST_F(MoveProperty, updateExpressionMovedProp)
{
    // Arrange
    auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet1Doc1->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );
    int valueVar2 = 10;
    prop2->setValue(valueVar2);

    App::ObjectIdentifier path(*prop);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet1Doc1, "Variable2"));
    varSet1Doc1->setExpression(path, expr);
    varSet1Doc1->ExpressionEngine.execute();

    // Assert before the move
    EXPECT_EQ(prop->getValue(), valueVar2);
    EXPECT_EQ(prop2->getValue(), valueVar2);

    // Act
    App::Property* movedProp = nullptr;
    movedProp = varSet1Doc1->moveDynamicProperty(prop, varSet2Doc1);
    varSet1Doc1->ExpressionEngine.execute();
    varSet2Doc1->ExpressionEngine.execute();

    // Assert after the move
    ASSERT_TRUE(movedProp != nullptr);
    EXPECT_EQ(varSet1Doc1->getPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet1Doc1->ExpressionEngine.getExpressions().size(), 0);

    auto* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        varSet2Doc1->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), valueVar2);

    std::map<App::ObjectIdentifier, const App::Expression*> expressions
        = varSet2Doc1->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1);
    EXPECT_STREQ(expressions.begin()->first.getPropertyName().c_str(), "Variable");
    EXPECT_STREQ(expressions.begin()->second->toString().c_str(), "VarSet.Variable2");
}

void MoveProperty::testUndoProperty(App::DocumentObject* target)
{
    // Act
    App::Property* movedProp = nullptr;
    {
        doc1->openTransaction("Move Property");
        movedProp = varSet1Doc1->moveDynamicProperty(prop, target);
        doc1->commitTransaction();
    }

    // Assert
    assertMovedProperty(movedProp, target);

    // Act: Undo the move
    bool undone = doc1->undo();

    // Assert: The property should be back to its original container and value
    EXPECT_TRUE(undone);
    auto* originalProp = freecad_cast<App::PropertyInteger*>(
        varSet1Doc1->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(originalProp != nullptr);
    EXPECT_EQ(originalProp->getValue(), value);
    EXPECT_EQ(target->getPropertyByName("Variable"), nullptr);
}

// Tests whether we can move a property and undo it
// test#VarSet.Variable -> test#VarSet001.Variable and back
TEST_F(MoveProperty, undo)
{
    testUndoProperty(varSet2Doc1);
}

// Tests whether we can move a property to a container in a different document
// test#VarSet.Variable -> test1#VarSet.Variable and back
TEST_F(MoveProperty, undoOtherDoc)
{
    testUndoProperty(varSetDoc2);
}

void MoveProperty::testUndoMovePropertyExpression(
    App::DocumentObject* sourceProp2,
    App::DocumentObject* target,
    const char* exprString,
    const char* exprStringAfterUndo
)
{
    // Arrange
    auto act = [this, target] {
        App::Property* movedProp = nullptr;
        {
            doc1->openTransaction("Move Property");
            movedProp = varSet1Doc1->moveDynamicProperty(prop, target);
            doc1->commitTransaction();
        }
        return movedProp;
    };


    testMovePropertyExpressionWithAct(sourceProp2, target, exprString, act);

    // Act: Undo the move
    bool undone = doc1->undo();

    doc1->recompute();
    doc2->recompute();
    sourceProp2->ExpressionEngine.execute();

    // Assert
    EXPECT_TRUE(undone);
    auto* originalProp = freecad_cast<App::PropertyInteger*>(
        varSet1Doc1->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(originalProp != nullptr);
    EXPECT_EQ(originalProp->getValue(), value);
    EXPECT_STREQ(
        sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
        exprStringAfterUndo
    );

    EXPECT_EQ(target->getPropertyByName("Variable"), nullptr);
}

// Tests whether we can undo a move of a property that is used in an expression
// in the originating container.
//
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = VarSet001.Variable
// and back
TEST_F(MoveProperty, undoExpressionOriginatingContainer)
{
    testUndoMovePropertyExpression(varSet1Doc1, varSet2Doc1, "VarSet001.Variable", "Variable");
}

// Tests whether we can undo a move of a property that is used in an expression
// in the target container.
//
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet001.Variable2 = VarSet.Variable -> test#VarSet001.Variable2 = Variable
// and back
TEST_F(MoveProperty, undoExpressionTargetContainer)
{
    testUndoMovePropertyExpression(varSet2Doc1, varSet2Doc1, "Variable", "VarSet.Variable");
}

// Tests whether we can undo a move of a property that is used in an expression
// in the originating container.
//
// test#VarSet.Variable -> test1#VarSet.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = test1#VarSet.Variable
// and back
TEST_F(MoveProperty, undoExpressionOriginatingContainerOtherDoc)
{
    testUndoMovePropertyExpression(varSet1Doc1, varSetDoc2, "test1#VarSet.Variable", "Variable");
}

// Tests whether we can undo a move of a property that is used in an expression
// in the target container.
//
// test#VarSet.Variable -> test1#VarSet.Variable where
// test1#VarSet.Variable2 = test#VarSet.Variable -> test1#VarSet.Variable2 = Variable
// and back
TEST_F(MoveProperty, undoExpressionTargetContainerOtherDoc)
{
    testUndoMovePropertyExpression(varSetDoc2, varSetDoc2, "Variable", "test#VarSet.Variable");
}

// Tests whether we can undo and redo a property move
//
// test#VarSet.Variable -> test#VarSet001.Variable and back and back again.
TEST_F(MoveProperty, redoSimple)
{
    testUndoProperty(varSet2Doc1);
    // Act: Redo the move
    bool redone = doc1->redo();

    // Assert: The property should be moved to the new container again
    EXPECT_TRUE(redone);
    App::Property* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        varSet2Doc1->getDynamicPropertyByName("Variable")
    );
    assertMovedProperty(movedPropWithType, varSet2Doc1);
}

// Tests whether we can undo and redo a property move to a different document
//
// test#VarSet.Variable -> test1#VarSet001.Variable and back and back again.
TEST_F(MoveProperty, redoOtherDoc)
{
    testUndoProperty(varSetDoc2);

    // Act: Redo the move
    bool redone = doc1->redo();
    doc1->recompute();
    doc2->recompute();

    // Assert: The property should be moved to the new container again
    EXPECT_TRUE(redone);
    App::Property* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        varSetDoc2->getDynamicPropertyByName("Variable")
    );
    assertMovedProperty(movedPropWithType, varSetDoc2);
}

void MoveProperty::testRedoMovePropertyExpression(
    App::DocumentObject* sourceProp2,
    App::DocumentObject* target,
    const char* exprString,
    const char* exprStringAfterUndo
)
{
    testUndoMovePropertyExpression(sourceProp2, target, exprString, exprStringAfterUndo);

    bool redone = doc1->redo();
    doc1->recompute();
    doc2->recompute();
    sourceProp2->ExpressionEngine.execute();

    // Assert: The property should be moved to the target container again
    EXPECT_TRUE(redone);
    auto* movedPropWithType = freecad_cast<App::PropertyInteger*>(
        target->getDynamicPropertyByName("Variable")
    );
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), value);
    EXPECT_STREQ(
        sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
        exprString
    );
}

// Tests whether we can undo and redo a move of a property that is used in an
// expression in the originating container.
//
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = VarSet001.Variable
// and back and back again.
TEST_F(MoveProperty, redoExpressionOriginatingContainer)
{
    testRedoMovePropertyExpression(varSet1Doc1, varSet2Doc1, "VarSet001.Variable", "Variable");
}

// Tests whether we can undo and redo a move of a property that is used in an expression
// in the target container.
//
// test#VarSet.Variable -> test#VarSet001.Variable where
// test#VarSet001.Variable2 = VarSet.Variable -> test#VarSet001.Variable2 = Variable
// and back and back again.
TEST_F(MoveProperty, redoExpressionTargetContainer)
{
    testRedoMovePropertyExpression(varSet2Doc1, varSet2Doc1, "Variable", "VarSet.Variable");
}

// Tests whether we can undo and redo a move of a property that is used in an
// expression in the originating container.
//
// test#VarSet.Variable -> test1#VarSet.Variable where
// test#VarSet.Variable2 = Variable -> test#VarSet.Variable2 = test1#VarSet.Variable
// and back and back again.
TEST_F(MoveProperty, redoExpressionOriginatingContainerOtherDoc)
{
    testRedoMovePropertyExpression(varSet1Doc1, varSetDoc2, "test1#VarSet.Variable", "Variable");
}

// Tests whether we can undo and redo a move of a property that is used in an
// expression in the target container.
//
// test#VarSet.Variable -> test1#VarSet.Variable where
// test1#VarSet.Variable2 = test#VarSet.Variable -> test1#VarSet.Variable2 = Variable
// and back and back again
TEST_F(MoveProperty, redoExpressionTargetContainerOtherDoc)
{
    testRedoMovePropertyExpression(varSetDoc2, varSetDoc2, "Variable", "test#VarSet.Variable");
}

// Tests the add-on compatibility case: code guarded on PropertiesList membership will call
// addProperty with the old name, and must get the canonical property back rather than an error.
TEST_F(PropertyAliasStatic, addOnAliasNameReturnsCanonical)
{
    App::Property* added = nullptr;
    ASSERT_NO_THROW(
        added = first->addDynamicProperty("App::PropertyInteger", "AliasPlain", "Variables")
    );

    EXPECT_EQ(added, &first->AliasTarget);
}

// Tests that a type disagreement is reported rather than silently returning the wrong property.
TEST_F(PropertyAliasStatic, addOnAliasNameWithWrongTypeThrows)
{
    EXPECT_THROW(
        first->addDynamicProperty("App::PropertyString", "AliasPlain", "Variables"),
        Base::TypeError
    );
}

// Tests that colliding with a real property still throws, unchanged.
TEST_F(PropertyAliasStatic, addOnRealNameStillThrows)
{
    EXPECT_THROW(
        first->addDynamicProperty("App::PropertyInteger", "AliasTarget", "Variables"),
        Base::NameError
    );
}

// Tests that enumeration reports both class-level and instance-level aliases with their
// metadata, which is what a retirement audit needs.
TEST_F(PropertyAliasStatic, getPropertyAliasesMergesClassAndInstance)
{
    first->addDynamicProperty("App::PropertyInteger", "Runtime", "Variables");
    first->addPropertyAlias("Runtime", "RuntimeAlias", App::PropertyAliasType::Normal, "1.2");

    auto aliases = first->getPropertyAliases();

    ASSERT_TRUE(aliases.contains("AliasDeprecated"));
    EXPECT_EQ(aliases["AliasDeprecated"].canonicalName, "AliasTarget");
    EXPECT_EQ(aliases["AliasDeprecated"].since, "1.1");
    EXPECT_EQ(aliases["AliasDeprecated"].type, App::PropertyAliasType::Deprecated);

    ASSERT_TRUE(aliases.contains("RuntimeAlias"));
    EXPECT_EQ(aliases["RuntimeAlias"].canonicalName, "Runtime");
    EXPECT_EQ(aliases["RuntimeAlias"].since, "1.2");
    EXPECT_EQ(aliases["RuntimeAlias"].type, App::PropertyAliasType::Normal);
}

// Tests the Python migration path: registering an alias renames a dynamic property left over
// from an older document, preserving its value.
TEST_F(PropertyAlias, registrationRenamesExistingDynamicProperty)
{
    auto* legacy = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "LegacyName", "Variables")
    );
    ASSERT_NE(legacy, nullptr);
    legacy->setValue(17);

    varSet->addPropertyAlias("CanonicalName", "LegacyName", App::PropertyAliasType::Deprecated, "1.1");

    auto* migrated = freecad_cast<App::PropertyInteger*>(varSet->getPropertyByName("CanonicalName"));
    ASSERT_NE(migrated, nullptr);
    EXPECT_EQ(migrated, legacy);
    EXPECT_EQ(migrated->getValue(), 17);
    EXPECT_STREQ(migrated->getName(), "CanonicalName");
}

// Tests that an already migrated container is left alone, so the recipe is safe to run on
// every load.
TEST_F(PropertyAlias, registrationSkipsWhenCanonicalExists)
{
    auto* legacy = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "LegacyName", "Variables")
    );
    legacy->setValue(17);
    auto* canonical = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "CanonicalName", "Variables")
    );
    canonical->setValue(23);

    varSet->addPropertyAlias("CanonicalName", "LegacyName");

    EXPECT_EQ(canonical->getValue(), 23);
    EXPECT_STREQ(legacy->getName(), "LegacyName");
}

// Tests that a locked property migrates. renameProperty refuses these today, which leaves an
// add-on that used locked=True permanently stuck with the old name.
TEST_F(PropertyAlias, registrationRenamesLockedDynamicProperty)
{
    auto* legacy = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "LegacyName", "Variables")
    );
    legacy->setStatus(App::Property::LockDynamic, true);
    legacy->setValue(17);

    varSet->addPropertyAlias("CanonicalName", "LegacyName");

    EXPECT_STREQ(legacy->getName(), "CanonicalName");
    EXPECT_EQ(legacy->getValue(), 17);
}

// Pins the D6 semantics: declaring an alias claims that name, so a colliding user-added
// property is renamed. This is exactly what obj.renameProperty() already does for unlocked
// properties, so the design changes nothing here.
TEST_F(PropertyAlias, registrationRenamesUserAddedPropertyOnCollision)
{
    auto* userAdded = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "UserChosen", "Variables")
    );
    userAdded->setValue(5);

    varSet->addPropertyAlias("ClaimedName", "UserChosen");

    EXPECT_STREQ(userAdded->getName(), "ClaimedName");
    EXPECT_EQ(userAdded->getValue(), 5);
}

// Tests that an expression written against an alias is rewritten to the canonical name when
// the document is restored, so files stop accumulating alias references.
TEST_F(PropertyAliasStatic, restoredExpressionIsCanonicalized)
{
    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.AliasDeprecated")
    );

    doc->recompute();
    doc->afterRestore();

    auto expressions = second->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1U);
    std::string text = expressions.begin()->second->toString();
    EXPECT_NE(text.find("AliasTarget"), std::string::npos);
    EXPECT_EQ(text.find("AliasDeprecated"), std::string::npos);
}

// Tests that healing an expression does not make the document dirty. Opening a file should
// never mark it as modified.
TEST_F(PropertyAliasStatic, canonicalizationLeavesDocumentUntouched)
{
    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.AliasDeprecated")
    );

    doc->recompute();
    doc->purgeTouched();
    doc->afterRestore();

    EXPECT_FALSE(doc->isTouched());
}

// Tests that the logged rewrite count is measured rather than assumed.
TEST_F(PropertyAliasStatic, canonicalizationLogsRewriteCount)
{
    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.AliasDeprecated")
    );
    doc->recompute();

    LogCapture capture;
    doc->afterRestore();

    auto matches = [](const std::string& line) {
        return line.find("canonicalized 1 expression reference(s)") != std::string::npos;
    };
    EXPECT_TRUE(std::ranges::any_of(capture.messages, matches));
}

// Tests that a document with no aliased references is left completely alone.
TEST_F(PropertyAliasStatic, canonicalizationSkipsDocumentsWithoutAliasedReferences)
{
    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.AliasTarget")
    );

    doc->recompute();
    doc->afterRestore();

    auto expressions = second->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1U);
    EXPECT_NE(expressions.begin()->second->toString().find("AliasTarget"), std::string::npos);
}

// Tests that an alias shadowed by a real property of the same name is left alone: it is a
// genuine property reference, not a stale alias, so canonicalization must not rewrite it.
TEST_F(PropertyAliasStatic, canonicalizationSkipsShadowedAlias)
{
    // "Shadowed" is first added as a real dynamic property, then separately registered as an
    // alias for AliasTarget. Because AliasTarget already exists, addPropertyAlias() declines to
    // migrate/rename the dynamic property, leaving "Shadowed" as a genuine property that
    // happens to share its name with a registered alias.
    first->addDynamicProperty("App::PropertyInteger", "Shadowed", "Variables");
    first->addPropertyAlias("AliasTarget", "Shadowed", App::PropertyAliasType::Normal, "1.3");
    ASSERT_TRUE(first->getPropertyByName("Shadowed", App::PropertyLookupMode::WithoutAliases));
    ASSERT_TRUE(first->getPropertyAliases().contains("Shadowed"));

    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.Shadowed")
    );

    doc->recompute();
    doc->afterRestore();

    auto expressions = second->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1U);
    std::string text = expressions.begin()->second->toString();
    EXPECT_NE(text.find("Shadowed"), std::string::npos);
    EXPECT_EQ(text.find("AliasTarget"), std::string::npos);
}

// Tests that a dangling alias -- one whose canonical name does not resolve to any real
// property, e.g. a typo or a bad add-on registration -- is left alone by canonicalization.
// Rewriting an expression to a canonical name that does not exist would permanently corrupt
// it, since the rewrite gets written back to the file on next save.
TEST_F(PropertyAliasStatic, canonicalizationLeavesDanglingAliasAlone)
{
    first->addPropertyAlias("DoesNotExist", "DanglingAlias", App::PropertyAliasType::Normal, "1.3");
    ASSERT_FALSE(first->getPropertyByName("DoesNotExist", App::PropertyLookupMode::WithoutAliases));
    ASSERT_TRUE(first->getPropertyAliases().contains("DanglingAlias"));

    second->setExpression(
        App::ObjectIdentifier(second, std::string("Integer")),
        App::Expression::parse(second, "First.DanglingAlias")
    );

    doc->recompute();
    doc->afterRestore();

    auto expressions = second->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1U);
    std::string text = expressions.begin()->second->toString();
    EXPECT_NE(text.find("DanglingAlias"), std::string::npos);
}
