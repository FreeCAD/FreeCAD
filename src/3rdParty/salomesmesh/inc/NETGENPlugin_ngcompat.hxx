// SPDX-License-Identifier: LGPL-2.1-or-later
//
// NETGENPlugin_ngcompat.hxx
//
// Compatibility layer between the SALOME NETGENPlugin and the two netgen
// Segment APIs:
//
//   <= 6.2.2601  public Segment fields: si, edgenr, epgeominfo[i].{dist,u,v,edgenr}
//   >= 6.2.2608  private fields + accessors; the face (si) and the geometric
//                edge (epgeominfo[].edgenr) live in an EdgeRegion that the
//                segment points to via Segment::SetIndex(EdgeRegionIndex)
//
// 6.2.2602 .. 6.2.2607 carry a transitional API (private Segment, but no
// EdgeRegion yet) and are rejected.
//
// Include AFTER the netgen headers (meshing.hpp / occgeom.hpp).
// NETGEN_VERSION is FreeCAD's (major << 16) + (minor << 8) + patch.

#ifndef NETGENPLUGIN_NGCOMPAT_HXX
#define NETGENPLUGIN_NGCOMPAT_HXX

#define NG_VER(a, b, c) (((a) << 16) + ((b) << 8) + (c))

#if NETGEN_VERSION > NG_VER(6, 2, 2601) && NETGEN_VERSION < NG_VER(6, 2, 2608)
#  error "netgen 6.2.2602-6.2.2607 has a transitional Segment API; build against <= 6.2.2601 or >= 6.2.2608"
#endif

#define NG_HAS_REGIONS (NETGEN_VERSION >= NG_VER(6, 2, 2608))

