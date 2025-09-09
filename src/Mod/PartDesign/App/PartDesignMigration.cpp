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
#include <App/Property.h>
#include <App/PropertyLinks.h>
#include <Base/Type.h>               // Base::Type::fromName
#include <App/PropertyLinks.h>       // PropertyLinkSub
#include <App/GeoFeature.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/PartDesign/App/FeatureLegacyTipAdapter.h>


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
    doc->recompute(); // ensure body has a shape

    // after creating ssb (the SubShapeBinder)
    auto* ada = static_cast<PartDesign::LegacyTipAdapter*>(
        doc->addObject("PartDesign::LegacyTipAdapter",
                       (std::string(body->getNameInDocument()) + "_LegacyTip").c_str()));

    body->addObject(ada);
    ada->Label.setValue(std::string(body->Label.getStrValue()) + " (Migrated Tip)");
    ada->Binder.setValue(ssb);
    doc->recompute(); // ensure adapter has a shape
    body->Tip.setValue(ada);

    return true;
}

// Container root = nearest App::Part if present, else the document pointer
static const void* containerRoot(App::DocumentObject* o)
{
    if (!o) return nullptr;
    if (auto* p = App::Part::getPartOfObject(o)) return p;
    return o->getDocument();
}


// Global placement of any GeoFeature: containerCS(gf) · gf->Placement
static Base::Placement globalOf(App::DocumentObject* o,
                                const std::function<Base::Placement(App::GeoFeature*)>& containerCS) {
    if (auto* gf = dynamic_cast<App::GeoFeature*>(o))
        return containerCS(gf) * gf->Placement.getValue();
    return Base::Placement(); // identity
}

// Safe reference check: is 'obj' referenced by anyone other than 'ignore'?
static bool hasExternalRefs(App::DocumentObject* obj, App::DocumentObject* ignore) {
    const auto& in = obj->getInList();
    for (auto* o : in) if (o != ignore) return true;
    return false;
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

        // Collect supports and find the first one that belongs to a different Body
        std::vector<App::DocumentObject*> supObjs;
        collectSupportObjects(obj, supObjs);

        PartDesign::Body* B2 = nullptr;
        for (auto* s : supObjs) {
            if (auto* b = ownerBody(s)) { if (b != B1) { B2 = b; break; } }
        }
        if (!B2) continue; // all supports intra-body

        // Allow when both bodies share the same container root:
        //  - same real App::Part, OR
        //  - both are top-level in the same document
        const void* root1 = containerRoot(B1);
        const void* root2 = containerRoot(B2);
        if (root1 != root2) continue;

        auto* B1gf = dynamic_cast<App::GeoFeature*>(B1);
        auto* B2gf = dynamic_cast<App::GeoFeature*>(B2);
        if (!B1gf || !B2gf) continue;

        const Placement C1 = containerCS(B1gf);
        const Placement C2 = containerCS(B2gf);

        auto it = legacyP.find(B2);
        if (it == legacyP.end())
            continue; // target body not part of the migration set

        const Placement P2 = it->second;

        // We want:     (new world) = C1 * binder.Placement
        // to match:    (old world) = C2 * P2 * old-binder-local
        // Preserve any existing local offset by post-multiplying.
        Placement T = C1.inverse() * C2 * P2;

        // Freeze Relative if present so our explicit Placement stays stable
        if (auto* rel = dynamic_cast<App::PropertyBool*>(obj->getPropertyByName("Relative")))
            rel->setValue(false);

        binderGF->Placement.setValue(T * binderGF->Placement.getValue());
        binderGF->touch();
    }
}


// 1) Classify a legacy plane object to a *role* string ("XY_Plane"/"XZ_Plane"/"YZ_Plane")
static const char* planeRoleOf(const App::DocumentObject* planeObj) {
    if (!planeObj) return nullptr;
    const char* n = planeObj->getNameInDocument();
    if (!n) return nullptr;

    // Strip trailing digits so "XY_Plane002" -> "XY_Plane"
    std::string s(n);
    while (!s.empty() && std::isdigit(static_cast<unsigned char>(s.back()))) s.pop_back();

    // Match against native constant roles
    using LCS = App::LocalCoordinateSystem;
    for (const char* r : LCS::PlaneRoles) if (s == r) return r;
    return nullptr; // not a known plane role
}

