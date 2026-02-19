// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
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

#include "Base/Interpreter.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>

#include <src/App/InitApplication.h>

class PropertyIntercept: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        docName = App::GetApplication().getUniqueDocumentName("test");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet"));
    }

    void TearDown() override
    {
        doc->removeObject(varSet->getNameInDocument());
        App::GetApplication().closeDocument(docName.c_str());
    }

    App::VarSet* getVarSet()
    {
        return varSet;
    }

    App::Document* getDoc()
    {
        return doc;
    }

private:
    App::VarSet* varSet {};
    std::string docName;
    App::Document* doc;
};

long evalPythonInt(const std::string& command)
{
    Py_Initialize();
    Base::PyGILStateLocker lock;

    long result = 0;
    Py::Object pyResult = Base::Interpreter().runStringObject(command.c_str());

    try {
        Py::Int f(pyResult);
        return f.as_long();
    }
    catch (Py::Exception& e) {
        Py::Object obj = e.errorValue();
        std::cerr << obj.str() << '\n';
    }

    return result;
}

Base::Vector3d evalPythonVector(const std::string& command)
{
    Py_Initialize();
    Base::PyGILStateLocker lock;

    Base::Vector3d result;
    Py::Object pyResult = Base::Interpreter().runStringObject(command.c_str());
    PyObject* p = pyResult.ptr();

    if (PyObject_HasAttrString(p, "x") && PyObject_HasAttrString(p, "y")
        && PyObject_HasAttrString(p, "z")) {
        Py::Float xObj = Py::Object(PyObject_GetAttrString(p, "x"));
        Py::Float yObj = Py::Object(PyObject_GetAttrString(p, "y"));
        Py::Float zObj = Py::Object(PyObject_GetAttrString(p, "z"));

        result.x = static_cast<double>(xObj);
        result.y = static_cast<double>(yObj);
        result.z = static_cast<double>(zObj);
    }

    return result;
}

std::vector<long> evalPythonIntList(const std::string& command)
{
    Py_Initialize();
    Base::PyGILStateLocker lock;

    std::vector<long> result;
    Py::Object pyResult = Base::Interpreter().runStringObject(command.c_str());
    PyObject* p = pyResult.ptr();

    if (PyList_Check(p)) {
        for (Py_ssize_t i = 0; i < PyList_Size(p); ++i) {
            Py::Object item = Py::Object(PyList_GetItem(p, i));
            try {
                Py::Int f(item);
                result.push_back(f.as_long());
            }
            catch (Py::Exception& e) {
                Py::Object obj = e.errorValue();
                std::cerr << obj.str() << '\n';
            }
        }
    }

    return result;
}

// Whether getting and setting an integer property works correctly with contexts
TEST_F(PropertyIntercept, testIntegerGetSet)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyInteger*>(
        getVarSet()->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
    );

    // Act - Initialize
    prop->setValue(1);
    long value = prop->getValue();

    // Assert
    ASSERT_EQ(value, 1);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);
    value = prop->getValue();

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(value, 1);

    // Act - Set value given a context
    prop->setValue(2);
    value = prop->getValue();

    // Assert - Value should be the context value
    ASSERT_EQ(value, 2);

    // Act - Remove context
    getVarSet()->popContext();
    value = prop->getValue();

    // Assert - Value should be the original value
    ASSERT_EQ(value, 1);
}

// Whether getting and setting an integer property works correctly with contexts in Python
TEST_F(PropertyIntercept, testIntegerPythonGetSet)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyInteger", "Variable", "Variables");

    // Act - Initialize
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = 1");
    long value = evalPythonInt("App.ActiveDocument.getObject('VarSet').Variable");

    // Assert
    ASSERT_EQ(value, 1);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);
    value = evalPythonInt("App.ActiveDocument.getObject('VarSet').Variable");

    // Assert - Value should be the same as no context is set
    ASSERT_EQ(value, 1);

    // Act - Set value given a context
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = 2");
    value = evalPythonInt("App.ActiveDocument.getObject('VarSet').Variable");

    // Assert - Value should be the context value
    ASSERT_EQ(value, 2);

    // Act - Remove context
    getVarSet()->popContext();
    value = evalPythonInt("App.ActiveDocument.getObject('VarSet').Variable");

    // Assert - Value should be the original value
    ASSERT_EQ(value, 1);
}