namespace ngcompat
{
using netgen::Mesh;
using netgen::Segment;

// ---- typed indices ---------------------------------------------------------
// The plugin keeps netgen ids as 1-based ints (points, segments, surface and
// volume elements, face descriptors). >= 2608 makes every index a distinct
// type with no implicit int conversion; these convert explicitly, always via
// the 1-based numbering, so the result does not depend on PointIndex::BASE.
// Nr() returns 0 for an invalid index, as the old raw value did.
#if NG_HAS_REGIONS
using Point3 = netgen::Point<3>;  // netgen::Point3d was removed
inline netgen::PointIndex          PI  (int i) { return netgen::PointIndex::FromNr1(i); }
inline netgen::SegmentIndex        SegI(int i) { return netgen::SegmentIndex::FromNr1(i); }
inline netgen::SurfaceElementIndex SEI (int i) { return netgen::SurfaceElementIndex::FromNr1(i); }
inline netgen::ElementIndex        EI  (int i) { return netgen::ElementIndex::FromNr1(i); }
inline netgen::FaceRegionIndex     FRI (int i) { return netgen::FaceRegionIndex::FromNr1(i); }
template <class T> inline int Nr(const T& x) { return x.IsValid() ? int(x.Nr1()) : 0; }

// SurfaceElement() returns a by-reference view (Element2dRef); take a real copy
// before adding elements, since AddSurfaceElement() may reallocate the storage
inline netgen::Element2d CopyEl2d(const netgen::Element2dRef& r)
{
    netgen::Element2d e(r.GetNP());
    for (int j = 1; j <= r.GetNP(); ++j) {
        e.PNum(j)       = r.PNum(j);
        e.GeomInfoPi(j) = r.GeomInfoPi(j);
    }
    e.SetIndex(r.GetIndex());
    return e;
}
#else
using Point3 = netgen::Point3d;
inline netgen::PointIndex PI  (int i) { return netgen::PointIndex(i); }
inline int                SegI(int i) { return i; }
inline int                SEI (int i) { return i; }
inline int                EI  (int i) { return i; }
inline int                FRI (int i) { return i; }
template <class T> inline int Nr(const T& x) { return int(x); }
inline netgen::Element2d CopyEl2d(const netgen::Element2d& e) { return e; }
#endif

#if NG_HAS_REGIONS

// ---- per-end geometry info -------------------------------------------------
inline netgen::EdgePointGeomInfo&       EPGI(Segment& s, int i)       { return s.EPGeomInfo(i); }
inline const netgen::EdgePointGeomInfo& EPGI(const Segment& s, int i) { return s.EPGeomInfo(i); }
inline double& Dist(Segment& s, int i)       { return s.EPGeomInfo(i).dist; }
inline double  Dist(const Segment& s, int i) { return s.EPGeomInfo(i).dist; }
inline double& U(Segment& s, int i)          { return s.GeomInfo(i).u; }
inline double  U(const Segment& s, int i)    { return s.GeomInfo(i).u; }
inline double& V(Segment& s, int i)          { return s.GeomInfo(i).v; }
inline double  V(const Segment& s, int i)    { return s.GeomInfo(i).v; }

// ---- face / geometric edge, now held by the EdgeRegion ---------------------
// Both ids are 1-based (fmap/emap FindIndex), 0 = none, as in the old fields.
inline int FaceOf(const Mesh& m, const Segment& s)
{
    if (!m.HasEdgeDescriptor(s)) return 0;
    int f = m.GetEdgeDescriptor(s).SurfNr(0);
    return f > 0 ? f : 0;
}

// `end` is ignored on >= 2608: a region holds one edge, the one set for end 0
inline int GeomEdgeOf(const Mesh& m, const Segment& s, int /*end*/ = 0)
{
    if (!m.HasEdgeDescriptor(s)) return 0;
    int e = m.GetEdgeDescriptor(s).EdgeNr();
    return e > 0 ? e : 0;
}

// Find or create the region for (faceID, edgeID). One region per distinct
// pair keeps the old per-segment meaning of (si, epgeominfo[].edgenr).
inline netgen::EdgeRegionIndex FindOrAddRegion(Mesh& m, int faceID, int edgeID)
{
    const int snr  = faceID > 0 ? faceID : -1;  // netgen's "unset" is -1
    const int ednr = edgeID > 0 ? edgeID : -1;

    for (auto i : m.EdgeDescriptors().Range()) {
        const auto& ed = m.GetEdgeDescriptor(i);
        if (ed.EdgeNr() == ednr && ed.SurfNr(0) == snr && ed.SurfNr(1) == -1)
            return i;
    }

    netgen::EdgeRegion ed(ednr, snr, -1);
    // same rule as Mesh::RebuildFDIndices()'s OCC fallback: match SurfNr only
    if (snr > 0) {
        for (auto k : m.FaceDescriptors().Range()) {
            if (m.GetFaceDescriptor(k).SurfNr() == snr) {
                ed.SetIndex(k);
                break;
            }
        }
    }
    return m.AddEdgeDescriptor(ed);
}

inline void SetFaceEdge(Mesh& m, Segment& s, int faceID, int edgeID)
{
    s.SetIndex(FindOrAddRegion(m, faceID, edgeID));
}
// replaces  seg.si = f;                 (keeps the segment's geometric edge)
inline void SetFace(Mesh& m, Segment& s, int faceID) { SetFaceEdge(m, s, faceID, GeomEdgeOf(m, s)); }
// replaces  seg.epgeominfo[end].edgenr = e;   (keeps the segment's face)
// Only end 0 decides the region's edge: when a segment ends on a vertex the
// plugin already gives end 1 the *next* edge's id, and end 0 is the one the
// plugin's readers and netgen's own OCC code use.
inline void SetEdge(Mesh& m, Segment& s, int end, int edgeID)
{
    if (end == 0) SetFaceEdge(m, s, FaceOf(m, s), edgeID);
}

// The plugin stored a running segment id in Segment::edgenr. That field no
// longer exists and nothing in netgen >= 2608 reads it.
inline void SetSegId(Segment&, int) {}
inline int  SegId(const Segment&) { return -1; }

#else  // netgen <= 6.2.2601: the original public fields

inline netgen::EdgePointGeomInfo&       EPGI(Segment& s, int i)       { return s.epgeominfo[i]; }
inline const netgen::EdgePointGeomInfo& EPGI(const Segment& s, int i) { return s.epgeominfo[i]; }
inline double& Dist(Segment& s, int i)       { return s.epgeominfo[i].dist; }
inline double  Dist(const Segment& s, int i) { return s.epgeominfo[i].dist; }
inline double& U(Segment& s, int i)          { return s.epgeominfo[i].u; }
inline double  U(const Segment& s, int i)    { return s.epgeominfo[i].u; }
inline double& V(Segment& s, int i)          { return s.epgeominfo[i].v; }
inline double  V(const Segment& s, int i)    { return s.epgeominfo[i].v; }

inline int  FaceOf(const Mesh&, const Segment& s)     { return s.si; }
inline int  GeomEdgeOf(const Mesh&, const Segment& s, int end = 0) { return s.epgeominfo[end].edgenr; }
inline void SetFace(Mesh&, Segment& s, int faceID)                { s.si = faceID; }
inline void SetEdge(Mesh&, Segment& s, int end, int edgeID)       { s.epgeominfo[end].edgenr = edgeID; }
inline void SetSegId(Segment& s, int id) { s.edgenr = id; }
inline int  SegId(const Segment& s)      { return s.edgenr; }

#endif

}  // namespace ngcompat

#endif  // NETGENPLUGIN_NGCOMPAT_HXX
