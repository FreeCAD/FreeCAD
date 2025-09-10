/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "PreCompiled.h"
#include <Mod/PartDesign/App/Feature.h>
#include "FeatureLegacyTipAdapter.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::LegacyTipAdapter, PartDesign::Feature)

LegacyTipAdapter::LegacyTipAdapter()
{
    ADD_PROPERTY_TYPE(BaseObject,(nullptr),"Base",
                      App::PropertyType(App::Prop_None),
                      "Optional, accepts any object (e.g. LCS/Datum) for UI nesting/icon");
}

App::DocumentObjectExecReturn* LegacyTipAdapter::execute()
{
    // Produce our Shape from a *real* Part::Feature if present
    Part::TopoShape shp;

    if (auto* pf = dynamic_cast<Part::Feature*>(BaseFeature.getValue())) {
        shp = pf->Shape.getShape();
    } else {
        // No Part::Feature base — this adapter acts as a transform-only container
        // (Shape stays whatever it was; typically empty). That’s fine for Tip migration
        // if you always link BaseFeature to the former Tip feature.
    }

    if (!shp.getShape().IsNull())
        Shape.setValue(shp);

    return App::DocumentObject::StdReturn;
}

const char* LegacyTipAdapter::getViewProviderName() const
{
    return "PartDesignGui::ViewProviderLegacyTipAdapter";
}