// Whether copying an integer property works correctly with contexts
TEST_F(PropertyIntercept, testIntegerCopy)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyInteger*>(
        getVarSet()->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
    );

    // Act - Initialize
    prop->setValue(1);
    long value = prop->getValue();

    // Assert
    ASSERT_EQ(value, 1);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as no context is set
    value = prop->getValue();
    ASSERT_EQ(value, 1);

    // Act - Set value given a context
    prop->setValue(2);
    value = prop->getValue();

    // Assert - Value should be the context value
    ASSERT_EQ(value, 2);

    auto* copiedProp = freecad_cast<App::PropertyInteger*>(prop->Copy());
    value = copiedProp->getValue();

    // Assert - Value should be the context value
    ASSERT_EQ(value, 2);

    // Act - Remove context
    getVarSet()->popContext();
    value = prop->getValue();

    // Assert - Value should be the original value
    ASSERT_EQ(value, 1);

    // Assert - Copied property value should still be the context value
    value = copiedProp->getValue();
    ASSERT_EQ(value, 2);
}

// Whether pasting an integer property works correctly with contexts
TEST_F(PropertyIntercept, testIntegerPaste)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyInteger*>(
        getVarSet()->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
    );

    auto* otherVarSet = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "OtherVarSet"));
    auto* toBePasted = freecad_cast<App::PropertyInteger*>(
        otherVarSet->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
    );
    toBePasted->setValue(2);

    // Act
    prop->setValue(1);
    long value = prop->getValue();

    // Assert
    ASSERT_EQ(value, 1);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as no context is set
    value = prop->getValue();
    ASSERT_EQ(value, 1);

    // Act - Paste the property
    prop->Paste(*toBePasted);
    value = prop->getValue();

    // Assert - Value should be the context value
    ASSERT_EQ(value, 2);

    // Act - Remove context
    getVarSet()->popContext();
    value = prop->getValue();

    // Assert - Value should be the original value
    ASSERT_EQ(value, 1);

    // Assert - The to-be-pasted property value should still be the context value
    value = toBePasted->getValue();
    ASSERT_EQ(value, 2);
}

void assertVector(const Base::Vector3d& vec, double x, double y, double z)
{
    ASSERT_DOUBLE_EQ(vec.x, x);
    ASSERT_DOUBLE_EQ(vec.y, y);
    ASSERT_DOUBLE_EQ(vec.z, z);
}

// Whether getting and setting a vector property with separate doubles works
// correctly with contexts.
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
TEST_F(PropertyIntercept, testVectorGetSetDouble)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyVector*>(
        getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables")
    );

    // Act
    prop->setValue(1.0, 2.0, 3.0);
    Base::Vector3d value = prop->getValue();

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as before
    value = prop->getValue();
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    prop->setValue(2.0, 3.0, 4.0);
    value = prop->getValue();

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = prop->getValue();

    // Assert - Value should be the original value
    assertVector(value, 1.0, 2.0, 3.0);
}

// Whether getting and setting a vector property with vector values works
// correctly with contexts.
TEST_F(PropertyIntercept, testVectorGetSetVector)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyVector*>(
        getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables")
    );

    // Act
    Base::Vector3d vec(1.0, 2.0, 3.0);
    prop->setValue(vec);
    Base::Vector3d value = prop->getValue();

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as before the context was set
    value = prop->getValue();
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    Base::Vector3d vec2(2.0, 3.0, 4.0);
    prop->setValue(vec2);
    value = prop->getValue();

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = prop->getValue();

    // Assert - Value should be the original value
    assertVector(value, 1.0, 2.0, 3.0);
}

// Whether getting and setting a vector property with tuple values works
// correctly with contexts in Python.
TEST_F(PropertyIntercept, testVectorPythonGetSetTuple)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables");

    const char* getVarCmd = "App.ActiveDocument.getObject('VarSet').Variable";

    // Act
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = (1.0, 2.0, 3.0)");
    Base::Vector3d value = evalPythonVector(getVarCmd);

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as before the context was set
    value = evalPythonVector(getVarCmd);
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = (2.0, 3.0, 4.0)");
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the original value
    assertVector(value, 1.0, 2.0, 3.0);
}

// Whether getting and setting a vector property with tuple values works
// correctly with double contexts in Python.
TEST_F(PropertyIntercept, testVectorPythonGetSetTupleDoubleContext)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables");
    const char* getVarCmd = "App.ActiveDocument.getObject('VarSet').Variable";
    const char* getVarContextCmd = "App.ActiveDocument.getObject('Context').Variable";

    // Act
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = (1.0, 2.0, 3.0)");
    Base::Vector3d value = evalPythonVector(getVarCmd);

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the same as no context is set
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = (2.0, 3.0, 4.0)");
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    auto* context2 = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context2"));
    context->pushContext(context2);
    value = evalPythonVector(getVarContextCmd);

    // Assert - Value should be the same as without context2
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Set value given a context
    Base::Interpreter().runString("App.ActiveDocument.getObject('Context').Variable = (3.0, 4.0, 5.0)");
    value = evalPythonVector(getVarContextCmd);

    // Assert - Value should be the context value
    assertVector(value, 3.0, 4.0, 5.0);

    // Act - Remove context 2
    context->popContext();
    value = evalPythonVector(getVarContextCmd);

    // Assert - Value should be the value of the first context
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the value of the first context
    assertVector(value, 1.0, 2.0, 3.0);
}

