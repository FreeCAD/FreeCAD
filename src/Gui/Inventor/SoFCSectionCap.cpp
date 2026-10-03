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

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoIndexedShape.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/system/gl.h>

#include "SoFCSectionCap.h"

using namespace Gui::Inventor;

SO_NODE_SOURCE(SoFCSectionCap)

namespace
{

/// A 32x32 polygon stipple for a hatch name (screen space, 8 px spacing).
bool stipplePattern(const std::string& hatch, GLubyte out[128])
{
    const auto& names = SoFCSectionCap::hatches();
    if (std::find(names.begin(), names.end(), hatch) == names.end()) {
        return false;  // "none"
    }
    std::memset(out, 0, 128);
    constexpr int s = 8;
    constexpr int dot = 2;
    auto mod = [](int a, int m) {
        return ((a % m) + m) % m;
    };
    const bool horizontal = hatch == "horizontal" || hatch == "cross" || hatch == "horizontal_dots";
    const bool vertical = hatch == "vertical" || hatch == "cross";
    const bool diag45 = hatch == "diag45" || hatch == "diagcross" || hatch == "diag45_dots";
    const bool diag135 = hatch == "diag135" || hatch == "diagcross";
    const bool dots = hatch == "dots" || hatch == "horizontal_dots";
    const bool diagDots = hatch == "diag45_dots";
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bool on = (horizontal && mod(y, s) == 0)
                || (vertical && mod(x, s) == 0)
                // stipple rows start at the bottom, so x - y = k rises like '/'
                || (diag45 && mod(x - y, s) == 0) || (diag135 && mod(x + y, s) == 0);
            if (dots) {
                on = on || (mod(x - s / 2, s) < dot && mod(y - s / 2, s) < dot);
            }
            if (diagDots) {  // on x - y = s/2, halfway between the '/' lines
                on = on || (mod(x - 3 * s / 4, s) < dot && mod(y - s / 4, s) < dot);
            }
            if (on) {
                out[y * 4 + x / 8] |= static_cast<GLubyte>(0x80 >> (x % 8));
            }
        }
    }
    return true;
}

/// Screen rectangle of a world box; false if the box reaches behind the camera.
bool screenRect(
    const SbBox3f& box,
    const SbMatrix& viewProj,
    const SbViewportRegion& region,
    int& x0,
    int& y0,
    int& x1,
    int& y1
)
{
    const SbVec2s origin = region.getViewportOriginPixels();
    const SbVec2s size = region.getViewportSizePixels();
    SbVec3f lo, hi;
    box.getBounds(lo, hi);
    float minx = 1e30f, miny = 1e30f, maxx = -1e30f, maxy = -1e30f;
    for (int i = 0; i < 8; ++i) {
        const SbVec3f corner((i & 1) ? hi[0] : lo[0], (i & 2) ? hi[1] : lo[1], (i & 4) ? hi[2] : lo[2]);
        // no divide: w <= 0 means behind the camera
        float in[4] = {corner[0], corner[1], corner[2], 1.0f};
        float res[4] = {0, 0, 0, 0};
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                res[c] += in[r] * viewProj[r][c];
            }
        }
        if (res[3] <= 1e-6f) {
            return false;
        }
        const float nx = res[0] / res[3];
        const float ny = res[1] / res[3];
        const float px = origin[0] + (nx * 0.5f + 0.5f) * size[0];
        const float py = origin[1] + (ny * 0.5f + 0.5f) * size[1];
        minx = std::min(minx, px);
        maxx = std::max(maxx, px);
        miny = std::min(miny, py);
        maxy = std::max(maxy, py);
    }
    x0 = std::max<int>(origin[0], static_cast<int>(std::floor(minx)) - 2);
    y0 = std::max<int>(origin[1], static_cast<int>(std::floor(miny)) - 2);
    x1 = std::min<int>(origin[0] + size[0], static_cast<int>(std::ceil(maxx)) + 2);
    y1 = std::min<int>(origin[1] + size[1], static_cast<int>(std::ceil(maxy)) + 2);
    return x1 > x0 && y1 > y0;
}

bool planeCutsBox(const SbPlane& plane, const SbBox3f& box)
{
    SbVec3f lo, hi;
    box.getBounds(lo, hi);
    bool above = false, below = false;
    for (int i = 0; i < 8; ++i) {
        const SbVec3f corner((i & 1) ? hi[0] : lo[0], (i & 2) ? hi[1] : lo[1], (i & 4) ? hi[2] : lo[2]);
        const float d = plane.getNormal().dot(corner) - plane.getDistanceFromOrigin();
        above = above || d > 0.0f;
        below = below || d < 0.0f;
    }
    return above && below;
}

}  // namespace

