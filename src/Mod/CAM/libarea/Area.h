// SPDX-License-Identifier: BSD-3-Clause

// Area.h
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// repository now moved to github

#pragma once

#include <map>
#include <optional>
#include <tuple>
#include <vector>
#include "Curve.h"
#include "clipper2/clipper.h"

namespace heeks
{

enum PocketMode
{
    SpiralPocketMode,
    ZigZagPocketMode,
    SingleOffsetPocketMode,
    ZigZagThenSingleOffsetPocketMode,
};

struct CAreaPocketParams
{
    double tool_radius;
    double extra_offset;
    double stepover;
    bool from_center;
    PocketMode mode;
    double zig_angle;
    bool only_cut_first_offset;
    CAreaPocketParams(
        double Tool_radius,
        double Extra_offset,
        double Stepover,
        bool From_center,
        PocketMode Mode,
        double Zig_angle
    )
    {
        tool_radius = Tool_radius;
        extra_offset = Extra_offset;
        stepover = Stepover;
        from_center = From_center;
        mode = Mode;
        zig_angle = Zig_angle;
        only_cut_first_offset = false;
    }
};

struct SegmentData
{
    CVertex orig;
    int edgeTag;  // 1 = offset kept (positive side), -1 = offset discarded (negative side), 0 = end cap
    int curveIndex;
    int vertexIndex;
};


// Metadata structure to facilitate correct conversion from clipper lines back to CCurves
//
// Note on purpose, and preserving correct behavior when populating the edgeData map:
//
// When populating the edgeData map, it is possible to have key duplication (two segments with
// matching x/y clipper endpoints), resulting in overwriting saved metadata. This is not desirable,
// but it is acceptable. Here are the consequences of such overwrites, broken down by SegmentData
// struct member
//
// SegmentData:
// - CVertex (original arc/line type, and arc center location)
// - tag indicating offset direction (if offsetting, otherwise unused/set to 1)
// - original curve/vertex index
//
// CVertex: The purpose of the CVertex metadata is to know after clipper geometry processing which
// clipper lines are actually meant to represent arcs, and what those arcs' centers are. If clipper
// inputs are sufficiently similar that this system gets confused, it's acceptable (if still wrong)
// to output the wrong geometry element because it's within clipper tolerances of the right element,
// and we are (necessarily) tolerant of clipper-scale imprecision in CAM geometry processing. This
// makes it ok to make an error in mapping back from clipper line segments to the original geometry
// type in these cases.
//
// tag, on closed path offsets: It is not possible for segments with duplicate x/y endpoints and
// different tags to exist in the output, because +1 tags are on the exterior of a closed region and
// -1 tags are the interior (i.e. a hole). A duplicated line segment that is both interior and
// exterior will be eliminated by the union operation after "naive" segment offsetting/joining. The
// SegmentData for these segments does not matter.
//
// tag, on open path offsets: This is more nuanced, because open paths can cross themselves or do a
// near pass when spiraling around, potentially creating overlapping offsets with different tags.
// The main saving grace here is that when this happens, the output open offset tag is poorly
// defined. If the same geometry is generated twice, (i.e.) from expansion of an endpoint and also
// the right side of the of the input path, does it or does it not deserve to be emitted as a
// right-side offset segment? Thankfully these poorly defined cases don't matter much to us in CAM,
// because we ultimately try to fit a tool on these paths. When clearance space for the tool drops
// towards ~0mm, it becomes quite ambiguous if we want to emit that section of the offset path or
// not. Operations that care to resolve this gracefully can require/compute a specific amount of
// clearance, eliminating this ambiguous ~0mm clearance region. Currently we don't do this in any
// operation (in fact, we hardly use open path offsets at all).
//
// original index in input geometry: This is used exclusively for ordering/sorting output open
// paths, such that clipping an open path produces output that is a subset of the original open
// path, in the original input order. This is a clearly desirable feature and will generally produce
// better-routed tool paths (supposing we follow the output path directly, and don't discard path
// order and replace it with output from a separate routing algorithm), but it's not critical to our
// application. Notably, though, since this information is only used in open path clipping,
// duplicate x/y endpoints would mean that the open path doubles back on itself exactly (or at least
// within clipper tolerance). This is not a standard/sensible CAM use case, and I don't think it is
// critical to ensure that open path clipping preserves order in this case. If a (new?) operation
// feels differently, it is possible to invoke open path clipping on successive non-overlapping
// subsets of the input path (i.e. trivially one edge at a time, or with a more complex algorithm to
// process more edges at once). Or perhaps if we find ourselves wanting to do that, then the feature
// can be built in to AreaClipper as an automatic feature at that time.
struct ConversionMetadata
{
    // New points may be created by clipper at the intersection of segments. This multimap tracks
    // the origin of such points; new points map to the segments that generated them. Edges are
    // specified by their end points, and all points are specified by their z values. This data
    // structure is used to determine the parent edge of edges connecting to new points.
    //
    // Format: newZ -> (e1ZMin, e1ZMax, e2ZMin, e2ZMax)
    std::multimap<int64_t, std::tuple<int64_t, int64_t, int64_t, int64_t>> intersections;

    // Maps (zMin, zMax) edge pairs to their original CVertex, edge tag, and ordering. This
    // information is used in SetFromResult to convert from clipper back to CCurves, and to restore
    // the order and orientation of open curves.
    std::map<std::pair<int64_t, int64_t>, SegmentData> edgeData;

