// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "BoxSelection.h"

#include <memory>
#include <vector>

#include <CXX/Objects.hxx>
#include <Inventor/nodes/SoCamera.h>

#include <App/Application.h>
#include <App/ComplexGeoDataPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Interpreter.h>
#include <Base/Tools2D.h>

#include "Application.h"
#include "Selection.h"
#include "Utilities.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;

namespace
{

using SelectionMode = enum
{
    CENTER,
    INTERSECT
};

static bool findObjectsOfTypeInBox(
    const std::string& type,
    const Base::ViewProjMethod& proj,
    Data::ComplexGeoData* data,
    const Base::Polygon2d& polygon,
    std::vector<std::string>& ret,
    SelectionMode mode
)
{
    size_t count = data->countSubElements(type.c_str());
    if (!count) {
        return false;
    }

    bool foundElement = false;
    for (size_t i = 1; i <= count; ++i) {
        std::string element(type);
        element += std::to_string(i);
        std::unique_ptr<Data::Segment> segment(data->getSubElementByName(element.c_str()));
        if (!segment) {
            continue;
        }

        if (type == "Vertex") {
            Base::Vector3d point;
            if (data->getFirstVertexFromSubElement(segment.get(), point)) {
                auto v = proj(point);
                if (polygon.Contains(Base::Vector2d(v.x, v.y))) {
                    foundElement = true;
                    ret.push_back(element);
                    continue;
                }
            }
        }
        else {
            std::vector<Base::Vector3d> points;
            std::vector<Data::ComplexGeoData::Line> lines;
            data->getLinesFromSubElement(segment.get(), points, lines);
            if (points.empty() || lines.empty()) {
                continue;
            }

            Base::Polygon2d loop;
            // TODO: can we assume the line returned above are in proper
            // order if the element is a face?
            auto v = proj(points[lines.front().I1]);
            loop.Add(Base::Vector2d(v.x, v.y));
            for (auto& line : lines) {
                for (auto i = line.I1; i < line.I2; ++i) {
                    auto projected = proj(points[i + 1]);
                    loop.Add(Base::Vector2d(projected.x, projected.y));
                }
            }

            if (!polygon.Intersect(loop)) {
                continue;
            }
            if (mode == CENTER && !polygon.Contains(loop.CalcBoundBox().GetCenter())) {
                continue;
            }
            ret.push_back(element);
            foundElement = true;
        }
    }

    return foundElement;
}

/**
 * @brief Collect box-selection matches for one view provider.
 *
 * Projects visible geometry into the view and tests it against the current
 * box or fence polygon.
 *
 * @param[in] vp View provider to query.
 * @param[in] mode Center or intersect selection mode.
 * @param[in] selectElement When true, collect subelements instead of whole objects.
 * @param[in] proj Projection function for 3D points.
 * @param[in] polygon Selection polygon in projected coordinates.
 * @param[in] mat Accumulated transformation matrix.
 * @param[in] transform Whether to apply object transforms while resolving geometry.
 * @param[in] depth Current recursion depth when walking subobjects.
 * @return Matching subelement names, or an empty-string entry for whole-object matches.
 */
std::vector<std::string> getBoxSelection(
    ViewProviderDocumentObject* vp,
    SelectionMode mode,
    bool selectElement,
    const Base::ViewProjMethod& proj,
    const Base::Polygon2d& polygon,
    const Base::Matrix4D& mat,
    bool transform = true,
    int depth = 0
)
{
    std::vector<std::string> ret;
    auto obj = vp->getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return ret;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    const auto selectionGate = SelectionSingleton::instance().getSelectionGate(doc);

    // DO NOT check this view object Visibility, let the caller do this. Because
    // we may be called by upper object hierarchy that manages our visibility.
    auto bbox3 = vp->getBoundingBox(nullptr, transform);
    Base::BoundBox2d bbox;
    const bool isBBox3Valid = bbox3.IsValid();
    if (isBBox3Valid && selectionGate == nullptr) {
        bbox = bbox3.Transformed(mat).ProjectBox(&proj);

        // check if both two boundary points are inside polygon, only
        // valid since we know the given polygon is a box.
        if (polygon.Contains(Base::Vector2d(bbox.MinX, bbox.MinY))
            && polygon.Contains(Base::Vector2d(bbox.MaxX, bbox.MaxY))) {
            ret.emplace_back("");
            return ret;
        }

        if (!bbox.Intersect(polygon)) {
            return ret;
        }
    }

    const auto& subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
    if (subs.empty()) {
        if (!selectElement) {
            if (mode == INTERSECT || (isBBox3Valid && polygon.Contains(bbox.GetCenter()))) {
                ret.emplace_back("");
            }
            return ret;
        }

        Base::PyGILStateLocker lock;
        PyObject* pyobj = nullptr;
        Base::Matrix4D matCopy(mat);
        obj->getSubObject(nullptr, &pyobj, &matCopy, transform, depth);
        if (!pyobj) {
            return ret;
        }

        Py::Object pyobject(pyobj, true);
        if (!PyObject_TypeCheck(pyobj, &Data::ComplexGeoDataPy::Type)) {
            return ret;
        }

        auto data = static_cast<Data::ComplexGeoDataPy*>(pyobj)->getComplexGeoDataPtr();
        const auto& allAllowedDocumentTypes = data->getElementTypes();

        if (selectionGate) {
            auto filteredTypes = selectionGate->getGatedTypes(allAllowedDocumentTypes);
            if (!filteredTypes.empty()) {
                for (const auto& type : filteredTypes) {
                    findObjectsOfTypeInBox(type, proj, data, polygon, ret, mode);
                }
                return ret;
            }
        }

        for (auto type : allAllowedDocumentTypes) {
            if (findObjectsOfTypeInBox(type, proj, data, polygon, ret, mode)) {
                break;
            }
        }
        return ret;
    }

    size_t count = 0;
    for (auto& sub : subs) {
        App::DocumentObject* parent = nullptr;
        std::string childName;
        Base::Matrix4D smat(mat);
        auto sobj
            = obj->resolve(sub.c_str(), &parent, &childName, nullptr, nullptr, &smat, transform, depth + 1);
        if (!sobj) {
            continue;
        }

        int vis;
        if (!parent || (vis = parent->isElementVisible(childName.c_str())) < 0) {
            vis = sobj->Visibility.getValue() ? 1 : 0;
        }
        if (!vis) {
            continue;
        }

        auto svp = freecad_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(sobj)
        );
        if (!svp) {
            continue;
        }

        const auto& sels
            = getBoxSelection(svp, mode, selectElement, proj, polygon, smat, false, depth + 1);
        if (sels.size() == 1 && sels[0].empty()) {
            ++count;
        }
        for (auto& sel : sels) {
            ret.emplace_back(sub + sel);
        }
    }

