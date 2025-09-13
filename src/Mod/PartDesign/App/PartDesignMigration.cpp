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



#include "PartDesignMigration.h"

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

static PartDesign::Body* ownerBody(App::DocumentObject* o)
{
    return o ? PartDesign::Body::findBodyOf(o) : nullptr;
}

/// Best-effort gather of support objects from (Sub)ShapeBinder.
/// Works across versions (Support can be PropertyLinkSubList or PropertyXLinkSubList).
static void collectSupportObjects(App::DocumentObject* binder,
                                  std::vector<App::DocumentObject*>& out)
{
    out.clear();
    if (!binder) return;

    // Primary path: "Support" as LinkSub or XLinkSub list
    if (auto* prop = dynamic_cast<App::Property*>(binder->getPropertyByName("Support"))) {

        if (auto* pL = dynamic_cast<App::PropertyLinkSubList*>(prop)) {
            const auto vals = pL->getValues(); // std::vector<App::DocumentObject*>
            for (auto* o : vals) if (o) out.push_back(o);
        }

        if (auto* pXL = dynamic_cast<App::PropertyXLinkSubList*>(prop)) {
            const auto vals = pXL->getValues(); // std::vector<App::DocumentObject*>
            for (auto* o : vals) if (o) out.push_back(o);
        }

        if (!out.empty()) {
            std::sort(out.begin(), out.end());
            out.erase(std::unique(out.begin(), out.end()), out.end());
            return;
        }
    }

    // Fallbacks seen in some PD objects ("Object"/"Objects")
    if (auto* p = dynamic_cast<App::PropertyLink*>(binder->getPropertyByName("Object"))) {
        if (auto* o = p->getValue()) out.push_back(o);
    }
    if (auto* pl = dynamic_cast<App::PropertyLinkList*>(binder->getPropertyByName("Objects"))) {
        for (auto* o : pl->getValues()) if (o) out.push_back(o);
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

static void migrateInterBodyBinders(App::Document* doc,
                                    const std::map<PartDesign::Body*, Base::Placement>& legacyP)
{
    if (!doc) return;
    // Gather both binder kinds
    std::vector<App::DocumentObject*> binders;
    for (auto* o : doc->getObjectsOfType(PartDesign::ShapeBinder::getClassTypeId()))    binders.push_back(o);
    for (auto* o : doc->getObjectsOfType(PartDesign::SubShapeBinder::getClassTypeId())) binders.push_back(o);

    for (auto* obj : binders) {
        auto* binderGF = dynamic_cast<App::GeoFeature*>(obj);
        if (!binderGF) continue;

        PartDesign::Body* B1 = ownerBody(obj);   // binder's owner body
        if (!B1) continue;

        // We only need this for binders that reference something outside their body.
        // (Intra-body binders don't need adjustment: zeroing B1 affects both binder & supports equally.)
        bool interBody = false;
        {
            std::vector<App::DocumentObject*> supObjs;
            collectSupportObjects(obj, supObjs);
            for (auto* s : supObjs) {
                PartDesign::Body* bs = ownerBody(s);
                if (bs && bs != B1) { interBody = true; break; }
            }
        }
        if (!interBody) continue;

        // Legacy placement of the binder's *owner* body
        auto itP1 = legacyP.find(B1);
        if (itP1 == legacyP.end()) continue;
        const Base::Placement P1 = itP1->second;

        // Freeze relative behavior so Placement is interpreted in doc/container frame
        if (auto* rel = dynamic_cast<App::PropertyBool*>(obj->getPropertyByName("Relative")))
            rel->setValue(false);

        // Pre-multiply by P1 so that after Body is zeroed, world pose is preserved:
        //   world_new = C1 * (P1 * L) = C1 * P1 * L = world_old
        const Base::Placement L = binderGF->Placement.getValue();
        binderGF->Placement.setValue(P1 * L);
        binderGF->touch();
    }
}

// A child is considered “loose-placement-only” iff:
//   (a) It has a Placement property, and
//   (b) BOTH ‘AttachmentSupport’ and ‘Support’ are empty (across Link/XLink/Sub/List variants).
//
// RATIONALE
// ---------
// • PD datums/features & binders expose their geometric dependency via
//   AttachmentSupport/Support → we DO NOT bake those here. They are handled
//   either by migrateInterBodyBinders() (inter-body binders) or they move
//   coherently when the body goes to identity.
// • App::Link uses ‘Link’ (not ‘Support’). We deliberately treat Links as
//   “placement-only” so we DO bake P_body into their Placement; otherwise
//   zeroing the body would change their world pose.
// • If a binder was turned into an “independent copy” (supports cleared),
//   it becomes placement-only by design and is baked here once.
static bool isLoosePlacementOnly(App::DocumentObject* o)
{
    if (!o) return false;
    // If it has one of the attachment support props with a value, it’s not “loose”
    for (const char* n : {"AttachmentSupport","Support"}) {
        if (auto* p = o->getPropertyByName(n)) {
            if (auto* l  = dynamic_cast<App::PropertyLink*>(p))          { if (l->getValue()) return false; }
            if (auto* ls = dynamic_cast<App::PropertyLinkSub*>(p))       { if (ls->getValue()) return false; }
            if (auto* ll = dynamic_cast<App::PropertyLinkList*>(p))      { if (!ll->getValues().empty()) return false; }
            if (auto* lsl= dynamic_cast<App::PropertyLinkSubList*>(p))   { if (!lsl->getValues().empty()) return false; }
            if (auto* xl = dynamic_cast<App::PropertyXLink*>(p))         { if (xl->getValue()) return false; }
            if (auto* xls= dynamic_cast<App::PropertyXLinkSub*>(p))      { if (xls->getValue()) return false; }
            if (auto* xll= dynamic_cast<App::PropertyXLinkList*>(p))     { if (!xll->getValues().empty()) return false; }
            if (auto* xlsl= dynamic_cast<App::PropertyXLinkSubList*>(p)) { if (!xlsl->getValues().empty()) return false; }
        }
    }
    // Has a Placement? Then it’s a loose-placement object we should bake.
    return dynamic_cast<App::PropertyPlacement*>(o->getPropertyByName("Placement")) != nullptr;
}


static void resetBodyPlacements(App::Document* doc, bool adjustLooseChildren=true)
{
    if (!doc) return;

    // Snapshot all Origin objects once
    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    // Single pass, no recompute inside the loop
    for (auto* b : bodies) {
        auto* body = dynamic_cast<PartDesign::Body*>(b);
	auto* origin = body->getOrigin();

        // Snapshot placements
        const Base::Placement Pbody   = body->Placement.getValue();
        const Base::Placement Porigin =
            (dynamic_cast<App::PropertyPlacement*>(origin->getPropertyByName("Placement")))
                ? static_cast<App::PropertyPlacement*>(origin->getPropertyByName("Placement"))->getValue()
                : Base::Placement();

        // Optionally bake Pbody into loose-placement-only children BEFORE zeroing body
        if (adjustLooseChildren) {
            for (auto* ch : body->Group.getValues()) {
                if (!ch || ch == origin) continue;
                if (!isLoosePlacementOnly(ch)) continue;
                if (auto* pl = dynamic_cast<App::PropertyPlacement*>(ch->getPropertyByName("Placement")))
                    pl->setValue(Pbody * pl->getValue());
            }
        }

        // Atomic swap for this body: zero body, then bake origin
        body->Placement.setValue(Base::Placement());           // identity
        if (auto* pl = dynamic_cast<App::PropertyPlacement*>(origin->getPropertyByName("Placement")))
            pl->setValue(Pbody * Porigin);
    }

    // One recompute at the very end
    doc->recompute();
}



} // namespace

// --- public entry -----------------------------------------------------------

// PartDesignMigration.cpp

namespace PartDesign {

void migrateLegacyBodyPlacements(App::Document* doc)
{
    if (!doc) return;

    // --- gather bodies
    std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
    if (bodies.empty()) return;

    //Snapshot legacy placements for ALL bodies
    std::map<PartDesign::Body*, Base::Placement> legacyP;
    for (auto* b : bodies){
        auto* body = dynamic_cast<PartDesign::Body*>(b);
        legacyP[body] = body->Placement.getValue();
    }

    bool anyLegacyBodyPlacement = false;
    for (const auto& kv : legacyP) {
        if (!kv.second.isIdentity()) { anyLegacyBodyPlacement = true; break; }
    }
    // If truly nothing to do, exit (but we’ll still create a Global Origin when top-level bodies exist)
    if (!anyLegacyBodyPlacement) return;

    doc->openTransaction("PartDesign: migrate legacy origins / placements");
    if (anyLegacyBodyPlacement) {
        Base::Console().message("[PD-Migrate] Adjusting inter-body binders\n");
        migrateInterBodyBinders(doc, legacyP); // IMPORTANT: same container-root guard inside
        Base::Console().message("[PD-Migrate] Clearing body placements\n");
        resetBodyPlacements(doc);
    }

    doc->commitTransaction();

    Base::Console().message("[PD-Migrate] recompute\n");
    doc->recompute();

}

} // namespace PartDesign