// 2) Get the *canonical* subobject under an Origin by *role*
static App::DocumentObject* getPlaneByRole(App::Origin* org, const char* role) {
    if (!org || !role) return nullptr;
    using LCS = App::LocalCoordinateSystem;
    if      (std::strcmp(role, LCS::PlaneRoles[0]) == 0) return org->getXY();
    else if (std::strcmp(role, LCS::PlaneRoles[1]) == 0) return org->getXZ();
    else if (std::strcmp(role, LCS::PlaneRoles[2]) == 0) return org->getYZ();
    return nullptr;
}

// ---- Support helpers: handle Support/AttachmentSupport (Link/XLink, single/list) ----
struct SupportView {
    App::Property* prop = nullptr;
    App::DocumentObject* target = nullptr;         // first linked object (or nullptr)
    std::vector<std::string> subs;                 // its sublist (may be empty)
    enum Kind { None, LinkSub, LinkSubList, XLinkSub, XLinkSubList } kind = None;
};

static SupportView readSketchSupport(App::DocumentObject* sk)
{
    SupportView sv;
    if (!sk) return sv;

    // Prefer AttachmentSupport (what your file has), then fall back to Support
    const char* candidates[] = {"AttachmentSupport", "Support"};

    for (const char* name : candidates) {
        if (auto* pl = dynamic_cast<App::PropertyLinkSubList*>(sk->getPropertyByName(name))) {
            sv.prop = pl;
            const auto& links   = pl->getValues();
            const auto& sublist = pl->getSubValues();        // aligned with links
            if (!links.empty()) {
                sv.target = links.front();
                if (!sublist.empty()) { sv.subs.clear(); sv.subs.push_back(sublist.front()); }
                sv.kind = SupportView::LinkSubList;
                return sv;
            }
        }
        if (auto* ps = dynamic_cast<App::PropertyLinkSub*>(sk->getPropertyByName(name))) {
            sv.prop   = ps;
            sv.target = ps->getValue();
            sv.subs   = ps->getSubValues();
            sv.kind   = SupportView::LinkSub;
            if (sv.target) return sv;
        }
        if (auto* xpl = dynamic_cast<App::PropertyXLinkSubList*>(sk->getPropertyByName(name))) {
            sv.prop = xpl;
            const auto& links = xpl->getValues();
            if (!links.empty()) {
                sv.target = links.front();
                const auto& sublist = xpl->getSubValues(links.front()); // needs object
                if (!sublist.empty()) { sv.subs.clear(); sv.subs.push_back(sublist.front()); }
                sv.kind = SupportView::XLinkSubList;
                return sv;
            }
        }
        if (auto* xps = dynamic_cast<App::PropertyXLinkSub*>(sk->getPropertyByName(name))) {
            sv.prop   = xps;
            sv.target = xps->getValue();
            sv.subs   = xps->getSubValues();
            sv.kind   = SupportView::XLinkSub;
            if (sv.target) return sv;
        }
    }

    return sv; // None
}

static void writeSketchSupportPlane(App::DocumentObject* sk,
                                    App::DocumentObject* planeObj,
                                    const std::vector<std::string>& subs = {})
{
    if (!sk || !planeObj) return;

    // Prefer AttachmentSupport, then Support
    const char* candidates[] = {"AttachmentSupport", "Support"};

    for (const char* name : candidates) {
        if (auto* pl = dynamic_cast<App::PropertyLinkSubList*>(sk->getPropertyByName(name))) {
            std::vector<App::DocumentObject*> objs{ planeObj };
            pl->setValues(objs, subs);
            return;
        }
        if (auto* ps = dynamic_cast<App::PropertyLinkSub*>(sk->getPropertyByName(name))) {
            ps->setValue(planeObj, subs);
            return;
        }
        if (auto* xpl = dynamic_cast<App::PropertyXLinkSubList*>(sk->getPropertyByName(name))) {
            std::vector<App::DocumentObject*> objs{ planeObj };
            xpl->setValues(objs, subs);
            return;
        }
        if (auto* xps = dynamic_cast<App::PropertyXLinkSub*>(sk->getPropertyByName(name))) {
            xps->setValue(planeObj, subs);
            return;
        }
    }
}


static App::Origin* getOrCreateGlobalOrigin(App::Document* doc) {
    if (!doc) return nullptr;
    // Reuse existing one if present
    if (auto* obj = doc->getObject("Global_Origin"))
        if (auto* og = dynamic_cast<App::Origin*>(obj)) return og;

    // Create a document-level origin and hide it
    auto* og = static_cast<App::Origin*>(doc->addObject("App::Origin", "Global_Origin"));
    if (og) {
        og->Label.setValue("Global Origin");
    }
    return og;
}


