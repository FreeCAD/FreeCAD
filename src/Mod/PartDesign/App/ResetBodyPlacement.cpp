/***************************************************************************
 *   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com> *
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



#include "ResetBodyPlacement.h"

#include <App/Origin.h>
#include <App/Datums.h>  // for AxisRoles / PlaneRoles / PointRoles
#include <App/Part.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/Link.h>
#include <App/PropertyStandard.h>
#include <App/Property.h>
#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>

#include <Base/Type.h>               // Base::Type::fromName
#include <Base/Placement.h>
#include <Base/CoordinateSystem.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ShapeBinder.h>


#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>   // std::replace
#include <cctype>      // std::tolower, std::isdigit
#include <cstring>


using Base::Placement;

namespace {

// --- helpers ---------------------------------------------------------------

template <typename T>
static std::vector<T*> getObjectsOf(App::Document* doc)
{
    std::vector<T*> out;
    if (!doc) return out;
    for (auto* o : doc->getObjectsOfType(T::getClassTypeId()))
        if (auto* t = dynamic_cast<T*>(o)) out.push_back(t);
    return out;
}


static bool hasEffectiveAttachment(App::DocumentObject* obj)
{
    if (!obj) return false;
    if (!obj->hasExtension(Part::AttachExtension::getExtensionClassTypeId()))
        return false;

    auto* ext = obj->getExtensionByType<Part::AttachExtension>();
    if (!ext) return false;

    return ext->AttachmentSupport.getValue() != nullptr;
}


} // namespace

// --- public entry -----------------------------------------------------------

// PartDesignMigration.cpp

namespace PartDesign {


void resetBodyPlacement(PartDesign::Body* body)
{
    if (!body) return;

    const Base::Placement Pbody = body->Placement.getValue();
    if (Pbody.isIdentity()) return;

    if (auto* origin = body->getOrigin()) {
        if (auto* pl = dynamic_cast<App::PropertyPlacement*>(
                origin->getPropertyByName("Placement"))) {
            pl->setValue(Pbody * pl->getValue());
        }
    }

    for (auto* ch : body->Group.getValues()) {
        if (!ch) continue;
        if (ch == body->getOrigin()) continue;

        // If attached to a supporto don't touch, it will follow the support.
        if (hasEffectiveAttachment(ch))
            continue;

        // Otherwise, if it has Placement premultiply it by Pbody 
        if (auto* pl = dynamic_cast<App::PropertyPlacement*>(
                ch->getPropertyByName("Placement"))) {
            pl->setValue(Pbody * pl->getValue()); // PRE-moltiplica
        }
    }

    // Body Placement reset
    body->Placement.setValue(Base::Placement()); // identity

}

void resetBodiesPlacements(App::Document* doc)
{
    if (!doc) return;

    // Tutti i Body del documento
    std::vector<App::DocumentObject*> bodies =
        doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    //Snapshot legacy placements for ALL bodies
    bool anyLegacyBodyPlacement = false;
    for (auto* b : bodies){
        auto* body = dynamic_cast<PartDesign::Body*>(b);
        const Base::Placement Pbody = body->Placement.getValue();
	if(!Pbody.isIdentity()) anyLegacyBodyPlacement = true;
    }
    if (!anyLegacyBodyPlacement) return;

    doc->openTransaction("PartDesign: migrate legacy origins / placements");
    for (auto* b : bodies) {
        auto* body = dynamic_cast<PartDesign::Body*>(b);
        resetBodyPlacement(body);
    }
    Base::Console().message("[PD-Migrate] Cleared body placements\n");
    doc->commitTransaction();

    // Recompute una sola volta al termine
    doc->recompute();
}



} // namespace PartDesign