// Whether getting and setting a vector property with vector values works
// correctly with contexts in Python.
TEST_F(PropertyIntercept, testVectorPythonGetSetVector)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables");
    auto* varSet2 = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "VarSet2"));
    auto* prop = freecad_cast<App::PropertyVector*>(
        varSet2->addDynamicProperty("App::PropertyVector", "Source", "Variables")
    );
    prop->setValue(1.0, 2.0, 3.0);
    const char* setVarCmd = "App.ActiveDocument.getObject('VarSet').Variable = "
                            "App.ActiveDocument.getObject('VarSet2').Source";
    const char* getVarCmd = "App.ActiveDocument.getObject('VarSet').Variable";

    // Act
    Base::Interpreter().runString(setVarCmd);
    Base::Vector3d value = evalPythonVector(getVarCmd);

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as no context is set
    value = evalPythonVector(getVarCmd);
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    prop->setValue(2.0, 3.0, 4.0);
    Base::Interpreter().runString(setVarCmd);
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the original value
    assertVector(value, 1.0, 2.0, 3.0);
}


// Whether getting and setting a vector property with vector values works
// correctly with double contexts in Python.
TEST_F(PropertyIntercept, testVectorPythonGetSetVectorDoubleContext)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyVector", "Variable", "Variables");
    auto* varSet2 = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "VarSet2"));
    auto* prop = freecad_cast<App::PropertyVector*>(
        varSet2->addDynamicProperty("App::PropertyVector", "Source", "Variables")
    );
    prop->setValue(1.0, 2.0, 3.0);
    const char* setVarCmd = "App.ActiveDocument.getObject('VarSet').Variable = "
                            "App.ActiveDocument.getObject('VarSet2').Source";
    const char* getVarCmd = "App.ActiveDocument.getObject('VarSet').Variable";

    // Act
    Base::Interpreter().runString(setVarCmd);
    Base::Vector3d value = evalPythonVector(getVarCmd);

    // Assert
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as no context is set
    value = evalPythonVector(getVarCmd);
    assertVector(value, 1.0, 2.0, 3.0);

    // Act - Set value given a context
    prop->setValue(2.0, 3.0, 4.0);
    Base::Interpreter().runString(setVarCmd);
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the context value
    assertVector(value, 2.0, 3.0, 4.0);

    auto* context2 = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context2"));
    context->pushContext(context2);

    const char* setVarContext = "App.ActiveDocument.getObject('Context').Variable = "
                                "App.ActiveDocument.getObject('VarSet2').Source";
    const char* getVarContext = "App.ActiveDocument.getObject('Context').Variable";

    // Assert - Value  should be the same as without context 2
    Base::Interpreter().runString(getVarContext);
    value = evalPythonVector(getVarContext);
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Set value in context 2
    prop->setValue(3.0, 4.0, 5.0);
    Base::Interpreter().runString(setVarContext);
    value = evalPythonVector(getVarContext);
    assertVector(value, 3.0, 4.0, 5.0);

    // Act - Remove context 2
    context->popContext();
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the value of the first context
    assertVector(value, 2.0, 3.0, 4.0);

    // Act - Remove context
    getVarSet()->popContext();
    value = evalPythonVector(getVarCmd);

    // Assert - Value should be the original value
    assertVector(value, 1.0, 2.0, 3.0);
}

// Whether using the operator and setting one value works.
TEST_F(PropertyIntercept, testIntegerListGetOperatorSetOneValue)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyIntegerList*>(
        getVarSet()->addDynamicProperty("App::PropertyIntegerList", "Variable", "Variables")
    );

    // Act - Initialize
    prop->setValue(1);

    // Assert
    ASSERT_EQ(prop->getSize(), 1);
    ASSERT_EQ((*prop)[0], 1);

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(prop->getSize(), 1);
    ASSERT_EQ((*prop)[0], 1);

    // Act - Set value given a context
    prop->setValue(2);

    // Assert - Value should be the context value
    ASSERT_EQ(prop->getSize(), 1);
    ASSERT_EQ((*prop)[0], 2);

    // Act - Remove context
    getVarSet()->popContext();

    // Assert - Value should be the original value
    ASSERT_EQ(prop->getSize(), 1);
    ASSERT_EQ((*prop)[0], 1);
}