void SoFCSectionCap::initClass()
{
    SO_NODE_INIT_CLASS(SoFCSectionCap, SoNode, "Node");
}

SoFCSectionCap::SoFCSectionCap()
{
    SO_NODE_CONSTRUCTOR(SoFCSectionCap);
}

SoFCSectionCap::~SoFCSectionCap()
{
    setScene(nullptr);
}

void SoFCSectionCap::setScene(SoNode* node)
{
    if (node == scene) {
        return;
    }
    if (node) {
        node->ref();
    }
    if (scene) {
        scene->unref();
    }
    scene = node;
    invalidate();
}

void SoFCSectionCap::setResolver(Resolver r, Excluder e)
{
    resolve = std::move(r);
    exclude = std::move(e);
    invalidate();
}

void SoFCSectionCap::invalidate()
{
    proxyValid = false;
    touch();
}

std::uint64_t SoFCSectionCap::digest(const std::string& key)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

const std::vector<SbColor>& SoFCSectionCap::palette()
{
    static const std::vector<SbColor> colors = {
        {0.90f, 0.37f, 0.26f},
        {0.25f, 0.55f, 0.85f},
        {0.36f, 0.72f, 0.36f},
        {0.93f, 0.69f, 0.13f},
        {0.58f, 0.40f, 0.74f},
        {0.20f, 0.70f, 0.70f},
        {0.85f, 0.45f, 0.65f},
        {0.60f, 0.60f, 0.22f},
        {0.62f, 0.38f, 0.22f},
        {0.47f, 0.57f, 0.95f},
        {0.75f, 0.20f, 0.27f},
        {0.45f, 0.47f, 0.52f},
    };
    return colors;
}

const std::vector<std::string>& SoFCSectionCap::hatches()
{
    // ordered so that neighbours differ the most
    static const std::vector<std::string> names = {
        "diag45",
        "horizontal",
        "dots",
        "diag135",
        "vertical",
        "diag45_dots",
        "diagcross",
        "horizontal_dots",
        "cross",
    };
    return names;
}

SectionCapStyle SoFCSectionCap::styleFor(const Group& group)
{
    auto found = allocated.find(group.key);
    if (found != allocated.end()) {
        return found->second;
    }
    // an unused color first, then an unused (color, hatch); both searches
    // start at the key's digest, so the result is the same on every run
    const int nc = static_cast<int>(palette().size());
    const int nh = static_cast<int>(hatches().size());
    const std::uint64_t h = digest(group.key);
    const int c0 = static_cast<int>(h % nc);
    const int t0 = static_cast<int>((h >> 32) % nh);
    auto colorUsed = [this](int c) {
        auto it = usedCombos.lower_bound({c, 0});
        return it != usedCombos.end() && it->first.first == c;
    };
    int chosen = c0 + t0 * nc;
    bool done = false;
    for (int k = 0; k < nc && !done; ++k) {
        const int c = (c0 + k) % nc;
        if (!colorUsed(c)) {
            chosen = c + t0 * nc;
            done = true;
        }
    }
    for (int k = 0; k < nc * nh && !done; ++k) {
        const int idx = (c0 + t0 * nc + k) % (nc * nh);
        if (usedCombos.find({idx % nc, idx / nc}) == usedCombos.end()) {
            chosen = idx;
            done = true;
        }
    }
    usedCombos[{chosen % nc, chosen / nc}] = group.key;
    SectionCapStyle style;
    style.color = palette()[chosen % nc];
    style.hatch = hatches()[chosen / nc];
    allocated[group.key] = style;
    return style;
}

