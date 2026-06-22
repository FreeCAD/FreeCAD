
// SPDX-License-Identifier: BSD-3-Clause

// pyarea.cpp
// Copyright 2017, Lorenz Lechner
// This program is released under the BSD license. See the file COPYING for details.


#ifdef _MSC_VER
# define strdup _strdup
#endif

#include "Area.h"
#include "Point.h"
#include "Adaptive.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>

#include <vector>


namespace py = pybind11;

using namespace heeks;

std::list<CVertex> getVertices(const CCurve& curve)
{
    return curve.m_vertices;
}

std::list<CCurve> getCurves(const CArea& area)
{
    return area.m_curves;
}

static void print_curve(const CCurve& c)
{
    std::size_t nvertices = c.m_vertices.size();
#if defined SIZEOF_SIZE_T && SIZEOF_SIZE_T == 4
    printf("number of vertices = %d\n", nvertices);
#elif defined(_WIN32)
    printf("number of vertices = %zu\n", nvertices);
#else
    printf("number of vertices = %lu\n", nvertices);
#endif
    int i = 0;
    for (std::list<CVertex>::const_iterator It = c.m_vertices.begin(); It != c.m_vertices.end();
         It++, i++) {
        const CVertex& vertex = *It;
        printf("vertex %d type = %d, x = %g, y = %g", i + 1, vertex.m_type, vertex.m_p.x, vertex.m_p.y);
        if (vertex.m_type) {
            printf(", xc = %g, yc = %g", vertex.m_c.x, vertex.m_c.y);
        }
        printf("\n");
    }
}

static void print_area(const CArea& a)
{
    for (std::list<CCurve>::const_iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++) {
        const CCurve& curve = *It;
        print_curve(curve);
    }
}

static unsigned int num_vertices(const CCurve& curve)
{
    return static_cast<unsigned int>(curve.m_vertices.size());
}

static CVertex FirstVertex(const CCurve& curve)
{
    return curve.m_vertices.front();
}

static CVertex LastVertex(const CCurve& curve)
{
    return curve.m_vertices.back();
}

static double get_accuracy()
{
    return CArea::get_accuracy();
}

static void set_accuracy(double accuracy)
{
    CArea::set_accuracy(accuracy);
}

static double get_clipper_scale()
{
    return CArea::get_clipper_scale();
}

static void set_clipper_scale(double scale)
{
    CArea::set_clipper_scale(scale);
}

static CArea copy_area(const CArea& area)
{
    CArea copy;
    copy.m_curves = area.m_curves;
    copy.m_arc_fitting_map = area.m_arc_fitting_map;
    return copy;
}

static bool holes_linked()
{
    return CArea::HolesLinked();
}

static void append_point(CCurve& c, const Point& p)
{
    c.m_vertices.push_back(CVertex(p));
}

static py::tuple nearest_point_to_curve(CCurve& c1, const CCurve& c2)
{
    double dist;
    Point p = c1.NearestPoint(c2, &dist);

    return py::make_tuple(p, dist);
}

std::list<CCurve> MakePocketToolpath(
    const CArea& a,
    double tool_radius,
    double extra_offset,
    double stepover,
    bool from_center,
    bool use_zig_zag,
    double zig_angle
)
{
    std::list<CCurve> toolpath;

    CAreaPocketParams params(
        tool_radius,
        extra_offset,
        stepover,
        from_center,
        use_zig_zag ? ZigZagPocketMode : SpiralPocketMode,
        zig_angle
    );
    a.SplitAndMakePocketToolpath(toolpath, params);

    return toolpath;
}

std::list<CArea> SplitArea(const CArea& a)
{
    std::list<CArea> areas;
    a.Split(areas);

    return areas;
}

py::list getCurveSpans(const CCurve& c)
{
    py::list span_list;
    const Point* prev_p = NULL;

    for (std::list<CVertex>::const_iterator VIt = c.m_vertices.begin(); VIt != c.m_vertices.end();
         VIt++) {
        const CVertex& vertex = *VIt;

        if (prev_p) {
            span_list.append(Span(*prev_p, vertex));
        }
        prev_p = &(vertex.m_p);
    }

    return span_list;
}

Span getFirstCurveSpan(const CCurve& c)
{
    if (c.m_vertices.size() < 2) {
        return Span();
    }

    std::list<CVertex>::const_iterator VIt = c.m_vertices.begin();
    const Point& p = (*VIt).m_p;
    VIt++;
    return Span(p, *VIt, true);
}

