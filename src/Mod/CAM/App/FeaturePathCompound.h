// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <App/PropertyLinks.h>

#include "FeaturePath.h"


namespace Path
{

class PathExport FeatureCompound: public Path::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Path::Feature);

public:
    /// Constructor
    FeatureCompound();
    ~FeatureCompound() override;

    App::PropertyLinkList Group;
    App::PropertyBool UsePlacements;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "PathGui::ViewProviderPathCompound";
    }
    App::DocumentObjectExecReturn* execute() override;

    /// Checks whether the object \a obj is part of this group.
    bool hasObject(const DocumentObject* obj) const;
    /// Adds an object to this group.
    void addObject(DocumentObject* obj);
    /// Removes an object from this group.
    void removeObject(DocumentObject* obj);
    PyObject* getPyObject() override;
};

using FeatureCompoundPython = App::FeaturePythonT<FeatureCompound>;

}  // namespace Path
