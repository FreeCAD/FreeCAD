// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License (LGPL)    *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopAbs.hxx>

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Trsf.hxx>

#include <vector>
#include <array>
#include <chrono>
#include <stdexcept>
#include <Python.h>
#include <unordered_map>
#include <limits>

// Include the TopoShapePy header using the full path
#include <Mod/Part/App/TopoShapePy.h>

namespace py = pybind11;

// Extract TopoDS_Shape from Python object using the exact FreeCAD pattern
static TopoDS_Shape extractTopoDS(py::object shape)
{
    PyObject* obj = shape.ptr();
    if (PyObject_IsInstance(obj, reinterpret_cast<PyObject*>(&Part::TopoShapePy::Type))) {
        Part::TopoShapePy* pyShape = static_cast<Part::TopoShapePy*>(obj);
        TopoDS_Shape sh = pyShape->getTopoShapePtr()->getShape();
        if (!sh.IsNull()) {
            return sh;
        }
    }

    throw std::runtime_error("Unable to extract TopoDS_Shape from object - expected Part.TopoShape");
}

/**
 * Fast C++ implementation of shape tessellation.
 *
 * This bypasses the Python bottleneck by:
 * 1. Using OCCT's native meshing directly
 * 2. Extracting triangles in C++ without Python loops
 * 3. Returning raw triangle data for Python to convert to OCL
 */
std::pair<std::vector<std::array<double, 3>>, std::vector<std::array<int, 3>>> shape_tessellate_fast(
    py::object shape,
    double linear_deflection,
    double angular_deflection,
    py::object timer_callback = py::none()
)
{
    TopoDS_Shape topo_shape = extractTopoDS(shape);

    // 1️⃣ Tessellation
    auto tess_start = std::chrono::high_resolution_clock::now();
    BRepMesh_IncrementalMesh(topo_shape, linear_deflection, false, angular_deflection);
    auto tess_end = std::chrono::high_resolution_clock::now();

    if (!timer_callback.is_none()) {
        double tess_time = std::chrono::duration<double>(tess_end - tess_start).count();
        timer_callback("tessellate", tess_time);
    }

    // 2️⃣ Extract triangles
    auto extract_start = std::chrono::high_resolution_clock::now();

    std::vector<std::array<double, 3>> vertices;
    std::vector<std::array<int, 3>> facets;

    // Map from vertex coordinates to vertex index for deduplication
    std::unordered_map<std::string, int> vertex_map;
    auto get_vertex_index = [&](const gp_Pnt& p) -> int {
        // Round coordinates to handle floating point precision issues
        std::string key = std::to_string(std::round(p.X() * 1000000.0) / 1000000.0) + ","
            + std::to_string(std::round(p.Y() * 1000000.0) / 1000000.0) + ","
            + std::to_string(std::round(p.Z() * 1000000.0) / 1000000.0);

        auto it = vertex_map.find(key);
        if (it != vertex_map.end()) {
            return it->second;
        }

        // Add new vertex
        int index = vertices.size();
        vertices.push_back({p.X(), p.Y(), p.Z()});
        vertex_map[key] = index;
        return index;
    };

    for (TopExp_Explorer exp(topo_shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) {
            continue;
        }

        gp_Trsf trsf = loc.Transformation();
        bool is_reversed = (face.Orientation() == TopAbs_REVERSED);

        for (int i = 1; i <= tri->NbTriangles(); ++i) {
            int n1, n2, n3;
            tri->Triangle(i).Get(n1, n2, n3);

            gp_Pnt p1 = tri->Node(n1).Transformed(trsf);
            gp_Pnt p2 = tri->Node(n2).Transformed(trsf);
            gp_Pnt p3 = tri->Node(n3).Transformed(trsf);

            // Check for degenerate triangles (all three points must be different)
            // Distance threshold to consider points identical
            const double tolerance = 1e-10;

            bool p1_equals_p2 = (p1.Distance(p2) < tolerance);
            bool p1_equals_p3 = (p1.Distance(p3) < tolerance);
            bool p2_equals_p3 = (p2.Distance(p3) < tolerance);

            // Skip degenerate triangles
            if (p1_equals_p2 || p1_equals_p3 || p2_equals_p3) {
                continue;
            }

            // Get deduplicated vertex indices
            int v1_idx = get_vertex_index(p1);
            int v2_idx = get_vertex_index(p2);
            int v3_idx = get_vertex_index(p3);

            // Maintain correct outward-facing normals
            if (is_reversed) {
                std::swap(v2_idx, v3_idx);
            }

            facets.push_back({v1_idx, v2_idx, v3_idx});
        }
    }

    auto extract_end = std::chrono::high_resolution_clock::now();
    if (!timer_callback.is_none()) {
        double extract_time = std::chrono::duration<double>(extract_end - extract_start).count();
        timer_callback("extract_triangles", extract_time);
    }

    return {vertices, facets};
}

