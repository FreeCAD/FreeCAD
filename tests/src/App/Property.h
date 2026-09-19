// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
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
#pragma once

#include <gtest/gtest.h>

#include <algorithm>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/Expression.h>
#include <App/FeatureTest.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyContainer.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>

#include <Base/Console.h>
#include <Base/Reader.h>

#include <src/App/InitApplication.h>

#include <xercesc/util/PlatformUtils.hpp>

/// RAII helper that captures developer warnings emitted via Base::Console().
class WarningCapture: public Base::ILogger
{
public:
    WarningCapture()
    {
        Base::Console().attachObserver(this);
    }

    ~WarningCapture() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "WarningCapture";
    }

    void sendLog(
        const std::string& /*notifierName*/,
        const std::string& message,
        Base::LogStyle level,
        Base::IntendedRecipient /*recipient*/,
        Base::ContentType /*content*/
    ) override
    {
        if (level == Base::LogStyle::Warning) {
            warnings.push_back(message);
        }
    }

    std::vector<std::string> warnings;
};

/// RAII helper that captures messages emitted via Base::Console().
class LogCapture: public Base::ILogger
{
public:
    LogCapture()
    {
        Base::Console().attachObserver(this);
    }

    ~LogCapture() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "LogCapture";
    }

    void sendLog(
        const std::string& /*notifierName*/,
        const std::string& message,
        Base::LogStyle level,
        Base::IntendedRecipient /*recipient*/,
        Base::ContentType /*content*/
    ) override
    {
        if (level == Base::LogStyle::Message) {
            messages.push_back(message);
        }
    }

    std::vector<std::string> messages;
};

class PropertyAliasStatic: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        std::string docName = App::GetApplication().getUniqueDocumentName("testAliasStatic");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void SetUp() override
    {
        first = freecad_cast<App::FeatureTest*>(doc->addObject("App::FeatureTest", "First"));
        second = freecad_cast<App::FeatureTest*>(doc->addObject("App::FeatureTest", "Second"));
    }

    void TearDown() override
    {
        doc->removeObject(first->getNameInDocument());
        doc->removeObject(second->getNameInDocument());
    }

    static void TearDownTestSuite()
    {
        App::GetApplication().closeDocument(doc->getName());
    }

    App::FeatureTest* first {};
    App::FeatureTest* second {};

    static App::Document* doc;
};

class PropertyAlias: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        std::string docName = App::GetApplication().getUniqueDocumentName("testAlias");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void SetUp() override
    {
        varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSetAlias"));
        dynProp = freecad_cast<App::PropertyInteger*>(
            varSet->addDynamicProperty("App::PropertyInteger", "NewName", "Variables")
        );
        dynProp->setValue(42);
    }

    void TearDown() override
    {
        doc->removeObject(varSet->getNameInDocument());
    }

    static void TearDownTestSuite()
    {
        App::GetApplication().closeDocument(doc->getName());
    }

    App::VarSet* varSet {};
    App::PropertyInteger* dynProp {};

    static App::Document* doc;
};

class PropertyAliasExtension: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        std::string docName = App::GetApplication().getUniqueDocumentName("testAliasExt");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        group = freecad_cast<App::DocumentObjectGroup*>(
            doc->addObject("App::DocumentObjectGroup", "Group")
        );
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(doc->getName());
    }

    App::Document* doc {};
    App::DocumentObjectGroup* group {};
};

class PropertyAliasDocument: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        std::string docName = App::GetApplication().getUniqueDocumentName("testAliasDoc");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(doc->getName());
    }

    App::Document* doc {};
};

namespace tests
{

/// PropertyContainer that records whether the legacy migration hooks were invoked.
class HookRecordingContainer: public App::PropertyContainer
{
    // PROPERTY_HEADER_WITH_OVERRIDE's getClassName() unconditionally requires a fully
    // qualified name (see PropertyContainer.h), so the class name must include the
    // enclosing "tests::" namespace even though this container is not a DocumentObject.
    PROPERTY_HEADER_WITH_OVERRIDE(tests::HookRecordingContainer);

public:
    HookRecordingContainer()
    {
        ADD_PROPERTY_TYPE(Renamed, (0), "Test", App::Prop_None, "Canonical property");
        ADD_PROPERTY_DEPRECATED_ALIAS(Renamed, "OldName", "1.1");
    }

