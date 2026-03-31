// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/FeatureCustom.h>
#include <App/FeaturePython.h>
#include <App/GeoFeature.h>  // must be before FeatureCustom.h

#include "Core/MeshKernel.h"

#include "Mesh.h"
#include "MeshProperties.h"


namespace Base
{
class XMLReader;
class Writer;
}  // namespace Base

namespace MeshCore
{
class MeshKernel;
}

namespace Mesh
{

class MeshFeaturePy;

/** Base class of all mesh feature classes in FreeCAD.
 * This class holds a MeshKernel object.
 * \author Werner Mayer
 */
class MeshExport Feature: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Mesh::Feature);

public:
    /// Constructor
    Feature();

    /** @name Properties */
    //@{
    /** Property that holds the mesh data. */
    PropertyMeshKernel Mesh;
    //@}

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "MeshGui::ViewProviderMeshFaceSet";
    }
    const App::PropertyComplexGeoData* getPropertyOfGeometry() const override
    {
        return &Mesh;
    }

    /// handles the MeshPy object
    PyObject* getPyObject() override;
};

using FeatureCustom = App::FeatureCustomT<Feature>;
using FeaturePython = App::FeaturePythonT<Feature>;

}  // namespace Mesh