// -------------------------------------------------------------------------
// Fast 2D Pattern Generation & Binary Search Boundary Clipping
// -------------------------------------------------------------------------

struct PolyBounds
{
    double xmin, xmax, ymin, ymax;
};

// Extremely fast PIP using Bounding Box Pre-check
bool isInsideFast(
    double x,
    double y,
    const std::vector<std::vector<std::array<double, 2>>>& polygons,
    const std::vector<PolyBounds>& bounds
)
{
    if (polygons.empty()) {
        return true;
    }
    bool inside = false;
    for (size_t p = 0; p < polygons.size(); ++p) {
        if (x < bounds[p].xmin || x > bounds[p].xmax || y < bounds[p].ymin || y > bounds[p].ymax) {
            continue;  // Skip ray-cast if completely outside this polygon's bounds
        }

        const auto& poly = polygons[p];
        if (poly.size() < 3) {
            continue;
        }
        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
            if (((poly[i][1] > y) != (poly[j][1] > y))
                && (x < (poly[j][0] - poly[i][0]) * (y - poly[i][1]) / (poly[j][1] - poly[i][1])
                        + poly[i][0])) {
                inside = !inside;
            }
        }
    }
    return inside;
}

// Helper to pre-calculate bounding boxes for polygons
std::vector<PolyBounds> calculate_bounds(const std::vector<std::vector<std::array<double, 2>>>& polygons)
{
    std::vector<PolyBounds> bounds(polygons.size());
    for (size_t p = 0; p < polygons.size(); ++p) {
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();

        for (const auto& pt : polygons[p]) {
            if (pt[0] < min_x) {
                min_x = pt[0];
            }
            if (pt[0] > max_x) {
                max_x = pt[0];
            }
            if (pt[1] < min_y) {
                min_y = pt[1];
            }
            if (pt[1] > max_y) {
                max_y = pt[1];
            }
        }
        bounds[p] = {min_x, max_x, min_y, max_y};
    }
    return bounds;
}

// Binary Search (Bisection) to find the exact boundary edge with 0.005mm tolerance!
std::array<double, 3> find_exact_edge(
    std::array<double, 3> p_in,
    std::array<double, 3> p_out,
    const std::vector<std::vector<std::array<double, 2>>>& polygons,
    const std::vector<PolyBounds>& bounds
)
{
    std::array<double, 3> p_mid;
    int max_iters = 20;  // Safety failsafe to prevent infinite loops from floating point noise

    // Stop exactly when the gap between the points is 0.005mm or less
    while (max_iters-- > 0 && std::hypot(p_out[0] - p_in[0], p_out[1] - p_in[1]) > 0.005) {
        p_mid = {(p_in[0] + p_out[0]) / 2.0, (p_in[1] + p_out[1]) / 2.0, (p_in[2] + p_out[2]) / 2.0};

        if (isInsideFast(p_mid[0], p_mid[1], polygons, bounds)) {
            p_in = p_mid;  // Midpoint is inside, move the inner bound outwards
        }
        else {
            p_out = p_mid;  // Midpoint is outside, move the outer bound inwards
        }
    }
    return p_in;  // Return the last known safe point exactly on the boundary
}