    void handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName, const char* PropName) override
    {
        (void)reader;
        (void)TypeName;
        nameHookCalls.emplace_back(PropName);
    }

    void handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop) override
    {
        (void)reader;
        (void)prop;
        typeHookCalls.emplace_back(TypeName);
    }

    App::PropertyInteger Renamed;
    std::vector<std::string> nameHookCalls;
    std::vector<std::string> typeHookCalls;
};

}  // namespace tests

class PropertyAliasRestore: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
        tests::HookRecordingContainer::init();
    }

    /// Build a Properties document containing a single static-style Property element.
    static std::string makeDocument(const char* propName, const char* typeName, const char* body)
    {
        std::string str = "<?xml version='1.0' encoding='utf-8'?>\n";
        str.append("<Properties Count='1'>\n");
        str.append("<Property name='");
        str.append(propName);
        str.append("' type='");
        str.append(typeName);
        str.append("'>\n");
        str.append(body);
        str.append("</Property>\n");
        str.append("</Properties>\n");
        return str;
    }

    void restoreInto(tests::HookRecordingContainer& container, const std::string& xml)
    {
        std::stringstream data(xml);
        Base::XMLReader reader("Document.xml", data);
        container.Restore(reader);
    }
};

class RenameProperty: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        std::string docName = App::GetApplication().getUniqueDocumentName("test");
        doc = App::GetApplication().newDocument(docName.c_str(), "testUser");
    }

    void SetUp() override
    {
        varSet = freecad_cast<App::VarSet*>(doc->addObject("App::VarSet", "VarSet"));
        prop = freecad_cast<App::PropertyInteger*>(
            varSet->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
        );
        prop->setValue(value);
    }

    void TearDown() override
    {
        doc->removeObject(varSet->getNameInDocument());
    }

    static void TearDownTestSuite()
    {
        App::GetApplication().closeDocument(doc->getName());
    }

    const long value = 123;
    App::VarSet* varSet;
    App::PropertyInteger* prop;

    static App::Document* doc;
};

class MoveProperty: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        std::string docName1 = App::GetApplication().getUniqueDocumentName("test");
        std::string docName2 = App::GetApplication().getUniqueDocumentName("test");
        doc1 = App::GetApplication().newDocument(docName1.c_str());
        doc2 = App::GetApplication().newDocument(docName2.c_str());

        varSet1Doc1 = freecad_cast<App::VarSet*>(doc1->addObject("App::VarSet", "VarSet"));
        varSet2Doc1 = freecad_cast<App::VarSet*>(doc1->addObject("App::VarSet", "VarSet"));
        varSetDoc2 = freecad_cast<App::VarSet*>(doc2->addObject("App::VarSet", "VarSet"));

        prop = freecad_cast<App::PropertyInteger*>(
            varSet1Doc1->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
        );
        prop->setValue(value);
    }

    void TearDown() override
    {
        for (auto* obj : doc1->topologicalSort()) {
            doc1->removeObject(obj->getNameInDocument());
        }
        doc2->removeObject(varSetDoc2->getNameInDocument());
        App::GetApplication().closeDocument(doc1->getName());
        App::GetApplication().closeDocument(doc2->getName());
    }

    void assertMovedProperty(App::Property* property, App::DocumentObject* target);

    void testMoveProperty(App::DocumentObject* target);

    void testMovePropertyExpressionWithAct(
        App::DocumentObject* sourceProp2,
        App::DocumentObject* target,
        const char* exprString,
        const std::function<App::Property*()>& act
    );

    void testMovePropertyExpression(
        App::DocumentObject* source,
        App::DocumentObject* target,
        const char* exprString
    );

    void testUndoProperty(App::DocumentObject* target);

    void testUndoMovePropertyExpression(
        App::DocumentObject* sourceProp2,
        App::DocumentObject* target,
        const char* exprString,
        const char* exprStringAfterUndo
    );

    void testRedoMovePropertyExpression(
        App::DocumentObject* sourceProp2,
        App::DocumentObject* target,
        const char* exprString,
        const char* exprStringAfterUndo
    );

    static void TearDownTestSuite()
    {}

    const long value = 123;

    App::Document* doc1;
    App::Document* doc2;

    App::VarSet* varSet1Doc1;
    App::VarSet* varSet2Doc1;
    App::VarSet* varSetDoc2;
    App::PropertyInteger* prop;
};
