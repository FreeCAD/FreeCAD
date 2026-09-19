// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *   Copyright (c) 2026 Sauli Kiviranta                                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "SemanticSourceCollector.h"
#include "SemanticHistoryAdapter.h"
#include "TopoShapeMapper.h"
#include <cstring>
#include <cstdlib>
#include <Base/Console.h>
#include <Standard_Failure.hxx>
#include <BRepBuilderAPI_Sewing.hxx>

#include <optional>
#include <vector>
#include <App/DocumentObject.h>
#include <App/SemanticReference.h>

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <gp_Pln.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <cmath>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

namespace Part
{

void collectUniqueSourceSeeds(App::SemanticGraph* graph,
                              App::DocumentObject* obj,
                              const TopoShape& sourceShape,
                              std::deque<TopoDS_Shape>& held,
                              std::vector<std::pair<App::SemanticId, const void*>>& inputs,
                              std::unordered_set<App::SemanticHandle>& seenSeeds)
{
    if (!graph || !obj || sourceShape.isNull()) {
        return;
    }
    const App::ObjectId fid = static_cast<App::ObjectId>(obj->getID());
    for (const App::SemanticBinding& b : graph->allBindings()) {
        if (b.feature != fid || !b.stid.valid()) {
            continue;
        }
        if (!isNamedIndex(b.index)
            || (b.index.type != "Face" && b.index.type != "Edge")) {
            continue;
        }
        if (!graph->allocator.isPublished(b.stid.handle)
            || graph->hasDeletedEvent(b.stid.handle)) {
            continue;
        }
        const char* typ = b.index.type.c_str();
        const std::optional<App::SemanticBinding> unique =
            App::uniqueBindingOnFeature(graph, b.stid, fid, typ);
        if (!unique.has_value()) {
            continue;
        }
        const TopAbs_ShapeEnum st =
            unique->index.type == "Face" ? TopAbs_FACE : TopAbs_EDGE;
        TopoDS_Shape s = sourceShape.findShape(st, unique->index.index);
        if (s.IsNull()) {
            continue;
        }
        // Keep one maker input per handle after validating the candidate; an
        // invalid early duplicate must not hide a later valid binding, and
        // duplicate groups make uniqueOneImageGenerated reject an otherwise
        // valid one-image result.
        if (!seenSeeds.insert(b.stid.handle).second) {
            continue;
        }
        held.push_back(s);
        inputs.push_back({b.stid, &held.back()});
    }
}

App::ElementIndex uniqueNamedIndexOnPublished(const TopoShape& published,
                                              const TopoDS_Shape& image)
{
    App::ElementIndex index;
    if (published.isNull() || image.IsNull()) {
        return index;
    }
    if (image.ShapeType() != TopAbs_FACE && image.ShapeType() != TopAbs_EDGE) {
        return index;
    }
    int number = published.findShape(image);
    if (number <= 0) {
        int matches = 0;
        int partner = 0;
        for (TopExp_Explorer ex(published.getShape(), image.ShapeType()); ex.More();
             ex.Next()) {
            if (ex.Current().IsPartner(image)) {
                ++matches;
                partner = published.findShape(ex.Current());
            }
        }
        number = matches == 1 ? partner : 0;
    }
    if (number <= 0) {
        return index;
    }
    index.type = image.ShapeType() == TopAbs_FACE ? "Face" : "Edge";
    index.index = number;
    return index;
}

int uniquePartnerIndex(const TopoShape& owner, const TopoDS_Shape& sub)
{
    if (owner.isNull() || sub.IsNull()) {
        return 0;
    }
    const TopAbs_ShapeEnum typ = sub.ShapeType();
    if (typ != TopAbs_EDGE && typ != TopAbs_FACE) {
        return 0;
    }
    std::vector<TopoDS_Shape> partners;
    for (TopExp_Explorer ex(owner.getShape(), typ); ex.More(); ex.Next()) {
        const TopoDS_Shape& s = ex.Current();
        if (!s.IsSame(sub) && !s.IsPartner(sub)) {
            continue;
        }
        bool dup = false;
        for (const auto& p : partners) {
            if (p.IsSame(s) || p.IsPartner(s)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            partners.push_back(s);
        }
    }
    if (partners.size() != 1) {
        return 0;
    }
    return owner.findShape(partners.front());
}

int uniqueCoplanarFaceIndex(const TopoShape& owner, const TopoDS_Shape& sub)
{
    if (owner.isNull() || sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
        return 0;
    }
    const BRepAdaptor_Surface src(TopoDS::Face(sub));
    if (src.GetType() != GeomAbs_Plane) {
        return 0;
    }
    const gp_Pln srcPln = src.Plane();
    std::vector<TopoDS_Shape> hits;
    for (TopExp_Explorer ex(owner.getShape(), TopAbs_FACE); ex.More(); ex.Next()) {
        const TopoDS_Shape& s = ex.Current();
        const BRepAdaptor_Surface dst(TopoDS::Face(s));
        if (dst.GetType() != GeomAbs_Plane) {
            continue;
        }
        if (!srcPln.Position().IsCoplanar(dst.Plane().Position(),
                                          Precision::Confusion(),
                                          Precision::Angular())) {
            continue;
        }
        bool dup = false;
        for (const auto& h : hits) {
            if (h.IsSame(s) || h.IsPartner(s)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            hits.push_back(s);
        }
    }
    if (hits.size() != 1) {
        return 0;
    }
    return owner.findShape(hits.front());
}

int uniqueGeometricEdgeIndex(const TopoShape& owner, const TopoDS_Shape& sub)
{
    if (owner.isNull() || sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
        return 0;
    }
    const TopoDS_Edge edge = TopoDS::Edge(sub);
    TopoDS_Vertex v1;
    TopoDS_Vertex v2;
    TopExp::Vertices(edge, v1, v2);
    if (v1.IsNull() || v2.IsNull()) {
        return 0;
    }
    const gp_Pnt p1 = BRep_Tool::Pnt(v1);
    const gp_Pnt p2 = BRep_Tool::Pnt(v2);
    std::vector<TopoDS_Shape> hits;
    for (TopExp_Explorer ex(owner.getShape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Edge e = TopoDS::Edge(ex.Current());
        TopoDS_Vertex a;
        TopoDS_Vertex b;
        TopExp::Vertices(e, a, b);
        if (a.IsNull() || b.IsNull()) {
            continue;
        }
        const gp_Pnt qa = BRep_Tool::Pnt(a);
        const gp_Pnt qb = BRep_Tool::Pnt(b);
        const bool same =
            ((p1.Distance(qa) <= Precision::Confusion()
              && p2.Distance(qb) <= Precision::Confusion())
             || (p1.Distance(qb) <= Precision::Confusion()
                 && p2.Distance(qa) <= Precision::Confusion()));
        if (!same) {
            continue;
        }
        bool dup = false;
        for (const auto& h : hits) {
            if (h.IsSame(e) || h.IsPartner(e)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            hits.push_back(e);
        }
    }
    if (hits.size() != 1) {
        return 0;
    }
    return owner.findShape(hits.front());
}

namespace {

bool sameEdgeEndpoints(const TopoDS_Edge& a, const TopoDS_Edge& b)
{
    TopoDS_Vertex a1, a2, b1, b2;
    TopExp::Vertices(a, a1, a2);
    TopExp::Vertices(b, b1, b2);
    if (a1.IsNull() || a2.IsNull() || b1.IsNull() || b2.IsNull()) {
        return false;
    }
    const gp_Pnt pa1 = BRep_Tool::Pnt(a1);
    const gp_Pnt pa2 = BRep_Tool::Pnt(a2);
    const gp_Pnt pb1 = BRep_Tool::Pnt(b1);
    const gp_Pnt pb2 = BRep_Tool::Pnt(b2);
    return (pa1.IsEqual(pb1, Precision::Confusion()) && pa2.IsEqual(pb2, Precision::Confusion()))
        || (pa1.IsEqual(pb2, Precision::Confusion()) && pa2.IsEqual(pb1, Precision::Confusion()));
}

bool sameSupportingCurve(const BRepAdaptor_Curve& src,
                         const BRepAdaptor_Curve& dst,
                         const TopoDS_Edge& srcEdge,
                         const TopoDS_Edge& dstEdge)
{
    if (src.GetType() != dst.GetType()) {
        return false;
    }
    if (src.GetType() == GeomAbs_Line) {
        const gp_Lin a = src.Line();
        const gp_Lin b = dst.Line();
        const gp_Dir da = a.Direction();
        const gp_Dir db = b.Direction();
        const bool dirOk = da.IsEqual(db, Precision::Angular())
            || da.IsOpposite(db, Precision::Angular());
        if (dirOk
            && (a.Location().IsEqual(b.Location(), Precision::Confusion())
                || a.Contains(b.Location(), Precision::Confusion()))) {
            return true;
        }
        return sameEdgeEndpoints(srcEdge, dstEdge);
    }
    if (src.GetType() == GeomAbs_Circle) {
        const gp_Circ a = src.Circle();
        const gp_Circ b = dst.Circle();
        if (std::abs(a.Radius() - b.Radius()) <= Precision::Confusion()
            && a.Axis().IsCoaxial(b.Axis(), Precision::Angular(), Precision::Confusion())) {
            return true;
        }
        return sameEdgeEndpoints(srcEdge, dstEdge);
    }
    return sameEdgeEndpoints(srcEdge, dstEdge);
}

}  // namespace

int uniqueSameCurveEdgeIndex(const TopoShape& owner, const TopoDS_Shape& sub)
{
    if (owner.isNull() || sub.IsNull() || sub.ShapeType() != TopAbs_EDGE) {
        return 0;
    }
    const TopoDS_Edge srcEdge = TopoDS::Edge(sub);
    const BRepAdaptor_Curve src(srcEdge);
    std::vector<TopoDS_Shape> hits;
    for (TopExp_Explorer ex(owner.getShape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Shape& s = ex.Current();
        const TopoDS_Edge dstEdge = TopoDS::Edge(s);
        const BRepAdaptor_Curve dst(dstEdge);
        if (!sameSupportingCurve(src, dst, srcEdge, dstEdge)) {
            continue;
        }
        bool dup = false;
        for (const auto& h : hits) {
            if (h.IsSame(s) || h.IsPartner(s)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            hits.push_back(s);
        }
    }
    if (hits.size() != 1) {
        return 0;
    }
    return owner.findShape(hits.front());
}

App::ElementIndex indexOnPublished(const TopoShape& owner, const TopoDS_Shape& sub)
{
    App::ElementIndex idx;
    if (sub.IsNull()) {
        return idx;
    }
    int n = owner.findShape(sub);
    if (n <= 0) {
        n = uniquePartnerIndex(owner, sub);
    }
    if (n <= 0 && sub.ShapeType() == TopAbs_FACE) {
        n = uniqueCoplanarFaceIndex(owner, sub);
    }
    if (n <= 0 && sub.ShapeType() == TopAbs_EDGE) {
        n = uniqueGeometricEdgeIndex(owner, sub);
    }
    if (n <= 0) {
        return idx;
    }
    switch (sub.ShapeType()) {
        case TopAbs_FACE:
            idx.type = "Face";
            break;
        case TopAbs_EDGE:
            idx.type = "Edge";
            break;
        default:
            return idx;
    }
    idx.index = n;
    return idx;
}

App::ElementIndex indexOnPublishedPartnerCoplanar(const TopoShape& owner,
                                                  const TopoDS_Shape& sub)
{
    App::ElementIndex idx;
    if (sub.IsNull()) {
        return idx;
    }
    int n = owner.findShape(sub);
    if (n <= 0) {
        // Capture is often pre-refine; publish uses the published solid.
        // Unique IsPartner after Refine is named; 0 or N stay unnamed.
        n = uniquePartnerIndex(owner, sub);
    }
    if (n <= 0 && sub.ShapeType() == TopAbs_FACE) {
        // Unique coplanar plane locates a captured cap after Refine.
        // Parallel non-coplanar faces stay distinct. 0 or N unnamed (I13).
        n = uniqueCoplanarFaceIndex(owner, sub);
    }
    if (n <= 0) {
        return idx;
    }
    switch (sub.ShapeType()) {
        case TopAbs_FACE:
            idx.type = "Face";
            break;
        case TopAbs_EDGE:
            idx.type = "Edge";
            break;
        case TopAbs_VERTEX:
            idx.type = "Vertex";
            break;
        default:
            return idx;
    }
    idx.index = n;
    return idx;
}

App::ElementIndex indexOnPublishedPartnerSameCurve(const TopoShape& owner,
                                                   const TopoDS_Shape& sub)
{
    App::ElementIndex idx;
    if (sub.IsNull()) {
        return idx;
    }
    int n = owner.findShape(sub);
    if (n <= 0) {
        n = uniquePartnerIndex(owner, sub);
    }
    if (n <= 0 && sub.ShapeType() == TopAbs_FACE) {
        n = uniqueCoplanarFaceIndex(owner, sub);
    }
    if (n <= 0 && sub.ShapeType() == TopAbs_EDGE) {
        n = uniqueSameCurveEdgeIndex(owner, sub);
    }
    if (n <= 0) {
        return idx;
    }
    switch (sub.ShapeType()) {
        case TopAbs_FACE:
            idx.type = "Face";
            break;
        case TopAbs_EDGE:
            idx.type = "Edge";
            break;
        default:
            return idx;
    }
    idx.index = n;
    return idx;
}


TopoDS_Shape uniquePartnerEdgeExcluding(const TopoDS_Shape& input,
                                        const TopoShape& result,
                                        const std::vector<TopoDS_Shape>& excludeStashed)
{
    TopoDS_Shape none;
    if (input.IsNull() || result.isNull()) {
        return none;
    }
    std::vector<TopoDS_Shape> partners;
    for (TopExp_Explorer ex(result.getShape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Shape& e = ex.Current();
        if (!e.IsSame(input) && !e.IsPartner(input)) {
            continue;
        }
        bool excluded = false;
        for (const auto& st : excludeStashed) {
            if (st.IsNull()) {
                continue;
            }
            if (e.IsSame(st) || e.IsPartner(st)) {
                excluded = true;
                break;
            }
        }
        if (excluded) {
            continue;
        }
        bool dup = false;
        for (const auto& p : partners) {
            if (p.IsSame(e) || p.IsPartner(e)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            partners.push_back(e);
        }
    }
    if (partners.size() == 1) {
        return partners.front();
    }
    return none;
}


bool sameOccShape(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    return !a.IsNull() && !b.IsNull() && a.IsSame(b);
}

int countGeneratedOf(BRepBuilderAPI_MakeShape* maker,
                     const TopoDS_Shape& input,
                     TopAbs_ShapeEnum t)
{
    int n = 0;
    if (!maker || input.IsNull()) {
        return 0;
    }
    for (TopTools_ListIteratorOfListOfShape it(maker->Generated(input)); it.More();
         it.Next()) {
        if (it.Value().ShapeType() == t) {
            ++n;
        }
    }
    return n;
}

std::size_t namedIndexCount(const std::vector<App::ElementIndex>& xs)
{
    std::size_t n = 0;
    for (const auto& x : xs) {
        if (isNamedIndex(x)) {
            ++n;
        }
    }
    return n;
}

std::vector<TopoDS_Shape> uniqueGeneratedThenModifiedEdgeImages(
    BRepBuilderAPI_MakeShape* maker,
    const TopoDS_Shape& input)
{
    std::vector<TopoDS_Shape> images;
    if (!maker || input.IsNull()) {
        return images;
    }
    for (TopTools_ListIteratorOfListOfShape it(maker->Generated(input)); it.More();
         it.Next()) {
        if (it.Value().ShapeType() == TopAbs_EDGE) {
            images.push_back(it.Value());
        }
    }
    if (images.empty()) {
        for (TopTools_ListIteratorOfListOfShape it(maker->Modified(input)); it.More();
             it.Next()) {
            if (it.Value().ShapeType() == TopAbs_EDGE) {
                images.push_back(it.Value());
            }
        }
    }
    return images;
}

std::vector<TopoDS_Shape> uniqueModifiedThenGeneratedEdgeImages(
    BRepBuilderAPI_MakeShape* maker,
    const TopoDS_Shape& input,
    const TopoShape& result)
{
    std::vector<TopoDS_Shape> images;
    if (!maker || input.IsNull()) {
        return images;
    }
    for (TopTools_ListIteratorOfListOfShape it(maker->Modified(input)); it.More(); it.Next()) {
        if (it.Value().ShapeType() == TopAbs_EDGE) {
            images.push_back(it.Value());
        }
    }
    if (images.empty()) {
        for (TopTools_ListIteratorOfListOfShape it(maker->Generated(input)); it.More();
             it.Next()) {
            if (it.Value().ShapeType() == TopAbs_EDGE) {
                images.push_back(it.Value());
            }
        }
    }
    if (images.empty() && result.findShape(input) > 0) {
        images.push_back(input);
    }
    // :U copies often fail IsSame/findShape (different location) but are
    // IsPartner. Dedup orientations. 1 unique geometry → name it; N → unnamed.
    if (images.empty() && !result.isNull()) {
        std::vector<TopoDS_Shape> partners;
        for (TopExp_Explorer ex(result.getShape(), TopAbs_EDGE); ex.More(); ex.Next()) {
            const TopoDS_Shape& e = ex.Current();
            if (!e.IsSame(input) && !e.IsPartner(input)) {
                continue;
            }
            bool dup = false;
            for (const auto& p : partners) {
                if (p.IsSame(e) || p.IsPartner(e)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                partners.push_back(e);
            }
        }
        if (partners.size() == 1) {
            images.push_back(partners.front());
        }
    }
    return images;
}

TopoDS_Shape uniqueGeneratedFace(BRepBuilderAPI_MakeShape* maker, const TopoDS_Shape& input)
{
    TopoDS_Shape unique;
    if (!maker || input.IsNull()) {
        return unique;
    }
    int n = 0;
    for (TopTools_ListIteratorOfListOfShape it(maker->Generated(input)); it.More();
         it.Next()) {
        if (it.Value().ShapeType() == TopAbs_FACE) {
            ++n;
            unique = it.Value();
        }
    }
    if (n != 1) {
        return TopoDS_Shape();
    }
    return unique;
}

App::ElementIndex uniqueZParallelEdgeOnPublishedFace(const TopoShape& owner,
                                                     const App::ElementIndex& faceIdx)
{
    if (owner.isNull() || !isNamedIndex(faceIdx) || faceIdx.type != "Face") {
        return {};
    }
    const TopoDS_Shape faceSh = owner.getSubShape(TopAbs_FACE, faceIdx.index, true);
    if (faceSh.IsNull() || faceSh.ShapeType() != TopAbs_FACE) {
        return {};
    }
    const gp_Dir zDir(0.0, 0.0, 1.0);
    std::vector<TopoDS_Shape> hits;
    for (TopExp_Explorer ex(faceSh, TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Shape& s = ex.Current();
        if (s.IsNull() || s.ShapeType() != TopAbs_EDGE) {
            continue;
        }
        const BRepAdaptor_Curve c(TopoDS::Edge(s));
        if (c.GetType() != GeomAbs_Line) {
            continue;
        }
        if (!c.Line().Direction().IsParallel(zDir, Precision::Angular())) {
            continue;
        }
        bool dup = false;
        for (const auto& h : hits) {
            if (h.IsSame(s) || h.IsPartner(s)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            hits.push_back(s);
        }
    }
    if (hits.size() != 1) {
        return {};
    }
    return indexOnPublishedPartnerSameCurve(owner, hits.front());
}

/// Map a Pipe/Loft/Helix pre-sew shell TShape onto the published sew+solid Shape.
/// indexOnPublishedPartnerSameCurve first; MapperSewing sew hop; unique shared-vertex
/// geometry hop; element-map if still unnamed. 0 or N stay unnamed (I13).
/// Formerly Pipe::pipeIndexOnPublished / Loft::loftIndexOnPublished /
/// Helix::helixIndexOnPublished (Pass 139). Does not mint.
App::ElementIndex indexOnPublishedPartnerSameCurveSewSharedVertex(
    const TopoShape& published,
    const TopoDS_Shape& sub,
    const TopoShape& preSew,
    BRepBuilderAPI_Sewing* sewer)
{
    App::ElementIndex idx;
    if (sub.IsNull()) {
        return idx;
    }
    idx = Part::indexOnPublishedPartnerSameCurve(published, sub);
    if (isNamedIndex(idx)) {
        return idx;
    }

    auto uniqueShared = [&](const TopoDS_Shape& cand) -> App::ElementIndex {
        App::ElementIndex hit;
        if (cand.IsNull()) {
            return hit;
        }
        hit = Part::indexOnPublishedPartnerSameCurve(published, cand);
        if (isNamedIndex(hit)) {
            return hit;
        }
        try {
            const std::vector<TopoShape> found =
                published.findSubShapesWithSharedVertex(TopoShape(cand));
            if (found.size() != 1) {
                return App::ElementIndex();
            }
            return Part::indexOnPublishedPartnerSameCurve(published, found.front().getShape());
        }
        catch (const Standard_Failure&) {
            return App::ElementIndex();
        }
    };

    auto uniqueMapped = [&](const TopoShape& source,
                            const TopoDS_Shape& srcSub) -> App::ElementIndex {
        App::ElementIndex hit;
        if (source.isNull() || srcSub.IsNull() || published.isNull()) {
            return hit;
        }
        const int n = source.findShape(srcSub);
        if (n <= 0) {
            return hit;
        }
        const char* type = nullptr;
        if (srcSub.ShapeType() == TopAbs_FACE) {
            type = "Face";
        }
        else if (srcSub.ShapeType() == TopAbs_EDGE) {
            type = "Edge";
        }
        else {
            return hit;
        }
        const Data::MappedName mapped =
            source.getMappedName(Data::IndexedName::fromConst(type, n));
        if (!mapped) {
            return hit;
        }
        const Data::MappedElement me = published.getElementName(mapped.toString().c_str());
        if (me.index.getIndex() > 0 && me.index.getType()
            && std::strcmp(me.index.getType(), type) == 0) {
            hit.type = type;
            hit.index = me.index.getIndex();
            return hit;
        }
        const std::string mappedStr = mapped.toString();
        std::vector<int> hits;
        const unsigned long count = published.countSubShapes(srcSub.ShapeType());
        for (unsigned long i = 1; i <= count; ++i) {
            const Data::MappedName p = published.getMappedName(
                Data::IndexedName::fromConst(type, static_cast<int>(i)));
            if (!p) {
                continue;
            }
            const std::string ps = p.toString();
            if (ps == mappedStr || ps.find(mappedStr) != std::string::npos) {
                hits.push_back(static_cast<int>(i));
            }
        }
        if (hits.size() != 1) {
            return App::ElementIndex();
        }
        hit.type = type;
        hit.index = hits.front();
        return hit;
    };

    if (sewer) {
        const std::vector<TopoDS_Shape> images = Part::MapperSewing(*sewer).modified(sub);
        if (images.size() == 1) {
            idx = uniqueShared(images.front());
            if (isNamedIndex(idx)) {
                return idx;
            }
        }
    }

    if (!preSew.isNull()) {
        const App::ElementIndex shellHit = Part::indexOnPublishedPartnerSameCurve(preSew, sub);
        if (isNamedIndex(shellHit)) {
            TopAbs_ShapeEnum ty = TopAbs_SHAPE;
            if (shellHit.type == "Face") {
                ty = TopAbs_FACE;
            }
            else if (shellHit.type == "Edge") {
                ty = TopAbs_EDGE;
            }
            if (ty != TopAbs_SHAPE) {
                const TopoDS_Shape shellSub = preSew.getSubShape(ty, shellHit.index, true);
                idx = uniqueShared(shellSub);
                if (isNamedIndex(idx)) {
                    return idx;
                }
                if (sewer) {
                    const std::vector<TopoDS_Shape> images =
                        Part::MapperSewing(*sewer).modified(shellSub);
                    if (images.size() == 1) {
                        idx = uniqueShared(images.front());
                        if (isNamedIndex(idx)) {
                            return idx;
                        }
                    }
                }
                idx = uniqueMapped(preSew, shellSub);
                if (isNamedIndex(idx)) {
                    return idx;
                }
            }
        }
        idx = uniqueMapped(preSew, sub);
        if (isNamedIndex(idx)) {
            return idx;
        }
    }

    idx = uniqueShared(sub);
    return idx;
}

std::size_t mergeUniqueZParallelEdgesOntoNamed(
    const TopoShape& published,
    const std::vector<App::ElementIndex>& namedFaces,
    std::vector<App::ElementIndex>& namedEdges)
{
    std::vector<App::ElementIndex> zEdges;
    zEdges.reserve(namedFaces.size());
    std::size_t nZ = 0;
    for (const auto& faceIdx : namedFaces) {
        zEdges.push_back(uniqueZParallelEdgeOnPublishedFace(published, faceIdx));
        if (isNamedIndex(zEdges.back()) && zEdges.back().type == "Edge") {
            ++nZ;
        }
    }
    if (namedEdges.empty()) {
        namedEdges = zEdges;
    }
    else {
        for (std::size_t i = 0; i < zEdges.size(); ++i) {
            if (!isNamedIndex(zEdges[i]) || zEdges[i].type != "Edge") {
                continue;
            }
            bool have = false;
            for (const auto& e : namedEdges) {
                if (e.type == zEdges[i].type && e.index == zEdges[i].index) {
                    have = true;
                    break;
                }
            }
            if (have) {
                continue;
            }
            if (i < namedEdges.size()
                && (!isNamedIndex(namedEdges[i])
                    || namedEdges[i].type != "Edge")) {
                namedEdges[i] = zEdges[i];
            }
            else {
                namedEdges.push_back(zEdges[i]);
            }
        }
    }
    return nZ;
}

std::size_t appendUniqueZParallelFaceRailEdges(
    const TopoShape& shell,
    const std::vector<SemanticSeededShape>& faces,
    std::vector<SemanticSeededShape>& edges)
{
    std::size_t appended = 0;
    for (const auto& p : faces) {
        if (p.shape.IsNull() || p.shape.ShapeType() != TopAbs_FACE) {
            continue;
        }
        const App::ElementIndex faceIdx =
            indexOnPublishedPartnerSameCurve(shell, p.shape);
        const App::ElementIndex zIdx =
            uniqueZParallelEdgeOnPublishedFace(shell, faceIdx);
        if (!isNamedIndex(zIdx) || zIdx.type != "Edge") {
            continue;
        }
        const TopoDS_Shape zEdge =
            shell.getSubShape(TopAbs_EDGE, zIdx.index, true);
        if (zEdge.IsNull()) {
            continue;
        }
        bool have = false;
        for (const auto& existing : edges) {
            if (sameOccShape(existing.shape, zEdge)) {
                have = true;
                break;
            }
        }
        if (have) {
            continue;
        }
        edges.push_back({p.fromSeed, zEdge});
        ++appended;
    }
    return appended;
}

std::size_t appendFromMakerGeneratedWhenFacesEmpty(
    const void* occMaker,
    const TopoShape& preSewShell,
    const std::vector<App::SemanticId>& curveSeeds,
    const std::vector<TopoShape>& profileEdges,
    const std::vector<App::SemanticId>& vertexSeeds,
    const std::vector<TopoShape>& profileVertices,
    std::vector<SemanticSeededShape>& faces,
    std::vector<SemanticSeededShape>& outEdges,
    bool* usedFromMaker)
{
    if (!faces.empty()) {
        return 0;
    }
    if (usedFromMaker) {
        *usedFromMaker = true;
    }
    if (!occMaker || preSewShell.isNull()) {
        return 0;
    }

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    for (std::size_t i = 0; i < curveSeeds.size() && i < profileEdges.size(); ++i) {
        held.push_back(profileEdges[i].getShape());
        inputs.push_back({curveSeeds[i], &held.back()});
    }
    if (!vertexSeeds.empty()) {
        for (std::size_t i = 0; i < vertexSeeds.size() && i < profileVertices.size(); ++i) {
            held.push_back(profileVertices[i].getShape());
            inputs.push_back({vertexSeeds[i], &held.back()});
        }
    }

    auto indexOf = [&preSewShell](const void* occ) -> App::ElementIndex {
        if (!occ) {
            return App::ElementIndex();
        }
        return indexOnPublishedPartnerSameCurve(
            preSewShell, *static_cast<const TopoDS_Shape*>(occ));
    };

    const HistoryTable fromHist =
        SemanticHistoryAdapter::fromMaker(occMaker, inputs, indexOf);

    auto alreadyHasEdge = [&outEdges](const TopoDS_Shape& s) -> bool {
        for (const auto& existing : outEdges) {
            if (sameOccShape(existing.shape, s)) {
                return true;
            }
        }
        return false;
    };

    for (const auto& rec : fromHist) {
        if (rec.kind != App::EventKind::Generated || !isNamedIndex(rec.toIndex)) {
            continue;
        }
        if (rec.toIndex.type == "Face") {
            TopoDS_Shape s = preSewShell.findShape(TopAbs_FACE, rec.toIndex.index);
            if (!s.IsNull()) {
                faces.push_back({rec.fromSeed, s});
            }
        }
        else if (rec.toIndex.type == "Edge") {
            TopoDS_Shape s = preSewShell.findShape(TopAbs_EDGE, rec.toIndex.index);
            if (!s.IsNull() && !alreadyHasEdge(s)) {
                outEdges.push_back({rec.fromSeed, s});
            }
        }
    }
    return fromHist.size();
}



void refreshNamedIndicesFromSeededShapes(
    const TopoShape& published,
    const TopoShape& preSewShell,
    BRepBuilderAPI_Sewing* sewer,
    const std::vector<SemanticSeededShape>& faces,
    const std::vector<SemanticSeededShape>& edges,
    std::vector<App::ElementIndex>& namedFaces,
    std::vector<App::ElementIndex>& namedEdges)
{
    namedFaces.clear();
    namedFaces.reserve(faces.size());
    for (const auto& p : faces) {
        namedFaces.push_back(
            indexOnPublishedPartnerSameCurveSewSharedVertex(
                published, p.shape, preSewShell, sewer));
    }
    namedEdges.clear();
    namedEdges.reserve(edges.size());
    for (const auto& p : edges) {
        namedEdges.push_back(
            indexOnPublishedPartnerSameCurveSewSharedVertex(
                published, p.shape, preSewShell, sewer));
    }
}


// A4: gated TESTS *Diag — default off; exact historical Console strings when on.
bool testsPublishDiagEnabled()
{
    auto envTruthy = [](const char* env) {
        return env && env[0] != '\0' && !(env[0] == '0' && env[1] == '\0');
    };
    // Prefer FREECAD_TESTS_DIAG; fall back to legacy FREECAD_TEST5_DIAG for one tip.
    if (envTruthy(std::getenv("FREECAD_TESTS_DIAG"))
        || envTruthy(std::getenv("FREECAD_TEST5_DIAG"))) {
        return true;
    }
    // Do not create the tag: DEFAULT (-1) would inherit MSG on release and
    // keep console noise. FreeCAD.setLogLevel("PartTestsDiag", ...) creates it.
    // Legacy PartTest5Diag accepted for one tip.
    const int* lvl = Base::Console().getLogLevel("PartTestsDiag", /*create=*/false);
    if (!lvl) {
        lvl = Base::Console().getLogLevel("PartTest5Diag", /*create=*/false);
    }
    if (!lvl) {
        return false;
    }
    return Base::Console().logLevel(*lvl) >= FC_LOGLEVEL_MSG;
}

void testsPublishDiag(const char* message)
{
    if (!message || !testsPublishDiagEnabled()) {
        return;
    }
    Base::Console().message("%s", message);
}

void testsPublishDiagSkip(const char* diagTag, const char* reason)
{
    if (!diagTag || !reason || !testsPublishDiagEnabled()) {
        return;
    }
    Base::Console().message("TESTS %s skip emit: %s\n", diagTag, reason);
}

void testsPublishDiagBound(const char* diagTag, std::size_t bound, std::size_t named)
{
    if (!diagTag || !testsPublishDiagEnabled()) {
        return;
    }
    Base::Console().message("TESTS %s bound=%zu named=%zu unnamed=0\n",
                            diagTag, bound, named);
}

}  // namespace Part