// Applies the Bisection clipping to any generated polyline
std::vector<std::vector<std::array<double, 3>>> clip_polyline_bisection(
    const std::vector<std::array<double, 3>>& polyline,
    const std::vector<std::vector<std::array<double, 2>>>& polygons,
    const std::vector<PolyBounds>& bounds
)
{
    std::vector<std::vector<std::array<double, 3>>> results;
    std::vector<std::array<double, 3>> current_segment;

    if (polyline.empty()) {
        return results;
    }

    bool prev_inside = isInsideFast(polyline[0][0], polyline[0][1], polygons, bounds);
    if (prev_inside) {
        current_segment.push_back(polyline[0]);
    }

    for (size_t i = 1; i < polyline.size(); ++i) {
        bool curr_inside = isInsideFast(polyline[i][0], polyline[i][1], polygons, bounds);

        if (curr_inside != prev_inside) {
            // Crossed boundary! Execute Binary Search.
            std::array<double, 3> p_in = prev_inside ? polyline[i - 1] : polyline[i];
            std::array<double, 3> p_out = prev_inside ? polyline[i] : polyline[i - 1];

            std::array<double, 3> edge_pt = find_exact_edge(p_in, p_out, polygons, bounds);

            if (prev_inside) {
                // Leaving the boundary
                current_segment.push_back(edge_pt);
                results.push_back(current_segment);
                current_segment.clear();
            }
            else {
                // Entering the boundary
                current_segment.push_back(edge_pt);
                current_segment.push_back(polyline[i]);  // also add the current inside point
            }
        }
        else if (curr_inside) {
            // Safely continuing inside the boundary
            current_segment.push_back(polyline[i]);
        }
        prev_inside = curr_inside;
    }
    if (!current_segment.empty()) {
        results.push_back(current_segment);
    }
    return results;
}

// -------------------------------------------------------------------------
// Fast Pattern Generators
// -------------------------------------------------------------------------

std::vector<std::vector<std::array<double, 3>>> generate_linear_pattern_cpp(
    double xmin,
    double xmax,
    double ymin,
    double ymax,
    double stepover,
    double angle_deg,
    bool is_zigzag,
    bool reversed,
    const std::vector<std::vector<std::array<double, 2>>>& polygons
)
{
    std::vector<std::vector<std::array<double, 3>>> final_endpoints;
    std::vector<PolyBounds> bounds = calculate_bounds(polygons);

    double angle_rad = angle_deg * M_PI / 180.0;
    double cos_a = std::cos(angle_rad);
    double sin_a = std::sin(angle_rad);

    double cx = (xmin + xmax) / 2.0;
    double cy = (ymin + ymax) / 2.0;
    double diag = std::hypot(xmax - xmin, ymax - ymin);
    int num_passes = static_cast<int>(std::ceil(diag / stepover)) + 1;

    for (int i = -num_passes / 2; i <= num_passes / 2; ++i) {
        double y_off = i * stepover;
        double start_x = cx + (-diag) * cos_a - y_off * sin_a;
        double start_y = cy + (-diag) * sin_a + y_off * cos_a;
        double end_x = cx + diag * cos_a - y_off * sin_a;
        double end_y = cy + diag * sin_a + y_off * cos_a;

        // Use a small number of samples, just enough to detect boundary crossings
        int num_samples = 200;
        std::vector<std::array<double, 3>> raw_line(num_samples + 1);
        for (int j = 0; j <= num_samples; ++j) {
            double t = static_cast<double>(j) / num_samples;
            raw_line[j] = {start_x + t * (end_x - start_x), start_y + t * (end_y - start_y), 0.0};
        }

        // Find precise entry and exit points
        std::vector<std::vector<std::array<double, 3>>> segments;
        bool prev_inside = isInsideFast(raw_line[0][0], raw_line[0][1], polygons, bounds);
        for (size_t k = 1; k < raw_line.size(); ++k) {
            bool curr_inside = isInsideFast(raw_line[k][0], raw_line[k][1], polygons, bounds);
            if (curr_inside != prev_inside) {
                if (!prev_inside) {  // Entering
                    segments.push_back(
                        {find_exact_edge(raw_line[k], raw_line[k - 1], polygons, bounds)}
                    );
                }
                else {  // Exiting
                    if (!segments.empty() && segments.back().size() == 1) {
                        segments.back().push_back(
                            find_exact_edge(raw_line[k - 1], raw_line[k], polygons, bounds)
                        );
                    }
                }
            }
            prev_inside = curr_inside;
        }
        // Handle case where line ends inside the boundary
        if (prev_inside && !segments.empty() && segments.back().size() == 1) {
            segments.back().push_back(raw_line.back());
        }

        if (is_zigzag && (i % 2 != 0)) {
            for (auto& seg : segments) {
                std::reverse(seg.begin(), seg.end());
            }
            std::reverse(segments.begin(), segments.end());
        }
        for (const auto& seg : segments) {
            if (seg.size() == 2) {
                final_endpoints.push_back(seg);
            }
        }
    }

    if (reversed) {
        std::reverse(final_endpoints.begin(), final_endpoints.end());
    }
    return final_endpoints;
}