static std::vector<App::Origin*> findTopLevelOrigins(App::Document* doc)
{
    std::vector<App::Origin*> out;
    if (!doc) return out;
    for (auto* o : doc->getObjectsOfType(App::Origin::getClassTypeId())) {
        auto* og = static_cast<App::Origin*>(o);
        if (!App::Part::getPartOfObject(og))  // no parent Part ⇒ top-level
            out.push_back(og);
    }
    return out;
}


// Remove an origin's children and the origin itself if no external refs remain.
// Returns true if removed (or nothing to remove), false if kept due to refs.
static bool removeOriginIfUnreferenced(App::Document* doc, App::Origin* og)
{
    if (!doc || !og) return true;
    
    if (hasExternalRefs(og, nullptr)) return false;

    // If any child is still referenced from outside the origin, keep everything.
    for (auto* ch : og->baseObjects()) {
        if (hasExternalRefs(ch, og))
            return false;
    }

    // Remove children first
    for (auto* ch : og->baseObjects()) {
        if (!hasExternalRefs(ch, og))
            doc->removeObject(ch->getNameInDocument());
    }

    // Now remove the origin if nothing references it
    if (!hasExternalRefs(og, nullptr))
        doc->removeObject(og->getNameInDocument());

    return true;
}



// For each App::Part in the document, relink supports of sketches that live in Bodies under that Part
// from any *origin feature* that is NOT the Part's own origin, to the matching plane of the Part's Origin,
// adjusting AttachmentOffset so world placement stays identical:
//   A' = G_new^{-1} · G_old · A
static void relinkInPartSketchesKeepWorld(
    App::Document* doc,
    const std::function<Base::Placement(App::GeoFeature*)>& containerCS)
{
    if (!doc) return;

        // Helper to get global placement for any object we may target here
    auto getGlobal = [&](App::DocumentObject* obj) -> Base::Placement {
         if (auto* gf = dynamic_cast<App::GeoFeature*>(obj))
             return globalOf(gf, containerCS);   // your existing helper
         // Top-level canonical origin has identity in world; if not GeoFeature, fallback to identity
         return Base::Placement();
    };
        

    // Walk all Parts present in the document
    auto parts = doc->getObjectsOfType(App::Part::getClassTypeId());
    for (auto* po : parts) {
        auto* part = static_cast<App::Part*>(po);
        auto* partOrigin = part->getOrigin();
        if (!partOrigin) continue;

        // Map of the Part origin's planes by base label, and their globals
       std::set<std::string> partOriginFeatures;
       for (auto* ch : partOrigin->baseObjects()) partOriginFeatures.insert(ch->getNameInDocument());

        // Relink each sketch whose *owner body* is under this Part
        auto sketchType = Base::Type::fromName("Sketcher::SketchObject");
        std::vector<App::DocumentObject*> sketches = doc->getObjectsOfType(sketchType);
        for (auto* sk : sketches) {
            auto* body = PartDesign::Body::findBodyOf(sk);
            if (!body) continue;
            if (App::Part::getPartOfObject(body) != part) continue; // only bodies under this Part

            auto sv = readSketchSupport(sk);
            if (!sv.prop || !sv.target) continue;

            App::DocumentObject* tgt = sv.target;
            if (!tgt) continue;


            // Resolve actual old plane + base label for both forms
            App::DocumentObject* oldPlane = nullptr;
            
            if (auto* tgtOrigin = dynamic_cast<App::Origin*>(tgt)) {
                const std::vector<std::string>& subs = sv.subs;   // may be empty
                if (!subs.empty() && tgtOrigin != partOrigin) {
                    // 1) Fast path: try exact document object by subname
                    oldPlane = sk->getDocument()->getObject(subs.front().c_str());
                }
            }
            
            if (!oldPlane)
                continue;

            // Counterpart plane under this Part's origin
            auto itPlane = partOriginFeatures.find(oldPlane->getNameInDocument());
            if (itPlane == partOriginFeatures.end()) continue;

            // using role of oldPlane
            const char* role = planeRoleOf(oldPlane);
            if (!role) continue;
            App::DocumentObject* newTargetObj = partOrigin;   // your Part_Origin
            std::vector<std::string> newSubs;
            newSubs.emplace_back(role);                      // {"XY_Plane"} | {"XZ_Plane"} | {"YZ_Plane"}
        
            auto* offProp = dynamic_cast<App::PropertyPlacement*>(sk->getPropertyByName("AttachmentOffset"));
            Base::Placement A = offProp ? offProp->getValue() : Base::Placement();
            Base::Placement g_old = getGlobal(oldPlane);
            Base::Placement g_new = newSubs.empty()
               ? getGlobal(newTargetObj)
               : getGlobal(getPlaneByRole(partOrigin, role));

            Base::Placement A_prime = g_new.inverse() * g_old * A;

            writeSketchSupportPlane(sk, newTargetObj, newSubs);
            if (offProp) offProp->setValue(A_prime);
            sk->touch();
        }

        // ---- Cleanup *body-local* legacy origins inside Bodies, and any extra origins under this Part ----
        {
            // A) body-local origins
            auto bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
            for (auto* o : bodies) {
                auto* body = static_cast<PartDesign::Body*>(o);
                for (auto* child : body->Group.getValues()) {
                    if (auto* og = dynamic_cast<App::Origin*>(child)) {
                        removeOriginIfUnreferenced(doc, og);
                    }
                }
            }
        
            // B) origins under this Part (if present), except the Part's canonical origin
            if (part && partOrigin) {
                for (auto* o : doc->getObjectsOfType(App::Origin::getClassTypeId())) {
                    auto* og = static_cast<App::Origin*>(o);
                    if (App::Part::getPartOfObject(og) == part && og != partOrigin)
                        removeOriginIfUnreferenced(doc, og);
                }
            }
        }


    }
}



