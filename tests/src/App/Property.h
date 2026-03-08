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
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void SetUp() override
    {
        varSet = freecad_cast<App::VarSet*>(_doc->addObject("App::VarSet", "VarSet"));
        prop = freecad_cast<App::PropertyInteger*>(
            varSet->addDynamicProperty("App::PropertyInteger", "Variable", "Variables")
        );
        prop->setValue(Value);
    }

    void TearDown() override
    {
        _doc->removeObject(varSet->getNameInDocument());
    }

    static void TearDownTestSuite()
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    const long Value = 123;
    App::VarSet* varSet {};
    App::PropertyInteger* prop {};

    static std::string _docName;
    static App::Document* _doc;
};
