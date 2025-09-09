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
#include <Mod/PartDesign/App/ShapeBinder.h>
#include "FeatureLegacyTipAdapter.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::LegacyTipAdapter, PartDesign::Feature)

LegacyTipAdapter::LegacyTipAdapter()
{
    ADD_PROPERTY_TYPE(Binder,(nullptr),"Base", App::PropertyType(App::Prop_None),
                      "SubShapeBinder carrying the legacy tip");
}

App::DocumentObjectExecReturn* LegacyTipAdapter::execute()
{
    auto* ssb = dynamic_cast<PartDesign::SubShapeBinder*>(Binder.getValue());
    if (!ssb) return new App::DocumentObjectExecReturn("No SubShapeBinder");

    const auto shp = static_cast<Part::Feature*>(ssb)->Shape.getShape();
    if (shp.isNull()) return new App::DocumentObjectExecReturn("Binder shape is empty");

    this->Shape.setValue(Part::TopoShape(shp));
    this->Placement.setValue(ssb->Placement.getValue());
    return App::DocumentObject::StdReturn;
}

const char* LegacyTipAdapter::getViewProviderName() const
{
    return "PartDesignGui::ViewProviderLegacyTipAdapter";
}

