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

class SoCallbackAction;
class SoGLRenderAction;
class SoPrimitiveVertex;
class SoState;

namespace Gui
{
namespace Inventor
{

/// How a section cap is filled.
struct GuiExport SectionCapStyle
{
    SbColor color {0.85f, 0.35f, 0.25f};
    std::string hatch {"none"};  ///< "none" or one of SoFCSectionCap::hatches()
    SbColor hatchColor {0.15f, 0.15f, 0.15f};
};

/// What the renderer did for one occurrence and one plane in the last frame.
struct GuiExport SectionCapRecord
{
    std::string key;        ///< occurrence
    std::string component;  ///< the object that owns the geometry
    int plane {0};
    SectionCapStyle style;
    std::string status;  ///< "capped", "skipped: not cut", "unsupported: ..."
};

/** Fills the cut faces of the active clip planes, using the stencil buffer.
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
    using Resolver = std::function<std::string(SoNode*)>;
    /// True for roots that are never capped (datums, origins).
    using Excluder = std::function<bool(SoNode*)>;

    void setScene(SoNode* scene);
    SoNode* getScene() const
    {
        return scene;
    }
    void setResolver(Resolver resolve, Excluder exclude);

    /// Drop the proxy (rebuilt on the next frame).
    void invalidate();

    const std::vector<SectionCapRecord>& lastRecords() const
    {
        return records;
    }
    /// "capped", "no-plane", "no-geometry", "no-cap" or "no-stencil"
    const std::string& lastStatus() const
    {
        return status;
    }

    /// FNV-1a, so default styles are the same on every run.
    static std::uint64_t digest(const std::string& key);
    /// The colors and hatches default styles are drawn from.
    static const std::vector<SbColor>& palette();
    static const std::vector<std::string>& hatches();

    void GLRender(SoGLRenderAction* action) override;

protected:
    ~SoFCSectionCap() override;

private:
    struct Group
    {
        std::string key;
        std::string component;
        std::vector<float> tris;  // world space, 9 floats per triangle
        SbBox3f box;
        bool transparent {false};
        bool closed {true};
    };

    void updateProxy();
    static std::uint64_t geometrySignature(SoNode* root);
    static void checkClosed(Group& group);
    SectionCapStyle styleFor(const Group& group);
    static void collectTriangle(
        void* self,
        SoCallbackAction* action,
        const SoPrimitiveVertex* v1,
        const SoPrimitiveVertex* v2,
        const SoPrimitiveVertex* v3
    );
    std::string keyForPath(SoCallbackAction* action, std::string& component);

    SoNode* scene {nullptr};
    Resolver resolve;
    Excluder exclude;
    std::map<std::string, SectionCapStyle> allocated;
    std::map<std::pair<int, int>, std::string> usedCombos;  // (color, hatch) -> key

    std::vector<Group> groups;
    std::map<std::string, std::size_t> groupIndex;
    std::uint32_t proxySceneId {0};
    std::uint64_t proxySignature {0};
    // per key: digest of the triangles, closed
    std::map<std::string, std::pair<std::uint64_t, bool>> closedCache;
    bool proxyValid {false};
    const void* lastShape {nullptr};
    std::size_t lastGroup {0};
    bool lastExcluded {false};

    std::vector<SectionCapRecord> records;
    std::string status {"idle"};
    bool rendering {false};
};

}  // namespace Inventor

}  // namespace Gui