    if (count == subs.size()) {
        ret.resize(1);
        ret[0].clear();
    }
    return ret;
}

}  // namespace

void Gui::applyBoxSelection(
    View3DInventorViewer* viewer,
    const std::vector<SbVec2s>& picked,
    bool selectElement,
    bool additive
)
{
    if (!viewer || picked.size() < 2) {
        return;
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }

    SelectionMode selectionMode = CENTER;
    std::vector<SbVec2f> glPolygon = viewer->getGLPolygon(picked);

    Base::Polygon2d polygon;
    if (glPolygon.size() == 2) {
        const auto& pt1 = glPolygon[0];
        const auto& pt2 = glPolygon[1];
        polygon.Add(Base::Vector2d(pt1[0], pt1[1]));
        polygon.Add(Base::Vector2d(pt1[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt1[1]));

        // when selecting from right to left then select by intersection
        // otherwise if the center is inside the rectangle
        if (pt1[0] > pt2[0]) {
            selectionMode = INTERSECT;
        }
    }
    else {
        for (const auto& point : glPolygon) {
            polygon.Add(Base::Vector2d(point[0], point[1]));
        }
    }

    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    if (!additive) {
        Gui::Selection().clearSelection(doc->getName());
    }

    Gui::ViewVolumeProjection proj(cam->getViewVolume());

    const std::vector<App::DocumentObject*> objects = doc->getObjects();
    for (auto* obj : objects) {
        if (App::GeoFeatureGroupExtension::getGroupOfObject(obj)) {
            continue;
        }

        auto vp = freecad_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(obj)
        );
        if (!vp || !vp->isVisible()) {
            continue;
        }

        Base::Matrix4D mat;
        for (auto& sub : getBoxSelection(vp, selectionMode, selectElement, proj, polygon, mat)) {
            Gui::Selection().addSelection(doc->getName(), obj->getNameInDocument(), sub.c_str());
        }
    }
}
