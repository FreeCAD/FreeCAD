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

PYBIND11_MODULE(surface_stl, m)
{
    m.doc() = "C++ helper: return raw mesh arrays from Part.Shape / Part.Compound";
    m.def(
        "shape_tessellate_fast",
        &shape_tessellate_fast,
        py::arg("shape"),
        py::arg("linear_deflection"),
        py::arg("angular_deflection"),
        py::arg("timer_callback") = py::none()
    );
}
