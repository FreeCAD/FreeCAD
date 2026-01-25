/***************************************************************************
 *   Copyright (c) 2024 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "Base/Unit.h"
#include "App/GroupExtension.h"
#include "FemPostFilter.h"

namespace Fem
{

enum PostGroupMode
{
    Serial,
    Parallel
};

// object grouping FEM filters and building the structure of the pipeline
class FemExport FemPostGroupExtension: public App::GroupExtension
{

    using inherited = App::GroupExtension;
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostGroupExtension);

public:
    /// Constructor
    FemPostGroupExtension();
    ~FemPostGroupExtension() override;

    void initExtension(App::ExtensionContainer* obj) override;

    App::PropertyEnumeration Mode;

    // Pipeline handling
    virtual void filterChanged(FemPostFilter*) {};          // settings change in filter
    virtual void filterPipelineChanged(FemPostFilter*) {};  // pipeline change in filter
    virtual void recomputeChildren();
    virtual FemPostObject* getLastPostObject();
    virtual bool holdsPostObject(FemPostObject* obj);

    // general
    std::vector<Fem::FemPostFilter*> getFilter();
    static App::DocumentObject* getGroupOfObject(const App::DocumentObject* obj);

protected:
    void extensionOnChanged(const App::Property* p) override;
    void onExtendedUnsetupObject() override;
    bool allowObject(App::DocumentObject* obj) override;

private:
    bool m_blockChange = false;
    static const char* ModeEnums[];
};

}  // namespace Fem