static void relinkTopLevelSketchesKeepWorld(
    App::Document* doc,
    App::Origin* canonical,
    const std::function<Base::Placement(App::GeoFeature*)>& containerCS)
{
    if (!doc || !canonical) return;

    // 2) Walk sketches in *top-level* Bodies and relink if owner != canonical
    auto sketchType = Base::Type::fromName("Sketcher::SketchObject");
    std::vector<App::DocumentObject*> sketches = doc->getObjectsOfType(sketchType);

    for (auto* sk : sketches) {
        auto* body = PartDesign::Body::findBodyOf(sk);
        if (!body || App::Part::getPartOfObject(body)) continue; // only top-level bodies

        auto sv = readSketchSupport(sk);
        if (!sv.prop || !sv.target) continue;

        App::DocumentObject* tgt = sv.target;

        if (!tgt) continue;


        App::DocumentObject* oldPlane = nullptr;
        App::Origin*         oldOwner = nullptr;
        
        if (auto* tgtOrigin = dynamic_cast<App::Origin*>(tgt)) {
            if (App::Part::getPartOfObject(tgtOrigin))                 // not a top-level origin
                continue;
        
            const std::vector<std::string>& subs = sv.subs;            // may be empty
            if (subs.empty())
                continue;
        
            // 1) Fast path: sub name is usually the child's document name
            oldPlane = sk->getDocument()->getObject(subs.front().c_str());        
            if (!oldPlane)
                continue;
        
            oldOwner = tgtOrigin;
        
        }
        // Not origin-based support
        else {
            continue;
        }

        // Already on canonical origin? nothing to do
        if (oldOwner == canonical) continue;

        // Counterpart under canonical

        // Helper to get global placement for any object we may target here
        auto getGlobal = [&](App::DocumentObject* obj) -> Base::Placement {
            if (auto* gf = dynamic_cast<App::GeoFeature*>(obj))
                return globalOf(gf, containerCS);   // your existing helper
            // Top-level canonical origin has identity in world; if not GeoFeature, fallback to identity
            return Base::Placement();
        };
        
        
        // --- Always attach to Origin + subelement (no plane-object fast path) ---
        
        // using role of oldPlane
        const char* role = planeRoleOf(oldPlane);
        if (!role) continue;
        App::DocumentObject* newTargetObj = canonical;   // your Global_Origin
        std::vector<std::string> newSubs;
        newSubs.emplace_back(role);                      // {"XY_Plane"} | {"XZ_Plane"} | {"YZ_Plane"}
        
        // Preserve world: A' = G_new^{-1} · G_old · A
        auto* offProp = dynamic_cast<App::PropertyPlacement*>(
            sk->getPropertyByName("AttachmentOffset"));
        Base::Placement A = offProp ? offProp->getValue() : Base::Placement();
        
        Base::Placement g_old = getGlobal(oldPlane);
        Base::Placement g_new = getGlobal(getPlaneByRole(canonical, role)); // frame of the subelement
        
        Base::Placement A_prime = g_new.inverse() * g_old * A;
        
        // Write support + offset
        writeSketchSupportPlane(sk, newTargetObj, newSubs);
        if (offProp) offProp->setValue(A_prime);
        sk->touch();
    }

    // ---- Cleanup extra *top-level* origins and their children (not the canonical) ----
    {
        std::vector<App::Origin*> tops = findTopLevelOrigins(doc);
        for (auto* og : tops) {
           if (og == canonical) continue;
           removeOriginIfUnreferenced(doc, og);
        }
    }

}


} // namespace