void assertListEqual(const App::PropertyIntegerList* prop, const std::vector<long>& expected)
{
    ASSERT_EQ(prop->getSize(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ((*prop)[i], expected[i]);
    }
}


// Whether setting the size of an integer list property works
TEST_F(PropertyIntercept, testIntegerListSetSize)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyIntegerList*>(
        getVarSet()->addDynamicProperty("App::PropertyIntegerList", "Variable", "Variables")
    );

    // Act - Initialize
    prop->setSize(3, 1);

    // Assert
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 1, 1});

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 1, 1});

    // Act - Set value given a context
    prop->setSize(4, 2);

    // Assert - Value should be the context value
    ASSERT_EQ(prop->getSize(), 4);
    assertListEqual(prop, {1, 1, 1, 2});

    // Act - Remove context
    getVarSet()->popContext();

    // Assert - Value should be the original value
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 1, 1});
}

// Whether setting the size of an integer list property works
TEST_F(PropertyIntercept, testIntegerListSetValues)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyIntegerList*>(
        getVarSet()->addDynamicProperty("App::PropertyIntegerList", "Variable", "Variables")
    );

    // Act - Initialize
    prop->setValues({1, 2, 3});

    // Assert
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 2, 3});

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 2, 3});

    // Act - Set value given a context
    prop->setValues({4, 5, 6, 7});

    // Assert - Value should be the context value
    ASSERT_EQ(prop->getSize(), 4);
    assertListEqual(prop, {4, 5, 6, 7});

    // Act - Remove context
    getVarSet()->popContext();

    // Assert - Value should be the original value
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 2, 3});
}

// Whether setting an integer list property works in Python
TEST_F(PropertyIntercept, testIntegerListPython)
{
    // Arrange
    getVarSet()->addDynamicProperty("App::PropertyIntegerList", "Variable", "Variables");

    const std::string getVarCmd = "App.ActiveDocument.getObject('VarSet').Variable";
    const std::string getLenCmd = "len(" + getVarCmd + ")";


    // Act - Initialize
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = [1, 2, 3]");
    long size = evalPythonInt(getLenCmd);
    auto values = evalPythonIntList(getVarCmd);

    // Assert
    ASSERT_EQ(size, 3);
    ASSERT_THAT(values, ::testing::ElementsAre(1, 2, 3));

    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);
    size = evalPythonInt(getLenCmd);
    values = evalPythonIntList(getVarCmd);

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(size, 3);
    ASSERT_THAT(values, ::testing::ElementsAre(1, 2, 3));

    // Act - Set value given a context
    Base::Interpreter().runString("App.ActiveDocument.getObject('VarSet').Variable = [4, 5, 6, 7]");
    size = evalPythonInt(getLenCmd);
    values = evalPythonIntList(getVarCmd);

    // Assert - Value should be the context value
    ASSERT_EQ(size, 4);
    ASSERT_THAT(values, ::testing::ElementsAre(4, 5, 6, 7));

    // Act - Remove context
    getVarSet()->popContext();
    size = evalPythonInt(getLenCmd);
    values = evalPythonIntList(getVarCmd);

    // Assert - Value should be the original value
    ASSERT_EQ(size, 3);
    ASSERT_THAT(values, ::testing::ElementsAre(1, 2, 3));
}

// Whether setting the size of an integer list property works
TEST_F(PropertyIntercept, testIntegerListTouchList)
{
    // Arrange
    auto* prop = freecad_cast<App::PropertyIntegerList*>(
        getVarSet()->addDynamicProperty("App::PropertyIntegerList", "Variable", "Variables")
    );
    prop->setValues({1, 2, 3});

    // Act - Initialize
    prop->set1Value(1, 10);
    std::set<int> touchList = prop->getTouchList();

    // Assert
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 10, 3});
    ASSERT_THAT(touchList, testing::UnorderedElementsAre(1));

    // Act - Clear touch list
    prop->clearTouchList();
    touchList = prop->getTouchList();

    // Assert
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 10, 3});
    ASSERT_THAT(touchList, testing::IsEmpty());


    // Act - Add a context
    auto* context = freecad_cast<App::VarSet*>(getDoc()->addObject("App::VarSet", "Context"));
    getVarSet()->pushContext(context);

    // Assert - Value should be the same as there are no writes given the context
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 10, 3});
    ASSERT_THAT(touchList, testing::IsEmpty());

    // Act - Set value given a context
    prop->set1Value(2, 20);
    touchList = prop->getTouchList();

    // Assert - Value should be the context value
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 10, 20});
    ASSERT_THAT(touchList, testing::UnorderedElementsAre(2));

    // Act - Remove context
    getVarSet()->popContext();
    touchList = prop->getTouchList();

    // Assert - Value should be the original value
    ASSERT_EQ(prop->getSize(), 3);
    assertListEqual(prop, {1, 10, 3});
    ASSERT_THAT(touchList, testing::IsEmpty());
}


// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