void SoFCSectionCap::GLRender(SoGLRenderAction* action)
{
    SoState* state = action->getState();
    records.clear();

    if (!scene || rendering) {
        return;
    }
    const SoClipPlaneElement* elt = SoClipPlaneElement::getInstance(state);
    const int nplanes = elt ? elt->getNum() : 0;
    if (nplanes == 0) {
        status = "no-plane";
        return;
    }
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (stencilBits < 1) {
        status = "no-stencil";  // the framebuffer has no stencil buffer: leave the cut open
        return;
    }
    rendering = true;
    updateProxy();
    if (groups.empty()) {
        status = "no-geometry";
        rendering = false;
        return;
    }

    const SbMatrix& view = SoViewingMatrixElement::get(state);
    SbMatrix viewProj = view;
    viewProj.multRight(SoProjectionMatrixElement::get(state));
    const SbViewportRegion& region = SoViewportRegionElement::get(state);

    // We are about to change GL state and draw outside the scene graph's own
    // caching, so drop any render cache.  Only do this when there is actually
    // something to draw, so an idle (no clip plane) frame stays cache-friendly.
    SoCacheElement::invalidate(state);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(view[0]);  // the proxy and the cap polygons are in world space

    // Upload every plane ourselves instead of trusting whatever the traversal
    // last left in the GL clip plane units.  The world plane is passed while
    // the modelview is the view matrix, which is exactly the convention
    // SoGLClipPlaneElement relies on, so the equations match world-space geometry.
    for (int j = 0; j < nplanes; ++j) {
        const SbPlane plane = elt->get(j, TRUE);
        const SbVec3f normal = plane.getNormal();
        const GLdouble equation[4] = {
            normal[0],
            normal[1],
            normal[2],
            -plane.getDistanceFromOrigin(),
        };
        glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0 + j), equation);
    }

    bool drewCap = false;
    glClearStencil(0);
    glStencilMask(0x1);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    GLubyte stipple[128];
    for (int p = 0; p < nplanes; ++p) {
        const SbPlane plane = elt->get(p, TRUE);
        const SbVec3f normal = plane.getNormal();
        SbVec3f u = std::fabs(normal[0]) < 0.9f ? SbVec3f(1, 0, 0) : SbVec3f(0, 1, 0);
        u = u - normal * normal.dot(u);
        u.normalize();
        SbVec3f v = normal.cross(u);
        v.normalize();

        for (const Group& group : groups) {
            SectionCapRecord rec;
            rec.key = group.key;
            rec.component = group.component;
            rec.plane = p;
            if (group.transparent) {
                rec.status = "unsupported: transparent";
                records.push_back(rec);
                continue;
            }
            if (!group.closed) {
                rec.status = "unsupported: open mesh";
                records.push_back(rec);
                continue;
            }
            if (!planeCutsBox(plane, group.box)) {
                rec.status = "skipped: not cut";
                records.push_back(rec);
                continue;
            }
            rec.style = styleFor(group);

            int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            const SbVec2s vo = region.getViewportOriginPixels();
            const SbVec2s vs = region.getViewportSizePixels();
            if (!screenRect(group.box, viewProj, region, x0, y0, x1, y1)) {
                x0 = vo[0];
                y0 = vo[1];
                x1 = vo[0] + vs[0];
                y1 = vo[1] + vs[1];
            }
            glEnable(GL_SCISSOR_TEST);
            glScissor(x0, y0, x1 - x0, y1 - y0);

            // 1. mark: only plane p clips
            for (int j = 0; j < nplanes; ++j) {
                if (j == p) {
                    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
                else {
                    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
            }
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glStencilMask(0x1);
            glStencilFunc(GL_ALWAYS, 0, 0x1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glBegin(GL_TRIANGLES);
            for (std::size_t i = 0; i + 2 < group.tris.size(); i += 3) {
                glVertex3f(group.tris[i], group.tris[i + 1], group.tris[i + 2]);
            }
            glEnd();

            // 2. fill: plane p off, every other plane on
            for (int j = 0; j < nplanes; ++j) {
                if (j == p) {
                    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
                else {
                    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
            }
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glStencilFunc(GL_EQUAL, 1, 0x1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            SbVec3f center = group.box.getCenter();
            center = center - normal * (normal.dot(center) - plane.getDistanceFromOrigin());
            float dx {}, dy {}, dz {};
            group.box.getSize(dx, dy, dz);
            const float half = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz) * 1.05f + 1e-3f;
            const SbVec3f corners[4] = {
                center + (u + v) * half,
                center + (-u + v) * half,
                center + (-u - v) * half,
                center + (u - v) * half,
            };
            auto quad = [&]() {
                glBegin(GL_QUADS);
                for (const SbVec3f& c : corners) {
                    glVertex3f(c[0], c[1], c[2]);
                }
                glEnd();
            };
            glColor3f(rec.style.color[0], rec.style.color[1], rec.style.color[2]);
            quad();
            if (stipplePattern(rec.style.hatch, stipple)) {
                glEnable(GL_POLYGON_STIPPLE);
                glPolygonStipple(stipple);
                glColor3f(rec.style.hatchColor[0], rec.style.hatchColor[1], rec.style.hatchColor[2]);
                quad();
                glDisable(GL_POLYGON_STIPPLE);
            }

            // 3. clear the bit inside this group's rectangle
            glStencilMask(0x1);
            glClear(GL_STENCIL_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            rec.status = "capped";
            drewCap = true;
            records.push_back(rec);
        }
    }

    glPopMatrix();
    glPopAttrib();
    status = drewCap ? "capped" : "no-cap";
    rendering = false;
}

std::string SoFCSectionCap::keyForPath(SoCallbackAction* action, std::string& component)
{
    // view providers along the path, with the child indices between them (not of switches)
    const auto* path = static_cast<const SoFullPath*>(action->getCurPath());
    std::vector<std::string> names;
    std::vector<std::string> pending;
    std::string key;
    component.clear();
    lastExcluded = false;
    for (int i = 1; i < path->getLength(); ++i) {
        SoNode* node = path->getNode(i);
        std::string name = resolve ? resolve(node) : std::string();
        if (!name.empty()) {
            // "Doc#Link\tDoc#Target": the component is the link target
            std::string target = name;
            const auto tab = name.find('\t');
            if (tab != std::string::npos) {
                target = name.substr(tab + 1);
                name = name.substr(0, tab);
            }
            if (!key.empty()) {
                for (const std::string& idx : pending) {
                    key += "/" + idx;
                }
                key += "/";
            }
            key += name;
            component = target;
            if (exclude && exclude(node)) {
                lastExcluded = true;
            }
            pending.clear();
            continue;
        }
        if (!key.empty()) {
            SoNode* parent = path->getNode(i - 1);
            if (!parent->isOfType(SoSwitch::getClassTypeId())) {
                pending.push_back(std::to_string(path->getIndex(i)));
            }
        }
    }
    // trailing indices tell the elements of a link array apart
    for (const std::string& idx : pending) {
        key += "/" + idx;
    }
    if (key.empty()) {
        key = "(unresolved)";
        component = key;
    }
    return key;
}

std::uint64_t SoFCSectionCap::geometrySignature(SoNode* root)
{
    // Highlighting touches indexed shapes (and so every group above them) on
    // each hover; use leaf node ids and group structure instead.
    std::uint64_t h = 0;
    auto mix = [&h](std::uint64_t v) {
        h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };
    std::vector<SoNode*> stack {root};
    while (!stack.empty()) {
        SoNode* node = stack.back();
        stack.pop_back();
        mix(reinterpret_cast<std::uintptr_t>(node));
        if (node->isOfType(SoPointSet::getClassTypeId())
            || node->isOfType(SoLineSet::getClassTypeId())
            || node->isOfType(SoMarkerSet::getClassTypeId())) {
            continue;  // no triangles; highlighted vertices / edges are touched on hover
        }
        if (node->isOfType(SoIndexedShape::getClassTypeId())) {
            mix(static_cast<std::uint64_t>(static_cast<SoIndexedShape*>(node)->coordIndex.getNum()));
            continue;
        }
        SoChildList* children = node->getChildren();
        if (!children) {
            mix(node->getNodeId());  // a leaf: coordinates, transform, material, ...
            continue;
        }
        mix(static_cast<std::uint64_t>(children->getLength()));
        if (node->isOfType(SoSwitch::getClassTypeId())) {
            mix(static_cast<std::uint64_t>(static_cast<SoSwitch*>(node)->whichChild.getValue()));
        }
        for (int i = children->getLength() - 1; i >= 0; --i) {
            stack.push_back((*children)[i]);
        }
    }
    return h;
}

void SoFCSectionCap::updateProxy()
{
    // nothing below the scene changed
    const std::uint32_t sceneId = scene->getNodeId();
    if (proxyValid && sceneId == proxySceneId) {
        return;
    }
    proxySceneId = sceneId;
    const std::uint64_t signature = geometrySignature(scene);
    if (proxyValid && signature == proxySignature) {
        return;
    }
    groups.clear();
    groupIndex.clear();
    lastShape = nullptr;
    SoCallbackAction collect;
    collect.addPreCallback(
        SoShape::getClassTypeId(),
        [](void* self, SoCallbackAction* action, const SoNode* node) -> SoCallbackAction::Response {
            auto* cap = static_cast<SoFCSectionCap*>(self);
            std::string component;
            const std::string key = cap->keyForPath(action, component);
            if (cap->lastExcluded) {
                cap->lastShape = nullptr;
                return SoCallbackAction::PRUNE;
            }
            auto it = cap->groupIndex.find(key);
            if (it == cap->groupIndex.end()) {
                Group group;
                group.key = key;
                group.component = component;
                cap->groups.push_back(group);
                it = cap->groupIndex.emplace(key, cap->groups.size() - 1).first;
            }
            cap->lastGroup = it->second;
            cap->lastShape = node;
            SbColor ambient, diffuse, specular, emission;
            float shininess = 0.0f, transparency = 0.0f;
            action->getMaterial(ambient, diffuse, specular, emission, shininess, transparency, 0);
            if (transparency > 0.01f) {
                cap->groups[it->second].transparent = true;
            }
            return SoCallbackAction::CONTINUE;
        },
        this
    );
    collect.addTriangleCallback(SoShape::getClassTypeId(), &SoFCSectionCap::collectTriangle, this);
    collect.apply(scene);
    // drop groups with lines and points only
    groups.erase(
        std::remove_if(groups.begin(), groups.end(), [](const Group& g) { return g.tris.empty(); }),
        groups.end()
    );
    groupIndex.clear();
    for (std::size_t i = 0; i < groups.size(); ++i) {
        Group& group = groups[i];
        groupIndex[group.key] = i;
        // the closed check is costly; reuse it while the triangles are unchanged
        std::uint64_t digestTris = 14695981039346656037ULL;
        for (float f : group.tris) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &f, sizeof(bits));
            digestTris = (digestTris ^ bits) * 1099511628211ULL;
        }
        auto cached = closedCache.find(group.key);
        if (cached != closedCache.end() && cached->second.first == digestTris) {
            group.closed = cached->second.second;
        }
        else {
            checkClosed(group);
            closedCache[group.key] = {digestTris, group.closed};
        }
    }
    for (auto it = closedCache.begin(); it != closedCache.end();) {
        it = groupIndex.count(it->first) ? std::next(it) : closedCache.erase(it);
    }
    proxySignature = signature;
    proxyValid = true;
}

void SoFCSectionCap::collectTriangle(
    void* self,
    SoCallbackAction* action,
    const SoPrimitiveVertex* v1,
    const SoPrimitiveVertex* v2,
    const SoPrimitiveVertex* v3
)
{
    auto* cap = static_cast<SoFCSectionCap*>(self);
    if (!cap->lastShape) {
        return;
    }
    Group& group = cap->groups[cap->lastGroup];
    const SbMatrix& model = action->getModelMatrix();
    for (const SoPrimitiveVertex* v : {v1, v2, v3}) {
        SbVec3f world;
        model.multVecMatrix(v->getPoint(), world);
        group.tris.push_back(world[0]);
        group.tris.push_back(world[1]);
        group.tris.push_back(world[2]);
        group.box.extendBy(world);
    }
}

void SoFCSectionCap::checkClosed(Group& group)
{
    // closed: (nearly) every edge is used an even number of times, vertices welded by position
    std::map<std::tuple<float, float, float>, int> ids;
    auto id = [&](std::size_t i) {
        auto key = std::make_tuple(group.tris[i], group.tris[i + 1], group.tris[i + 2]);
        auto it = ids.find(key);
        if (it == ids.end()) {
            it = ids.emplace(key, static_cast<int>(ids.size())).first;
        }
        return it->second;
    };
    std::map<std::pair<int, int>, int> edges;
    for (std::size_t t = 0; t + 8 < group.tris.size(); t += 9) {
        const int a = id(t), b = id(t + 3), c = id(t + 6);
        for (auto e : {std::make_pair(a, b), std::make_pair(b, c), std::make_pair(c, a)}) {
            if (e.first == e.second) {
                continue;  // degenerate triangle
            }
            if (e.first > e.second) {
                std::swap(e.first, e.second);
            }
            ++edges[e];
        }
    }
    // tessellation can leave a few cracks where faces meet; an open surface has far more
    std::size_t odd = 0;
    for (const auto& kv : edges) {
        odd += kv.second % 2;
    }
    group.closed = !edges.empty() && odd * 100 <= edges.size();
}
