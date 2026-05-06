// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak <furkan1795@gmail.com>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <Base/Tools2D.h>
#include <Base/Vector3D.h>

class SoCoordinate3;
class SoDrawStyle;
class SoLineSet;
class SoMarkerSet;
class SoMaterial;
class SoSeparator;

namespace App
{
class DocumentObject;
}

namespace Part
{
class Geometry;
}

namespace Sketcher
{
class SketchObject;
}

namespace SketcherGui
{

struct ExternalGeometryMatch
{
    int index = -1;
    bool matchesType = false;

    bool isValid() const
    {
        return index >= 0;
    }
};

ExternalGeometryMatch findExternalMatch(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
);
int resolveExternalGeometryIndex(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection,
    int fallbackIndex
);

struct DrawingParameters;

class LazyExternalGeometryLayer
{
public:
    enum class ElementType
    {
        Edge,
        Vertex
    };

    struct Element
    {
        Element() = default;
        Element(const Element&) = delete;
        Element& operator=(const Element&) = delete;
        Element(Element&&) noexcept = default;
        Element& operator=(Element&&) noexcept = default;

        int id = -1;
        std::string sourceObjectName;
        std::string subName;
        ElementType type = ElementType::Edge;
        bool intersection = false;
        std::vector<std::unique_ptr<Part::Geometry>> geometry;
    };

    struct HitResult
    {
        int id = -1;
        bool isVertex = false;
        double distance = 0.0;

        bool isValid() const
        {
            return id >= 0;
        }
    };

    LazyExternalGeometryLayer();
    ~LazyExternalGeometryLayer();

    LazyExternalGeometryLayer(const LazyExternalGeometryLayer&) = delete;
    LazyExternalGeometryLayer& operator=(const LazyExternalGeometryLayer&) = delete;

    void attachTo(SoSeparator* editRoot);
    void detach();

    void clear();
    void rebuildSources(Sketcher::SketchObject* sketch);
    bool syncSources(Sketcher::SketchObject* sketch);

    const Element* getElement(int id) const;
    const Part::Geometry* getPreviewGeometry(int id) const;
    bool isElementVertex(int id) const;
    bool isElementSelected(int id) const;
    std::vector<int> getSelectedElementIds() const;

    void setPreselectedElement(int id);
    void clearPreselectedElement();
    void setSelectedElement(int id, bool selected);
    void clearSelectedElements();

    void setEnabled(bool enabled);
    bool isEnabled() const;
    HitResult findClosestElement(
        Sketcher::SketchObject* sketch,
        const Base::Vector2d& point,
        double maxDistance
    );

    void draw(const DrawingParameters& parameters, int viewOrientationFactor);

private:
    struct Source
    {
        App::DocumentObject* object = nullptr;
        int edgeCount = 0;
        int vertexCount = 0;
    };

    int addSourceReference(
        Sketcher::SketchObject* sketch,
        App::DocumentObject* sourceObject,
        const std::string& subName,
        bool intersection,
        bool buildPreview = true
    );

    Element* getElement(int id);
    const Element* findElementBySource(
        App::DocumentObject* sourceObject,
        const std::string& subName,
        bool intersection
    ) const;

    static std::vector<Source> collectSources(Sketcher::SketchObject* sketch);
    static std::vector<std::string> getSourceNames(const std::vector<Source>& sources);

    void createNodes();
    void resetNodes();
    void rebuildFromSourceObjects(
        Sketcher::SketchObject* sketch,
        const std::vector<Source>& sources,
        std::vector<std::string> sourceNames
    );
    bool ensurePreview(Sketcher::SketchObject* sketch, Element& element);
    void appendElementToCoin(
        const Element& element,
        std::vector<Base::Vector3d>& points,
        std::vector<Base::Vector3d>& curveCoords,
        std::vector<int32_t>& curveVertexCounts
    );
    bool shouldDraw(const Element& element) const;

private:
    SoSeparator* parentRoot = nullptr;
    SoSeparator* root = nullptr;
    SoMaterial* highlightMaterial = nullptr;
    SoCoordinate3* highlightPointCoordinates = nullptr;
    SoCoordinate3* highlightCurveCoordinates = nullptr;
    SoDrawStyle* highlightPointDrawStyle = nullptr;
    SoDrawStyle* highlightCurveDrawStyle = nullptr;
    SoMarkerSet* highlightPointSet = nullptr;
    SoLineSet* highlightCurveSet = nullptr;

    std::vector<Element> elements;
    std::vector<std::string> sourceObjectNames;
    int preselectedElementId = -1;
    std::set<int> selectedElementIds;
    bool enabled = true;
    int nextId = 1;
};

}  // namespace SketcherGui
