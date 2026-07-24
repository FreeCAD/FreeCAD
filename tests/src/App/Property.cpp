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

std::string PropertyAlias::_docName;
App::Document* PropertyAlias::_doc {nullptr};

std::string RenameProperty::_docName;
App::Document* RenameProperty::_doc {nullptr};

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

    std::string cmd = "vs = App.getDocument('" + _docName + "').getObject('"
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

    std::string cmd = "vs = App.getDocument('" + _docName + "').getObject('"
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
    std::string cmd = "vs = App.getDocument('" + _docName + "').getObject('"
        + varSet->getNameInDocument()
        + "')\n"
          "vs.addPropertyAlias('NewName', 'PyAlias')\n"
          "p = vs.getPropertyByName('PyAlias')\n"
          "assert p is not None, 'Alias registered from Python must resolve'";
    Base::Interpreter().runString(cmd.c_str());

    EXPECT_EQ(varSet->getPropertyByName("PyAlias"), varSet->getPropertyByName("NewName"));
}

// Tests whether we can rename a property
TEST_F(RenameProperty, renameProperty)
{
    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}

// Tests whether we can rename a property from Python
TEST_F(RenameProperty, renamePropertyPython)
{
    // Act
    Base::Interpreter().runString(
        "App.ActiveDocument.getObject('VarSet').renameProperty('Variable', 'NewName')"
    );

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}

// Tests whether we can rename a property in a chain
TEST_F(RenameProperty, renamePropertyChain)
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
TEST_F(RenameProperty, renameStaticProperty)
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
TEST_F(RenameProperty, renameStaticPropertyPython)
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
TEST_F(RenameProperty, renameLockedProperty)
{
    // Arrange
    prop->setStatus(App::Property::LockDynamic, true);

    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop, "NewName"), Base::RuntimeError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}

// Tests whether we can rename to a property that already exists
TEST_F(RenameProperty, renameToExistingProperty)
{
    // Arrange
    App::Property* prop2 = varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables");

    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop2, "Variable"), Base::NameError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_STREQ(varSet->getPropertyName(prop2), "Variable2");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable2"), prop2);
}

// Tests whether we can rename to a property that is invalid
TEST_F(RenameProperty, renameToInvalidProperty)
{
    // Act / Assert
    EXPECT_THROW(varSet->renameDynamicProperty(prop, "0Variable"), Base::NameError);

    // Assert
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), Value);
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
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(prop2->getValue(), Value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), Value);
}

// Tests whether we can rename a property that is used in an expression in a different container
TEST_F(RenameProperty, updateExpressionDifferentContainer)
{
    // Arrange
    auto* varSet2 = freecad_cast<App::VarSet*>(_doc->addObject("App::VarSet", "VarSet2"));
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "VarSet.Variable"));
    varSet2->setExpression(path, expr);
    varSet2->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(prop2->getValue(), Value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet2->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), Value);

    // Tear down
    _doc->removeObject(varSet2->getNameInDocument());
}

// Tests whether we can rename a property that is used in an expression in a different document
TEST_F(RenameProperty, updateExpressionDifferentDocument)
{
    // Arrange
    std::string docName = App::GetApplication().getUniqueDocumentName("test2");
    App::Document* doc = App::GetApplication().newDocument(docName.c_str(), "testUser");

    auto* varSet2 = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet2"));
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );

    App::ObjectIdentifier path(*prop2);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "test#VarSet.Variable"));
    _doc->saveAs("test.FCStd");
    doc->saveAs("test2.FCStd");
    varSet2->setExpression(path, expr);
    varSet2->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(prop2->getValue(), Value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet2->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
    EXPECT_EQ(prop2->getValue(), Value);

    // Tear down
    doc->removeObject(varSet2->getNameInDocument());
}

// Test if we can rename a property which value is the result of an expression
TEST_F(RenameProperty, renamePropertyWithExpression)
{
    // Arrange
    auto* prop2 = freecad_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables")
    );
    prop2->setValue(Value);

    App::ObjectIdentifier path(*prop);
    std::shared_ptr<App::Expression> expr(App::Expression::parse(varSet, "Variable2"));
    varSet->setExpression(path, expr);
    varSet->ExpressionEngine.execute();

    // Assert before the rename
    EXPECT_EQ(prop2->getValue(), Value);
    EXPECT_EQ(prop->getValue(), Value);

    // Act
    bool isRenamed = varSet->renameDynamicProperty(prop, "NewName");
    varSet->ExpressionEngine.execute();

    // Assert after the rename
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act
    prop2->setValue(Value + 1);
    varSet->ExpressionEngine.execute();

    // Assert
    EXPECT_EQ(prop2->getValue(), Value + 1);
    EXPECT_EQ(prop->getValue(), Value + 1);
}

// Tests whether we can rename a property and undo it
TEST_F(RenameProperty, undoRenameProperty)
{
    // Act
    bool isRenamed = false;
    {
        _doc->openTransaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
        _doc->commitTransaction();
    }

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act: Undo the rename
    bool undone = _doc->undo();

    // Assert: The property should be back to its original name and value
    EXPECT_TRUE(undone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);
}


// Tests whether we can rename a property, undo, and redo it
TEST_F(RenameProperty, redoRenameProperty)
{
    // Act
    bool isRenamed = false;
    {
        _doc->openTransaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
        _doc->commitTransaction();
    }

    // Assert
    EXPECT_TRUE(isRenamed);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);

    // Act: Undo the rename
    bool undone = _doc->undo();

    // Assert: The property should be back to its original name and value
    EXPECT_TRUE(undone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "Variable");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), nullptr);

    // Act: Redo the rename
    bool redone = _doc->redo();
    EXPECT_TRUE(redone);
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
    EXPECT_EQ(prop->getValue(), Value);
    EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
    EXPECT_EQ(varSet->getDynamicPropertyByName("NewName"), prop);
}
