// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ThinProfile.h"
#include <BRepTools_WireExplorer.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <Base/Exception.h>

namespace PartDesign
{
using Part::TopoShape;
ThinProfileGraph makeThinProfileGraph(const Part::TopoShape& profile, long tag)
{
    auto edges = profile.getSubTopoShapes(TopAbs_EDGE);
    if (edges.empty()) {
        throw Base::ValueError("Rib requires profile edges");
    }

    for (auto& edge : edges) {
        edge.setShape(edge.getShape().Oriented(TopAbs_FORWARD), false);
    }

    // Split geometric crossings and share coincident vertices using the kernel's
    // arrangement/history machinery. Wire assembly alone cannot detect an X.
    if (edges.size() > 1) {
        std::vector<std::vector<TopoShape>> modified;
        TopoShape split(tag, profile.Hasher);
        split.makeElementGeneralFuse(edges, modified, 0.0, "ThinCrossings");
        edges = split.getSubTopoShapes(TopAbs_EDGE);
    }

    TopTools_IndexedMapOfShape vertices;
    std::vector<std::pair<int, int>> ends;
    for (const auto& edge : edges) {
        TopoDS_Vertex a, b;
        TopExp::Vertices(TopoDS::Edge(edge.getShape()), a, b);
        ends.emplace_back(vertices.Add(a) - 1, vertices.Add(b) - 1);
    }

    std::vector<std::vector<size_t>> incidence(vertices.Extent());
    for (size_t i = 0; i < ends.size(); ++i) {
        incidence[ends[i].first].push_back(i);
        incidence[ends[i].second].push_back(i);
    }

    std::vector<bool> used(edges.size(), false);
    std::vector<TopoShape> chains;
    auto walk = [&](size_t seed, int vertex) {
        std::vector<TopoShape> chain;
        auto current = seed;
        while (!used[current]) {
            used[current] = true;
            chain.push_back(edges[current]);
            vertex = ends[current].first == vertex ? ends[current].second : ends[current].first;
            if (incidence[vertex].size() != 2) {
                break;
            }
            current = incidence[vertex][0] == current ? incidence[vertex][1] : incidence[vertex][0];
        }
        chains.push_back(TopoShape(tag, profile.Hasher).makeElementWires(chain, "ThinChain"));
    };

    for (size_t v = 0; v < incidence.size(); ++v) {
        if (incidence[v].size() != 2) {
            for (auto e : incidence[v]) {
                if (!used[e]) {
                    walk(e, v);
                }
            }
        }
    }
    for (size_t e = 0; e < edges.size(); ++e) {
        if (!used[e]) {
            walk(e, ends[e].first);
        }
    }
    ThinProfileGraph result;
    for (size_t v = 0; v < incidence.size(); ++v) {
        if (incidence[v].size() == 1) {
            result.freeVertices.Add(vertices(v + 1));
        }
    }

    for (auto wire : chains) {
        // Anchor a chain to source topology, not selection order or world axes.
        for (const auto& edge : edges) {
            bool found = false;
            for (BRepTools_WireExplorer it(TopoDS::Wire(wire.getShape())); it.More(); it.Next()) {
                if (it.Current().IsSame(edge.getShape())) {
                    if (it.Current().Orientation() != TopAbs_FORWARD) {
                        wire.setShape(wire.getShape().Reversed(), false);
                    }
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        result.chains.push_back(wire);
    }

    return result;
}

}  // namespace PartDesign