// --- public entry -----------------------------------------------------------

// PartDesignMigration.cpp

namespace PartDesign {

void migrateLegacyBodyPlacements(App::Document* doc)
{
    if (!doc) return;

    // --- gather bodies
    std::vector<PartDesign::Body*> bodies;
    for (auto* o : doc->getObjectsOfType(PartDesign::Body::getClassTypeId()))
        if (auto* b = dynamic_cast<PartDesign::Body*>(o))
            bodies.push_back(b);
    if (bodies.empty()) return;

    auto isTopLevelBody = [](PartDesign::Body* b) -> bool {
        return App::Part::getPartOfObject(b) == nullptr;
    };

    bool haveTopLevel = false;
    bool anyLegacyPlacement = false;
    for (auto* b : bodies) {
        haveTopLevel |= isTopLevelBody(b);
        if (!b->Placement.getValue().isIdentity())
            anyLegacyPlacement = true;
    }

    // If truly nothing to do, exit (but we’ll still create a Global Origin when top-level bodies exist)
    if (!haveTopLevel && !anyLegacyPlacement) {
        return;
    }

    doc->openTransaction("PartDesign: migrate legacy origins / placements");

    // 1) Ensure ONE canonical top-level origin if there are top-level bodies
    App::Origin* gGlobalOrigin = nullptr;
    if (haveTopLevel) {
        gGlobalOrigin = getOrCreateGlobalOrigin(doc); // this should reuse existing or create "Global_Origin"
        Base::Console().message("[PD-Migrate] canonical top-level origin: %s\n",
            gGlobalOrigin ? gGlobalOrigin->getNameInDocument() : "<none>");
    }

    // 2) Relink SKETCH supports (world-preserving)
    // 2a) Top-level Bodies → canonical top-level origin planes
    if (gGlobalOrigin) {
        Base::Console().message("[PD-Migrate] relink top-level sketches → top-level origin\n");
        relinkTopLevelSketchesKeepWorld(doc, gGlobalOrigin,
            [&](App::GeoFeature* gf){ return containerCS(gf); });
    }

    // 2b) Bodies inside a Part → that Part’s origin planes
    Base::Console().message("[PD-Migrate] relink in-Part sketches → Part origins\n");
    relinkInPartSketchesKeepWorld(doc,
        [&](App::GeoFeature* gf){ return containerCS(gf); });

    // 3) Snapshot legacy placements for ALL bodies
    std::map<PartDesign::Body*, Base::Placement> legacyP;
    for (auto* b : bodies)
        legacyP[b] = b->Placement.getValue();

    bool anyLegacyBodyPlacement = false;
    for (const auto& kv : legacyP) {
        if (!kv.second.isIdentity()) { anyLegacyBodyPlacement = true; break; }
    }

    if (anyLegacyBodyPlacement) {
        Base::Console().message("[PD-Migrate] wrap tips (push Body.Placement to Tip)\n");
        migrateBodyTips(doc, bodies, legacyP);

        Base::Console().message("[PD-Migrate] adjust inter-body binders\n");
        migrateInterBodyBinders(doc, legacyP); // IMPORTANT: same container-root guard inside

        Base::Console().message("[PD-Migrate] clear Body.Placement\n");
        for (auto* b : bodies)
            if (!legacyP[b].isIdentity())
                b->Placement.setValue(Base::Placement()); // identity
    }

    doc->commitTransaction();

    Base::Console().message("[PD-Migrate] recompute\n");
    doc->recompute();

}

} // namespace PartDesign

