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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Property.h>
#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ShapeBinder.h>

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#include <string>

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

/// Container CS = parent's world *without* the object's own Placement
static Placement containerCS(App::GeoFeature* gf)
{
    // globalPlacement() includes parents *and* own Placement
    // Multiply by inverse(own) to peel the object's Placement off.
    const Placement own = gf->Placement.getValue();
    return gf->globalPlacement() * own.inverse();
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


/// Create a PartDesign::SubShapeBinder inside the body, pointing to the current Tip,
static bool wrapTipWithBinder(App::Document* doc, PartDesign::Body* body, const Base::Placement& P)
{
    if (!doc || !body || P.isIdentity()) return false;
    App::DocumentObject* tip = body->Tip.getValue();
    if (!tip) return false;

    // Build a safe name
    std::string objName = std::string(body->getNameInDocument()) + "_LegacyTip";

    auto* ssb = dynamic_cast<PartDesign::SubShapeBinder*>(
        doc->addObject("PartDesign::SubShapeBinder", objName.c_str()));
    if (!ssb) return false;

    // Ensure it’s under the Body so it can be the Tip
    body->addObject(ssb);

    // Bind the whole 'tip' shape
    if (auto* pXL = dynamic_cast<App::PropertyXLinkSubList*>(ssb->getPropertyByName("Support"))) {
        // Your build accepts a vector<DocumentObject*> for XLink
        pXL->setValues(std::vector<App::DocumentObject*>{ tip });
    } else if (auto* pL = dynamic_cast<App::PropertyLinkSubList*>(ssb->getPropertyByName("Support"))) {
        // LinkSubList expects two parallel vectors: objects & subnames
        std::vector<App::DocumentObject*> objs{ tip };
        std::vector<std::string> subs(1); // one empty string => whole object
        pL->setValues(objs, subs);
    } else if (auto* p = dynamic_cast<App::PropertyLink*>(ssb->getPropertyByName("Object"))) {
        p->setValue(tip);
    }

    // Freeze relative behavior (if present) and assign the legacy transform
    if (auto* rel = dynamic_cast<App::PropertyBool*>(ssb->getPropertyByName("Relative")))
        rel->setValue(false);

    ssb->Placement.setValue(P);
    ssb->Label.setValue(std::string(body->Label.getStrValue()) + " (Legacy Tip)");

    body->Tip.setValue(ssb);
    return true;
}


static void migrateBodyTips(App::Document* doc,
                            const std::vector<PartDesign::Body*>& bodies,
                            const std::map<PartDesign::Body*, Placement>& legacyP)
{
    for (auto* b : bodies) {
        const Placement& P = legacyP.at(b);
        if (P.isIdentity()) continue;
        wrapTipWithBinder(doc, b, P);
    }
}

static void migrateInterBodyBinders(App::Document* doc,
                                    const std::map<PartDesign::Body*, Placement>& legacyP)
{
    if (!doc) return;

    // Handle both classic ShapeBinder and SubShapeBinder
    std::vector<App::DocumentObject*> binders;
    for (auto* o : doc->getObjectsOfType(PartDesign::ShapeBinder::getClassTypeId())) binders.push_back(o);
    for (auto* o : doc->getObjectsOfType(PartDesign::SubShapeBinder::getClassTypeId())) binders.push_back(o);

    for (auto* obj : binders) {
        auto* binderGF = dynamic_cast<App::GeoFeature*>(obj);
        if (!binderGF) continue;

        auto* B1 = ownerBody(obj);
        if (!B1) continue; // orphan or outside bodies

        std::vector<App::DocumentObject*> supObjs;
        collectSupportObjects(obj, supObjs);

        // Find the first support that belongs to a different Body
        PartDesign::Body* B2 = nullptr;
        for (auto* s : supObjs) {
            if (auto* b = ownerBody(s)) { if (b != B1) { B2 = b; break; } }
        }
        if (!B2) continue; // all supports intra-body

        auto* B1gf = dynamic_cast<App::GeoFeature*>(B1);
        auto* B2gf = dynamic_cast<App::GeoFeature*>(B2);
        if (!B1gf || !B2gf) continue;

        const Placement C1 = containerCS(B1gf);
        const Placement C2 = containerCS(B2gf);
        const Placement P2 = legacyP.at(B2);

        // We want:     (new world) = C1 * binder.Placement
        // to match:    (old world) = C2 * P2 * old-binder-local
        // Most binders had no extra local Placement relative to support;
        // however, preserve any existing offset by post-multiplying.
        Placement T = C1.inverse() * C2 * P2;

        // Freeze Relative if present so our explicit Placement stays stable.
        if (auto* rel = dynamic_cast<App::PropertyBool*>(obj->getPropertyByName("Relative")))
            rel->setValue(false);

        binderGF->Placement.setValue(T * binderGF->Placement.getValue());
        binderGF->touch();
    }
}

/*
static bool needsMigration(const std::vector<PartDesign::Body*>& bodies)
{
    for (auto* b : bodies) {
        if (!b) continue;
        // Heuristic: old docs will have non-identity Body.Placement
        if (!b->Placement.getValue().isIdentity())
            return true;
    }
    return false;
}
*/

} // namespace

// --- public entry -----------------------------------------------------------

// PartDesignMigration.cpp

namespace PartDesign {

void migrateLegacyBodyPlacements(App::Document* doc)
{
    if (!doc) return;

    // Gather all PartDesign bodies
    std::vector<PartDesign::Body*> bodies;
    for (auto* o : doc->getObjectsOfType(PartDesign::Body::getClassTypeId()))
        if (auto* b = dynamic_cast<PartDesign::Body*>(o)) bodies.push_back(b);

    if (bodies.empty())
        return;

    // Check if there is anything to migrate (any non-identity Body.Placement)
    bool anyLegacy = false;
    for (auto* b : bodies) {
        if (!b) continue;
        if (!b->Placement.getValue().isIdentity()) { anyLegacy = true; break; }
    }
    if (!anyLegacy)
        return; // nothing to do for "new" documents

    // Snapshot legacy placements before mutations
    std::map<PartDesign::Body*, Base::Placement> legacyP;
    for (auto* b : bodies)
        legacyP[b] = b->Placement.getValue();

    // Do everything in one transaction; we won't toggle recomputes here
    doc->openTransaction("Migrate legacy PartDesign Body.Placement");

    // 1) Push each legacy Body.Placement to the end via a binder as new Tip
    migrateBodyTips(doc, bodies, legacyP);

    // 2) Adjust inter-body (Sub)ShapeBinders so they still point correctly
    migrateInterBodyBinders(doc, legacyP);

    // 3) Nullify Body placements
    for (auto* b : bodies) {
        const auto it = legacyP.find(b);
        if (it != legacyP.end() && !it->second.isIdentity())
            b->Placement.setValue(Base::Placement()); // identity
    }

    doc->commitTransaction();
    doc->recompute();
}

} // namespace PartDesign

