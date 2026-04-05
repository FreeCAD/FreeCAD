// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2019 Zheng Lei <realthunder.dev@gmail.com>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/nodes/SoCamera.h>
#endif

#include "BoxSelection.h"
#include <Base/Interpreter.h>
#include <App/ComplexGeoDataPy.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Gui/Application.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderDocumentObject.h>

using namespace Gui;

BoxSelection::BoxSelection(App::Document* doc, Gui::View3DInventorViewer* viewer)
    : doc {doc}
    , viewer {viewer}
{}

void BoxSelection::perform(bool selectElement)
{
    SelectionMode selectionMode = CENTER;

    std::vector<SbVec2f> picked = viewer->getGLPolygon();
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    if (picked.size() == 2) {
        SbVec2f pt1 = picked[0];
        SbVec2f pt2 = picked[1];
        polygon.Add(Base::Vector2d(pt1[0], pt1[1]));
        polygon.Add(Base::Vector2d(pt1[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt2[1]));
        polygon.Add(Base::Vector2d(pt2[0], pt1[1]));

        // when selecting from right to left then select by intersection
        // otherwise if the center is inside the rectangle
        if (picked[0][0] > picked[1][0]) {
            selectionMode = INTERSECT;
        }
    }
    else {
        for (const auto& it : picked) {
            polygon.Add(Base::Vector2d(it[0], it[1]));
        }
    }

    const std::vector<App::DocumentObject*> objects = doc->getObjects();
    for (auto obj : objects) {
        if (App::GeoFeatureGroupExtension::getGroupOfObject(obj)) {
            continue;
        }

        auto vp = dynamic_cast<ViewProviderDocumentObject*>(
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

std::vector<std::string> BoxSelection::getBoxSelection(
    ViewProviderDocumentObject* vp,
    SelectionMode mode,
    bool selectElement,
    const Base::ViewProjMethod& proj,
    const Base::Polygon2d& polygon,
    const Base::Matrix4D& mat,
    bool transform,
    int depth
)
{
    std::vector<std::string> ret;
    auto obj = vp->getObject();
    if (!obj || !obj->isAttachedToDocument()) {
        return ret;
    }

    // DO NOT check this view object Visibility, let the caller do this. Because
    // we may be called by upper object hierarchy that manages our visibility.

    auto bbox3 = vp->getBoundingBox(nullptr, transform);
    if (!bbox3.IsValid()) {
        return ret;
    }

    auto bbox = bbox3.Transformed(mat).ProjectBox(&proj);

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


    const auto& subs = obj->getSubObjects(App::DocumentObject::GS_SELECT);
    if (subs.empty()) {
        if (!selectElement) {
            if (mode == INTERSECT || polygon.Contains(bbox.GetCenter())) {
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
        for (auto type : data->getElementTypes()) {
            size_t count = data->countSubElements(type);
            if (!count) {
                continue;
            }
            for (size_t i = 1; i <= count; ++i) {
                std::string element(type);
                element += std::to_string(i);
                std::unique_ptr<Data::Segment> segment(data->getSubElementByName(element.c_str()));
                if (!segment) {
                    continue;
                }

                std::vector<Base::Vector3d> points;
                std::vector<Data::ComplexGeoData::Line> lines;
                data->getLinesFromSubElement(segment.get(), points, lines);

                if (lines.empty()) {
                    if (points.empty()) {
                        continue;
                    }

                    auto v = proj(points[0]);
                    if (polygon.Contains(Base::Vector2d(v.x, v.y))) {
                        ret.push_back(element);
                    }
                    continue;
                }

                Base::Polygon2d loop;
                // TODO: can we assume the line returned above are in proper
                // order if the element is a face?
                auto v = proj(points[lines.front().I1]);
                loop.Add(Base::Vector2d(v.x, v.y));
                for (auto& line : lines) {
                    for (auto i = line.I1; i < line.I2; ++i) {
                        auto v = proj(points[i + 1]);
                        loop.Add(Base::Vector2d(v.x, v.y));
                    }
                }

                if (!polygon.Intersect(loop)) {
                    continue;
                }

                if (mode == CENTER && !polygon.Contains(loop.CalcBoundBox().GetCenter())) {
                    continue;
                }
                ret.push_back(element);
            }
            break;
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

        int vis = 0;
        if (!parent || (vis = parent->isElementVisible(childName.c_str())) < 0) {
            vis = sobj->Visibility.getValue() ? 1 : 0;
        }

        if (vis == 0) {
            continue;
        }

        auto svp = dynamic_cast<ViewProviderDocumentObject*>(
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
