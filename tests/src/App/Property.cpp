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

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Interpreter.h>

#include <App/Application.h>
#include <App/AutoTransaction.h>
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

App::Document* RenameProperty::doc {nullptr};

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
        "App.ActiveDocument.getObject('VarSet').renameProperty('Variable', 'NewName')");

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
            "App.ActiveDocument.getObject('VarSet006').renameProperty('Label', 'NewName')"),
        Base::Exception);

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

// Tests whether we can rename to a property that already exists
TEST_F(RenameProperty, toExistingProperty)
{
    // Arrange
    App::Property* prop2 =
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables");

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
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));

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
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));

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
        varSet2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));

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
        varSet->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));
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
    // Arrange
    doc->setUndoMode(1);

    // Act
    bool isRenamed = false;
    {
        App::AutoTransaction transaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
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
    // Arrange
    doc->setUndoMode(1);

    // Act
    bool isRenamed = false;
    {
        App::AutoTransaction transaction("Rename Property");
        isRenamed = varSet->renameDynamicProperty(prop, "NewName");
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

    auto* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(target->getDynamicPropertyByName("Variable"));
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
    // TODO: temporary
    // doc1->setUndoMode(1);
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
    App::Property* prop2 =
        varSet2Doc1->addDynamicProperty("App::PropertyInteger", "Variable", "Variables");

    // Act / Assert
    EXPECT_THROW(varSet1Doc1->moveDynamicProperty(prop, varSet2Doc1), Base::NameError);

    EXPECT_EQ(varSet1Doc1->getPropertyByName("Variable"), prop);
    EXPECT_EQ(varSet2Doc1->getPropertyByName("Variable"), prop2);
}

void MoveProperty::testMovePropertyExpressionWithAct(App::DocumentObject* sourceProp2,
                                                     App::DocumentObject* target,
                                                     const char* exprString,
                                                     const std::function<App::Property*()>& act)
{
    // Arrange
    const auto* prop2 = freecad_cast<App::PropertyInteger*>(
        sourceProp2->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));

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

    auto* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(target->getDynamicPropertyByName("Variable"));
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), value);
    EXPECT_STREQ(sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
                 exprString);
}

void MoveProperty::testMovePropertyExpression(App::DocumentObject* sourceProp2,
                                              App::DocumentObject* target,
                                              const char* exprString)
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
        varSet1Doc1->addDynamicProperty("App::PropertyInteger", "Variable2", "Variables"));
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

    auto* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(varSet2Doc1->getDynamicPropertyByName("Variable"));
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), valueVar2);

    std::map<App::ObjectIdentifier, const App::Expression*> expressions =
        varSet2Doc1->ExpressionEngine.getExpressions();
    ASSERT_EQ(expressions.size(), 1);
    EXPECT_STREQ(expressions.begin()->first.getPropertyName().c_str(), "Variable");
    EXPECT_STREQ(expressions.begin()->second->toString().c_str(), "VarSet.Variable2");
}

void MoveProperty::testUndoProperty(App::DocumentObject* target)
{
    // Arrange
    doc1->setUndoMode(1);
    doc2->setUndoMode(1);

    // Act
    App::Property* movedProp = nullptr;
    {
        App::AutoTransaction transaction("Move Property");
        movedProp = varSet1Doc1->moveDynamicProperty(prop, target);
    }

    // Assert
    assertMovedProperty(movedProp, target);

    // Act: Undo the move
    bool undone = doc1->undo();

    // Assert: The property should be back to its original container and value
    EXPECT_TRUE(undone);
    auto* originalProp =
        freecad_cast<App::PropertyInteger*>(varSet1Doc1->getDynamicPropertyByName("Variable"));
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

void MoveProperty::testUndoMovePropertyExpression(App::DocumentObject* sourceProp2,
                                                  App::DocumentObject* target,
                                                  const char* exprString,
                                                  const char* exprStringAfterUndo)
{
    // Arrange
    doc1->setUndoMode(1);

    auto act = [this, target] {
        App::Property* movedProp = nullptr;
        {
            App::AutoTransaction transaction("Move Property");
            movedProp = varSet1Doc1->moveDynamicProperty(prop, target);
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
    auto* originalProp =
        freecad_cast<App::PropertyInteger*>(varSet1Doc1->getDynamicPropertyByName("Variable"));
    ASSERT_TRUE(originalProp != nullptr);
    EXPECT_EQ(originalProp->getValue(), value);
    EXPECT_STREQ(sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
                 exprStringAfterUndo);

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
    App::Property* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(varSet2Doc1->getDynamicPropertyByName("Variable"));
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
    App::Property* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(varSetDoc2->getDynamicPropertyByName("Variable"));
    assertMovedProperty(movedPropWithType, varSetDoc2);
}

void MoveProperty::testRedoMovePropertyExpression(App::DocumentObject* sourceProp2,
                                                  App::DocumentObject* target,
                                                  const char* exprString,
                                                  const char* exprStringAfterUndo)
{
    testUndoMovePropertyExpression(sourceProp2, target, exprString, exprStringAfterUndo);

    bool redone = doc1->redo();
    doc1->recompute();
    doc2->recompute();
    sourceProp2->ExpressionEngine.execute();

    // Assert: The property should be moved to the target container again
    EXPECT_TRUE(redone);
    auto* movedPropWithType =
        freecad_cast<App::PropertyInteger*>(target->getDynamicPropertyByName("Variable"));
    ASSERT_TRUE(movedPropWithType != nullptr);
    EXPECT_EQ(movedPropWithType->getValue(), value);
    EXPECT_STREQ(sourceProp2->ExpressionEngine.getExpressions().begin()->second->toString().c_str(),
                 exprString);
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
