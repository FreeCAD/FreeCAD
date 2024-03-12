/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef POINTS_FEATURE_H
#define POINTS_FEATURE_H

#include <App/FeatureCustom.h>
#include <App/FeaturePython.h>
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>

#include "Points.h"
#include "PropertyPointKernel.h"


namespace Base
{
class Writer;
}

namespace App
{
class Color;
}

namespace Points
{
class Property;
class PointsFeaturePy;

/** Base class of all Points feature classes in FreeCAD.
 * This class holds an PointsKernel object.
 */
class PointsExport Feature: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Points::Feature);

public:
    /// Constructor
    Feature();

    /** @name methods override Feature */
    //@{
    void Restore(Base::XMLReader& reader) override;
    void RestoreDocFile(Base::Reader& reader) override;
    short mustExecute() const override;
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "PointsGui::ViewProviderScattered";
    }

    const App::PropertyComplexGeoData* getPropertyOfGeometry() const override
    {
        return &Points;
    }

protected:
    void onChanged(const App::Property* prop) override;
    //@}

public:
    PropertyPointKernel Points; /**< The point kernel property. */
};

using FeatureCustom = App::FeatureCustomT<Feature>;
using FeaturePython = App::FeaturePythonT<Feature>;

}  // namespace Points


#endif