std::vector<std::vector<std::array<double, 3>>> generate_circular_pattern_cpp(
    double xmin,
    double xmax,
    double ymin,
    double ymax,
    double cx,
    double cy,
    double stepover,
    double sample_interval,
    bool is_zigzag,
    bool reversed,
    const std::vector<std::vector<std::array<double, 2>>>& polygons
)
{
    std::vector<std::vector<std::array<double, 3>>> scan_lines;
    std::vector<PolyBounds> bounds = calculate_bounds(polygons);

    double d1 = std::hypot(xmin - cx, ymin - cy);
    double d2 = std::hypot(xmin - cx, ymax - cy);
    double d3 = std::hypot(xmax - cx, ymin - cy);
    double d4 = std::hypot(xmax - cx, ymax - cy);
    double max_radius = std::max({d1, d2, d3, d4}) + stepover;

    int num_passes = static_cast<int>(std::ceil(max_radius / stepover)) + 1;
    std::vector<int> passes;
    for (int i = 1; i <= num_passes; ++i) {
        passes.push_back(i);
    }
    if (reversed) {
        std::reverse(passes.begin(), passes.end());
    }

    int pass_idx = 0;
    for (int i : passes) {
        double r = i * stepover;
        int n_pts = std::max(16, static_cast<int>(std::ceil(2 * M_PI * r / sample_interval)));
        bool cw = is_zigzag && (pass_idx % 2 != 0);

        std::vector<std::array<double, 3>> raw_ring;
        for (int j = 0; j <= n_pts; ++j) {
            double a = 2.0 * M_PI * j / n_pts;
            if (cw) {
                a = -a;
            }
            raw_ring.push_back({cx + r * std::cos(a), cy + r * std::sin(a), 0.0});
        }

        auto clipped_segments = clip_polyline_bisection(raw_ring, polygons, bounds);
        for (auto& seg : clipped_segments) {
            scan_lines.push_back(seg);
        }
        pass_idx++;
    }
    return scan_lines;
}

std::vector<std::vector<std::array<double, 3>>> generate_spiral_pattern_cpp(
    double xmin,
    double xmax,
    double ymin,
    double ymax,
    double cx,
    double cy,
    double stepover,
    double sample_interval,
    bool reversed,
    const std::vector<std::vector<std::array<double, 2>>>& polygons
)
{
    std::vector<std::vector<std::array<double, 3>>> scan_lines;
    std::vector<PolyBounds> bounds = calculate_bounds(polygons);

    double d1 = std::hypot(xmin - cx, ymin - cy);
    double d2 = std::hypot(xmin - cx, ymax - cy);
    double d3 = std::hypot(xmax - cx, ymin - cy);
    double d4 = std::hypot(xmax - cx, ymax - cy);
    double max_radius = std::max({d1, d2, d3, d4}) + stepover;

    double b = stepover / (2.0 * M_PI);
    double stop_radians = (b > 0) ? (max_radius / b) : 0;

    std::vector<std::array<double, 3>> raw_points;
    double theta = 0.0;
    int loop_count = 0;
    double loop_radians = 0.0;

    while (theta <= stop_radians) {
        double r = b * theta;
        raw_points.push_back({cx + r * std::cos(theta), cy + r * std::sin(theta), 0.0});

        double current_radius = std::max(static_cast<double>(loop_count + 1) * stepover, stepover);
        double step_angle = sample_interval / current_radius;
        theta += step_angle;
        loop_radians += step_angle;

        if (loop_radians > 2.0 * M_PI) {
            loop_count++;
            loop_radians -= 2.0 * M_PI;
        }
    }

    if (reversed) {
        std::reverse(raw_points.begin(), raw_points.end());
    }

    scan_lines = clip_polyline_bisection(raw_points, polygons, bounds);
    return scan_lines;
}


PYBIND11_MODULE(surface_generator, m)
{
    m.doc() = "C++ helper for 3D Surface operations";

    m.def(
        "shape_tessellate_fast",
        &shape_tessellate_fast,
        py::arg("shape"),
        py::arg("linear_deflection"),
        py::arg("angular_deflection"),
        py::arg("timer_callback") = py::none()
    );

    m.def("generate_linear_pattern_cpp", &generate_linear_pattern_cpp);
    m.def("generate_circular_pattern_cpp", &generate_circular_pattern_cpp);
    m.def("generate_spiral_pattern_cpp", &generate_spiral_pattern_cpp);
}
