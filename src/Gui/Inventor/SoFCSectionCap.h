/******************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association <www.freecad.org>         *
 *                                                                            *
 *   This file is part of FreeCAD.                                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify it       *
 *   under the terms of the GNU Lesser General Public License as              *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU         *
 *   Lesser General Public License for more details.                          *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see                                  *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <Inventor/SbBox3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <FCGlobal.h>

class SbPlane;
class SoGLRenderAction;

namespace Gui
{
namespace Inventor
{

/// What the renderer did for one occurrence and one plane in the last frame.
struct GuiExport SectionCapRecord
{
    std::string key;        ///< occurrence
    std::string component;  ///< the object that owns the geometry
    int plane {0};
    SbColor color {0.0f, 0.0f, 0.0f};  ///< the part's color, if capped
    std::string hatch;                 ///< its hatch, if capped
    std::string status;                ///< "capped", "skipped: not cut", "unsupported: ..."
};

/** Fills the cut faces of the active clip planes in the part's own material,
 * using the stencil buffer, hatches and outlines them. Each part gets its own
 * hatch.
 *
 * Must be the last child of the viewer's view provider root, so that every clip
 * plane is in the traversal state. Open and transparent meshes are reported in
 * the records and left uncapped.
 */
class GuiExport SoFCSectionCap: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCSectionCap);

public:
    static void initClass();
    SoFCSectionCap();

    /// "Doc#Object" for a view provider root ("Doc#Link\tDoc#Target" for a link), else "".
    /// Sets excluded for roots that are never capped (datums, origins).
    using Resolver = std::function<std::string(SoNode*, bool& excluded)>;

    void setScene(SoNode* scene);
    void setResolver(Resolver resolve);

    const std::vector<SectionCapRecord>& lastRecords() const
    {
        return records;
    }
    /// "capped", "no-plane", "no-geometry", "no-cap" or "no-stencil"
    const std::string& lastStatus() const
    {
        return status;
    }

    void GLRender(SoGLRenderAction* action) override;

protected:
    ~SoFCSectionCap() override;

private:
    struct Material
    {
        SbColor ambient {0.2f, 0.2f, 0.2f};
        SbColor diffuse {0.8f, 0.8f, 0.8f};
        SbColor specular {0.0f, 0.0f, 0.0f};
        SbColor emission {0.0f, 0.0f, 0.0f};
        float shininess {0.2f};
    };
    struct Group
    {
        std::string key;
        std::string component;
        std::vector<float> tris;  // world space, 9 floats per triangle
        SbBox3f box;
        bool transparent {false};
        bool closed {true};
        Material material;
        std::vector<std::pair<std::array<float, 4>, std::vector<float>>> outlines;  // per plane
        std::map<std::uint32_t, std::pair<Material, int>> materials;  // votes, by diffuse
    };

    void updateProxy();
    static std::uint64_t geometrySignature(SoNode* root);
    static void checkClosed(Group& group, std::vector<float>& patch);
    static const std::vector<float>& outlineOf(Group& group, const SbPlane& plane);
    int hatchFor(const std::string& key);

    SoNode* scene {nullptr};
    Resolver resolve;
    std::map<std::string, int> hatchOf;  // by key, so a part keeps its hatch
    std::vector<int> hatchUse;

    std::vector<Group> groups;
    std::map<std::string, std::size_t> groupIndex;
    std::uint32_t proxySceneId {0};
    std::uint64_t proxySignature {0};
    struct ClosedInfo
    {
        std::uint64_t digest {0};  // of the collected triangles
        bool closed {false};
        std::vector<float> patch;  // triangles that close the cracks
    };
    std::map<std::string, ClosedInfo> closedCache;
    bool proxyValid {false};
    const void* lastShape {nullptr};
    std::size_t lastGroup {0};

    std::vector<SectionCapRecord> records;
    std::string status {"idle"};
};

}  // namespace Inventor

}  // namespace Gui