Span getLastCurveSpan(const CCurve& c)
{
    if (c.m_vertices.size() < 2) {
        return Span();
    }

    std::list<CVertex>::const_reverse_iterator VIt = c.m_vertices.rbegin();
    const CVertex& v = (*VIt);
    VIt++;

    return Span((*VIt).m_p, v, c.m_vertices.size() == 2);
}

double AreaGetArea(const CArea& a)
{
    return a.GetArea();
}


void init_pyarea(py::module& m)
{
    py::class_<Point>(m, "Point")
        .def(py::init<double, double>())
        .def(py::init<Point>())
        .def(float() * py::self)
        .def(py::self * float())
        .def(py::self / float())
        .def(py::self * py::self)
        .def(py::self - py::self)
        .def(py::self + py::self)
        .def(py::self ^ py::self)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(-py::self)
        .def(~py::self)
        .def("dist", &Point::dist)
        .def("length", &Point::length)
        .def("normalize", &Point::normalize)
        .def("Rotate", static_cast<void (Point::*)(double, double)>(&Point::Rotate))
        .def("Rotate", static_cast<void (Point::*)(double)>(&Point::Rotate))
        .def_readwrite("x", &Point::x)
        .def_readwrite("y", &Point::y);

    py::class_<CVertex>(m, "Vertex")
        .def(py::init<CVertex>())
        .def(py::init<int, Point, Point>())
        .def(py::init<Point>())
        .def_readwrite("type", &CVertex::m_type)
        .def_readwrite("p", &CVertex::m_p)
        .def_readwrite("c", &CVertex::m_c);

    py::class_<Span>(m, "Span")
        .def(py::init<Span>())
        .def(py::init<Point, CVertex, bool>())
        .def("NearestPoint", static_cast<Point (Span::*)(const Point& p) const>(&Span::NearestPoint))
        .def(
            "NearestPoint",
            static_cast<Point (Span::*)(const Span& p, double* d) const>(&Span::NearestPoint)
        )
        .def("GetBox", &Span::GetBox)
        .def("IncludedAngle", &Span::IncludedAngle)
        .def("GetArea", &Span::GetArea)
        .def("On", &Span::On)
        .def("MidPerim", &Span::MidPerim)
        .def("MidParam", &Span::MidParam)
        .def("Length", &Span::Length)
        .def("GetVector", &Span::GetVector)
        .def_readwrite("p", &Span::m_p)
        .def_readwrite("v", &Span::m_v);

    py::class_<CCurve>(m, "Curve")
        .def(py::init<>())
        .def("getVertices", &getVertices)
        .def("append", &CCurve::append)
        .def("append", &append_point)
        .def("text", &print_curve)
        .def("NearestPoint", static_cast<Point (CCurve::*)(const Point& p) const>(&CCurve::NearestPoint))
        .def("NearestPoint", &nearest_point_to_curve)
        .def("Reverse", &CCurve::Reverse)
        .def("getNumVertices", &num_vertices)
        .def("FirstVertex", &FirstVertex)
        .def("LastVertex", &LastVertex)
        .def("GetArea", &CCurve::GetArea)
        .def("IsClockwise", &CCurve::IsClockwise)
        .def("IsClosed", &CCurve::IsClosed)
        .def("ChangeStart", &CCurve::ChangeStart)
        .def("GetSpans", &getCurveSpans)
        .def("GetFirstSpan", &getFirstCurveSpan)
        .def("GetLastSpan", &getLastCurveSpan)
        .def("Perim", &CCurve::Perim)
        .def("PerimToPoint", &CCurve::PerimToPoint)
        .def("PointToPerim", &CCurve::PointToPerim);

    py::class_<CBox2D>(m, "Box")
        .def(py::init<CBox2D>())
        .def("MinX", &CBox2D::MinX)
        .def("MaxX", &CBox2D::MaxX)
        .def("MinY", &CBox2D::MinY)
        .def("MaxY", &CBox2D::MaxY);

    py::class_<CArea>(m, "Area")
        .def(py::init<>())
        .def("getCurves", &getCurves)
        .def("append", &CArea::append)
        .def("ClipperNoop", &CArea::ClipperNoop)
        .def("Subtract", &CArea::Subtract)
        .def("Intersect", &CArea::Intersect)
        .def("Union", &CArea::Union)
        .def("OffsetInward", &CArea::OffsetInward)  // Deprecated, prefer Offset
        .def("Offset", [](CArea& self, double offset) { self.Offset(offset); })
        .def("text", &print_area)
        .def("num_curves", &CArea::num_curves)
        .def("NearestPoint", &CArea::NearestPoint)
        .def("GetBox", &CArea::GetBox)
        .def("Reorder", &CArea::Reorder)
        .def("MakePocketToolpath", &MakePocketToolpath)
        .def("Split", &SplitArea)
        .def("Thicken", &CArea::Thicken)
        .def("GetArea", &AreaGetArea)
        .def("TestIntersectOpenPathReversal", &CArea::TestIntersectOpenPathReversal);

    m.def("get_accuracy", get_accuracy);
    m.def("set_accuracy", set_accuracy);
    m.def("get_clipper_scale", get_clipper_scale);
    m.def("set_clipper_scale", set_clipper_scale);
    m.def("copy_area", copy_area);
    m.def("holes_linked", holes_linked);

    using namespace AdaptivePath;
    py::enum_<MotionType>(m, "AdaptiveMotionType")
        .value("Cutting", MotionType::mtCutting)
        .value("LinkClear", MotionType::mtLinkClear)
        .value("LinkNotClear", MotionType::mtLinkNotClear)
        .value("LinkClearAtPrevPass", MotionType::mtLinkClearAtPrevPass);

    py::enum_<OperationType>(m, "AdaptiveOperationType")
        .value("ClearingInside", OperationType::otClearingInside)
        .value("ClearingOutside", OperationType::otClearingOutside)
        .value("ProfilingInside", OperationType::otProfilingInside)
        .value("ProfilingOutside", OperationType::otProfilingOutside);

    py::class_<AdaptiveOutput>(m, "AdaptiveOutput")
        .def(py::init<>())
        .def_readwrite("HelixCenterPoint", &AdaptiveOutput::HelixCenterPoint)
        .def_readwrite("StartPoint", &AdaptiveOutput::StartPoint)
        .def_readwrite("AdaptivePaths", &AdaptiveOutput::AdaptivePaths)
        .def_readwrite("ReturnMotionType", &AdaptiveOutput::ReturnMotionType)
        .def_readwrite("ClearedArea", &AdaptiveOutput::ClearedArea)
        .def_readwrite("StartPointNotFound", &AdaptiveOutput::StartPointNotFound)
        .def_readwrite("LeadPathFailed", &AdaptiveOutput::LeadPathFailed)
        .def_readwrite("UnexpectedRotateIterations", &AdaptiveOutput::UnexpectedRotateIterations)
        .def_readwrite("TooManyFailedEngagements", &AdaptiveOutput::TooManyFailedEngagements)
        .def_readwrite("UnclearedAreaRemains", &AdaptiveOutput::UnclearedAreaRemains)
        .def_readwrite("FailedToSetUpFinishingPass", &AdaptiveOutput::FailedToSetUpFinishingPass)
        .def_readwrite("FinishingLeadInFailed", &AdaptiveOutput::FinishingLeadInFailed);

    py::class_<Adaptive2d>(m, "Adaptive2d")
        .def(py::init<>())
        .def("Execute", &Adaptive2d::Execute)
        .def_readwrite("stepOverFactor", &Adaptive2d::stepOverFactor)
        .def_readwrite("toolDiameter", &Adaptive2d::toolDiameter)
        .def_readwrite("stockToLeave", &Adaptive2d::stockToLeave)
        .def_readwrite("helixRampTargetDiameter", &Adaptive2d::helixRampTargetDiameter)
        .def_readwrite("helixRampMinDiameter", &Adaptive2d::helixRampMinDiameter)
        .def_readwrite("forceInsideOut", &Adaptive2d::forceInsideOut)
        .def_readwrite("finishingProfile", &Adaptive2d::finishingProfile)
        .def_readwrite("tolerance", &Adaptive2d::tolerance)
        .def_readwrite("keepToolDownDistRatio", &Adaptive2d::keepToolDownDistRatio)
        .def_readwrite("opType", &Adaptive2d::opType);
}

PYBIND11_MODULE(area, m)
{
    m.doc() = "not yet";
    init_pyarea(m);
};
