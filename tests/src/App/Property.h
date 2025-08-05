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
#ifndef TEST_PROPERTY_H
#define TEST_PROPERTY_H

#include <gtest/gtest.h>

#include <App/Document.h>
#include <App/PropertyStandard.h>
#include <App/VarSet.h>

#include <src/App/InitApplication.h>

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
            varSet->addDynamicProperty("App::PropertyInteger", "Variable", "Variables"));
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
        std::string docName1 = App::GetApplication().getUniqueDocumentName("test");
        std::string docName2 = App::GetApplication().getUniqueDocumentName("test");
        doc1 = App::GetApplication().newDocument(docName1.c_str());
        doc2 = App::GetApplication().newDocument(docName2.c_str());
    }

    void SetUp() override
    {
        varSet1Doc1 = freecad_cast<App::VarSet*>(doc1->addObject("App::VarSet", "VarSet"));
        varSet2Doc1 = freecad_cast<App::VarSet*>(doc1->addObject("App::VarSet", "VarSet"));
        varSetDoc2 = freecad_cast<App::VarSet*>(doc2->addObject("App::VarSet", "VarSet"));

        prop = freecad_cast<App::PropertyInteger*>(
            varSet1Doc1->addDynamicProperty("App::PropertyInteger", "Variable", "Variables"));
        prop->setValue(value);
    }

    void TearDown() override
    {
        // std::cout << "pending recompute varSet1Doc1? " <<
        // varSet1Doc1->testStatus(App::ObjectStatus::PendingRecompute)
        //           << std::endl;
        // std::cout << "pending recompute varSet1Doc1? " <<
        // varSet2Doc1->testStatus(App::ObjectStatus::PendingRecompute)
        //           << std::endl;
        // doc1->recompute();
        for (auto* obj : doc1->topologicalSort()) {
            std::cout << "removing object: " << obj->getNameInDocument() << std::endl;
            doc1->removeObject(obj->getNameInDocument());
        }
        // doc1->removeObject(varSet1Doc1->getNameInDocument());
        // doc1->removeObject(varSet2Doc1->getNameInDocument());
        // doc1->recompute();
        doc2->removeObject(varSetDoc2->getNameInDocument());
        // doc2->recompute();
    }

    void testMoveProperty(App::DocumentObject* target);

    void testMovePropertyExpression(App::DocumentObject* source, const char* exprString);

    static void TearDownTestSuite()
    {
        App::GetApplication().closeDocument(doc1->getName());
        App::GetApplication().closeDocument(doc2->getName());
    }

    const long value = 123;

    App::VarSet* varSet1Doc1;
    App::VarSet* varSet2Doc1;
    App::VarSet* varSetDoc2;
    App::PropertyInteger* prop;

    static App::Document* doc1;
    static App::Document* doc2;
};
#endif