    // Deduplication cache: maps (x,y) in Clipper coordinates to the z-label already assigned there
    std::map<std::pair<int64_t, int64_t>, int64_t> xy_to_z;

    // Track the next z-value available for allocation
    int64_t z_next;

    // Track the next curve index for MakePoly calls
    int nextCurveIndex;

    ConversionMetadata()
        : z_next(1)
        , nextCurveIndex(0)
    {}
};

class CArea
{
public:
    std::list<CCurve> m_curves;

    static double m_accuracy;
    static bool m_fit_arcs;
    static double m_processing_done;  // 0.0 to 100.0, set inside MakeOnePocketCurve
    static double m_single_area_processing_length;
    static double m_after_MakeOffsets_length;
    static double m_MakeOffsets_increment;
    static double m_split_processing_length;
    static bool m_set_processing_length_in_split;
    static bool m_please_abort;  // the user sets this from another thread, to tell
                                 // MakeOnePocketCurve to finish with no result.
    static double m_clipper_scale;

    void append(const CCurve& curve);
    void move(CCurve&& curve);
    void Subtract(const CArea& a2);
    void Intersect(const CArea& a2);
    void Union(const CArea& a2);
    void Xor(const CArea& a2);

    // Offsets wires; input wires must be closed shapes
    void Offset(double offset);

    // Offsets wires; open wires allowed. Positive offset is kept in this CArea, and
    // negative offset is returned in a new CArea. Endcaps (round) are filtered out
    CArea OpenOffset(double offset);

    void ClipperNoop();  // converts to clipper and back (i.e. arc fitting) without any clipping
                         // operations
    void Thicken(double value);
    unsigned int num_curves()
    {
        return static_cast<int>(m_curves.size());
    }
    Point NearestPoint(const Point& p) const;
    void GetBox(CBox2D& box);
    void Reorder();
    void MakePocketToolpath(std::list<CCurve>& toolpath, const CAreaPocketParams& params) const;
    void SplitAndMakePocketToolpath(std::list<CCurve>& toolpath, const CAreaPocketParams& params) const;
    void MakeOnePocketCurve(std::list<CCurve>& curve_list, const CAreaPocketParams& params) const;
    static bool HolesLinked();
    void Split(std::list<CArea>& m_areas) const;
    double GetArea(bool always_add = false) const;

    // Test helper method for checking that if clipper reverses/reorders open paths,
    // it is handled properly.
    //
    // This CArea should hold an open path, and the provided clip_area should have a closed
    // path. This method will behave the same as Intersect, but before converting clipper
    // results back to CArea it will optionally reverse the open paths.
    //
    // For testing only — not production API.
    // reverseOpenPathContents: if true, reverse the contents (vertices) of each open path
    // reverseOpenPathOrder: if true, reverse the order of the open paths themselves
    void Debug_IntersectOpenPathReversal(
        const CArea& clip_area,
        bool reverseOpenPathContents,
        bool reverseOpenPathOrder
    );

    // Avoid outside direct accessing static member variable because of Windows DLL issue
#define CAREA_PARAM_DECLARE(_type, _name) \
    static _type get_##_name(); \
    static void set_##_name(_type _name);

    CAREA_PARAM_DECLARE(double, tolerance)
    CAREA_PARAM_DECLARE(bool, fit_arcs)
    CAREA_PARAM_DECLARE(double, accuracy)
    CAREA_PARAM_DECLARE(double, clipper_scale)

    void PopulateClipper(Clipper2Lib::Clipper64& c, bool as_clip, ConversionMetadata& metadata) const;

    void Clip(
        Clipper2Lib::ClipType op,
        const CArea& clip_area,
        Clipper2Lib::FillRule fillType = Clipper2Lib::FillRule::EvenOdd
    );

    // Reorders open paths so they match the original input order and direction.
    // Must be run after Clipper operations, before converting back to arcs.
    void ReorderOpenPaths(Clipper2Lib::Paths64& paths, const ConversionMetadata& metadata);

private:
    // Returns (minZ, maxZ) of the vertices of the parent edge
    static std::pair<int64_t, int64_t> getParentEdge(
        const Clipper2Lib::Point64& p1,
        const Clipper2Lib::Point64& p2,
        const ConversionMetadata& metadata
    );

    void NaiveOffset(double offset);

    Clipper2Lib::Path64 MakePoly(const CCurve& curve, ConversionMetadata& metadata) const;

    void SetFromResult(
        Clipper2Lib::Paths64& paths,
        bool isClosed,
        ConversionMetadata& metadata,
        std::optional<std::reference_wrapper<CArea>> cNeg = std::nullopt
    );

    // Internal implementation of Clip with optional open path reversal
    void _Clip(
        Clipper2Lib::ClipType op,
        const CArea& clip_area,
        Clipper2Lib::FillRule fillType,
        bool reverseOpenPathContents = false,
        bool reverseOpenPathOrder = false,
        std::optional<std::reference_wrapper<CArea>> cNeg = std::nullopt
    );
};

enum eOverlapType
{
    eOutside,
    eInside,
    eSiblings,
    eCrossing,
};

eOverlapType GetOverlapType(const CCurve& c1, const CCurve& c2);
eOverlapType GetOverlapType(const CArea& a1, const CArea& a2);

}  // namespace heeks
