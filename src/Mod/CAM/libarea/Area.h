// SPDX-License-Identifier: BSD-3-Clause

// Area.h
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// repository now moved to github

#pragma once

#include <map>
#include <optional>
#include <set>
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


// Arc fitting map for tracking arc information through Clipper operations
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
    std::map<std::pair<int64_t, int64_t>, SegmentData> zData;

    // Deduplication cache: maps (x,y) in Clipper coordinates to the z-label already assigned there
    std::map<std::pair<int64_t, int64_t>, int64_t> xy_to_z;

    // Track the maximum z-value used for allocation
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
    ConversionMetadata m_metadata;
    static double m_accuracy;
    static double m_clipper_clean_distance;
    static bool m_fit_arcs;
    static int m_min_arc_points;
    static int m_max_arc_points;
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
    void OffsetInward(double inwards_value);  // Deprecated: use Offset

    // Offsets wires; input wires must be closed shapes
    void Offset(
        double offset,
        Clipper2Lib::JoinType joinType = Clipper2Lib::JoinType::Round,
        Clipper2Lib::EndType endType = Clipper2Lib::EndType::Round,
        double miterLimit = 5.0,
        double arcTolerance = 0.0
    );

    // Offsets wires; open wires allowed. Positive offset is kept in this CArea, and
    // negative offset is returned in a new CArea. Endcaps (round) are filtered out
    CArea OpenOffset(double offset, double arcTolerance = 0.0);

    void ClipperNoop();  // converts to clipper and back (arc fiting) without performing clipper ops
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

    // Test helper method for checking that if clipper reverses open paths,
    // it is handled properly.
    //
    // This CArea should hold an open path, and the provided clip_area should have a closed
    // path. This method will behave the same as Intersect, but before converting clipper
    // results back to CArea it will optionally reverse the open paths.
    //
    // reverseOpenPathContents: if true, reverse the contents (vertices) of each open path
    // reverseOpenPathOrder: if true, reverse the order of the open paths themselves
    void TestIntersectOpenPathReversal(
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
    CAREA_PARAM_DECLARE(double, clipper_clean_distance)
    CAREA_PARAM_DECLARE(double, accuracy)
    CAREA_PARAM_DECLARE(double, clipper_scale)

    void PopulateClipper(Clipper2Lib::Clipper64& c, bool as_clip);

    // Following functions is add to operate on possible open curves
    void Clip(
        Clipper2Lib::ClipType op,
        const CArea& clip_area,
        Clipper2Lib::FillRule subjFillType = Clipper2Lib::FillRule::EvenOdd,
        Clipper2Lib::FillRule clipFillType = Clipper2Lib::FillRule::EvenOdd
    );

    // Reorders open paths so they match the original input order and direction.
    // Must be run after Clipper operations, before converting back to arcs.
    void ReorderOpenPaths(Clipper2Lib::Paths64& paths);

private:
    // Z-callback for Clipper intersection handling
    void ZCallback(
        const Clipper2Lib::Point64& e1bot,
        const Clipper2Lib::Point64& e1top,
        const Clipper2Lib::Point64& e2bot,
        const Clipper2Lib::Point64& e2top,
        Clipper2Lib::Point64& pt
    );

    // Helper to create bound Z callback
    Clipper2Lib::ZCallback64 MakeZCallback();

    // Returns (minZ, maxZ) of the vertices of the parent edge
    std::pair<int64_t, int64_t> getParentEdge(
        const Clipper2Lib::Point64& p1,
        const Clipper2Lib::Point64& p2
    );

    std::list<std::list<int>> NaiveOffset(double offset, double arcTolerance = 0);

    Clipper2Lib::Path64 MakePoly(const CCurve& curve, std::list<int> edge_orientations = {});

    void _Union(
        const std::list<std::list<int>>& orientations,
        std::optional<std::reference_wrapper<CArea>> cNeg = std::nullopt
    );

    void SetFromResult(
        Clipper2Lib::Paths64& paths,
        bool isClosed,
        std::optional<std::reference_wrapper<CArea>> cNeg = std::nullopt
    );

    // Internal implementation of Clip with optional open path reversal
    void _Clip(
        Clipper2Lib::ClipType op,
        const CArea& clip_area,
        Clipper2Lib::FillRule subjFillType,
        Clipper2Lib::FillRule clipFillType,
        bool reverseOpenPathContents,
        bool reverseOpenPathOrder
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
