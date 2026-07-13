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

#include <FCConfig.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Precision.hxx>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/GroupExtension.h>
#include <App/IndexedName.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditModeCoinManagerParameters.h"
#include "LazyExternalGeometryLayer.h"

namespace
{
constexpr int DisplayCurveMaxSubdivisionDepth = 8;
constexpr double DisplayCurveRelativeDeviation = 0.01;

bool isIndexedSubNameOfType(const std::string& subName, const char* type)
{
    const char* lastPart = std::strrchr(subName.c_str(), '.');
    const Data::IndexedName indexedSubName(lastPart ? lastPart + 1 : subName.c_str());
    return indexedSubName && indexedSubName.getIndex() > 0
        && std::strcmp(indexedSubName.getType(), type) == 0;
}

bool isEdgeSubName(const std::string& subName)
{
    return isIndexedSubNameOfType(subName, "Edge");
}

bool isVertexSubName(const std::string& subName)
{
    return isIndexedSubNameOfType(subName, "Vertex");
}

bool isSourceVisible(App::DocumentObject* object)
{
    if (!Gui::Application::Instance || !object) {
        return false;
    }

    for (auto* current = object; current;) {
        auto* viewProvider = Gui::Application::Instance->getViewProvider(current);
        if (!viewProvider || !viewProvider->isVisible()) {
            return false;
        }

        auto* owner = App::GeoFeatureGroupExtension::getGroupOfObject(current);
        if (!owner || owner == current) {
            owner = App::GroupExtension::getGroupOfObject(current);
        }

        if (!owner || owner == current) {
            break;
        }

        current = owner;
    }

    return true;
}

bool isValidExternalSource(App::DocumentObject* object)
{
    return object && !Base::Tools::isNullOrEmpty(object->getNameInDocument())
        && !object->isDerivedFrom<Sketcher::SketchObject>() && isSourceVisible(object);
}

template<typename Elements>
auto findElementById(Elements& elements, int id)
{
    auto it = std::find_if(elements.begin(), elements.end(), [id](const auto& element) {
        return element.id == id;
    });

    return it == elements.end() ? nullptr : &(*it);
}

Base::Vector2d toVector2d(const Base::Vector3d& point)
{
    return {point.x, point.y};
}

double distance2d(const Base::Vector2d& a, const Base::Vector2d& b)
{
    return (a - b).Length();
}

double distancePointToSegment2d(const Base::Vector2d& p, const Base::Vector2d& a, const Base::Vector2d& b)
{
    const Base::Vector2d ab = b - a;
    const Base::Vector2d ap = p - a;
    const double ab2 = ab.x * ab.x + ab.y * ab.y;

    if (ab2 <= std::numeric_limits<double>::epsilon()) {
        return distance2d(p, a);
    }

    const double t = std::clamp((ap.x * ab.x + ap.y * ab.y) / ab2, 0.0, 1.0);
    return distance2d(p, a + ab * t);
}

bool getCurveParameterRange(const Part::GeomCurve& curve, double& first, double& last)
{
    first = curve.getFirstParameter();
    last = curve.getLastParameter();
    if (last < first) {
        std::swap(first, last);
    }

    return std::isfinite(first) && std::isfinite(last)
        && std::abs(last - first) > std::numeric_limits<double>::epsilon();
}

void appendAdaptiveCurveSegment(
    const Part::GeomCurve& curve,
    double first,
    double last,
    Base::Vector3d firstPoint,
    Base::Vector3d lastPoint,
    int depth,
    std::vector<Base::Vector3d>& curveCoords
)
{
    const double middle = 0.5 * (first + last);
    const Base::Vector3d middlePoint = curve.value(middle);
    const double chordLength = distance2d(toVector2d(firstPoint), toVector2d(lastPoint));
    const double tolerance
        = std::max(Precision::Confusion(), chordLength * DisplayCurveRelativeDeviation);
    const double deviation = distancePointToSegment2d(
        toVector2d(middlePoint),
        toVector2d(firstPoint),
        toVector2d(lastPoint)
    );

    if (depth >= DisplayCurveMaxSubdivisionDepth || deviation <= tolerance) {
        curveCoords.push_back(lastPoint);
        return;
    }

    appendAdaptiveCurveSegment(curve, first, middle, firstPoint, middlePoint, depth + 1, curveCoords);
    appendAdaptiveCurveSegment(curve, middle, last, middlePoint, lastPoint, depth + 1, curveCoords);
}

void appendCurveToCoin(
    const Part::Geometry* geometry,
    std::vector<Base::Vector3d>& curveCoords,
    std::vector<int32_t>& curveVertexCounts
)
{
    if (auto line = freecad_cast<const Part::GeomLineSegment*>(geometry)) {
        curveCoords.push_back(line->getStartPoint());
        curveCoords.push_back(line->getEndPoint());
        curveVertexCounts.push_back(2);
        return;
    }

    if (auto curve = freecad_cast<const Part::GeomCurve*>(geometry)) {
        double first = 0.0;
        double last = 0.0;
        if (!getCurveParameterRange(*curve, first, last)) {
            return;
        }

        const std::size_t firstVertexIndex = curveCoords.size();
        const Base::Vector3d firstPoint = curve->value(first);
        const Base::Vector3d lastPoint = curve->value(last);
        curveCoords.push_back(firstPoint);
        appendAdaptiveCurveSegment(*curve, first, last, firstPoint, lastPoint, 0, curveCoords);
        curveVertexCounts.push_back(static_cast<int32_t>(curveCoords.size() - firstVertexIndex));
    }
}

bool matchesExternalType(long type, bool intersection)
{
    return type == static_cast<long>(Sketcher::ExtType::Both)
        || (type == static_cast<long>(Sketcher::ExtType::Projection) && !intersection)
        || (type == static_cast<long>(Sketcher::ExtType::Intersection) && intersection);
}
}  // namespace

namespace SketcherGui
{

ExternalGeometryMatch findExternalMatch(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
)
{
    if (!sketch || !sourceObject) {
        return {};
    }

    const auto objects = sketch->ExternalGeometry.getValues();
    const auto subElements = sketch->ExternalGeometry.getSubValues();
    auto types = sketch->ExternalTypes.getValues();
    types.resize(objects.size(), static_cast<long>(Sketcher::ExtType::Projection));

    ExternalGeometryMatch firstMatch;
    for (std::size_t i = 0; i < objects.size() && i < subElements.size(); ++i) {
        if (objects[i] != sourceObject || subElements[i] != subName) {
            continue;
        }

        ExternalGeometryMatch match {static_cast<int>(i), matchesExternalType(types[i], intersection)};
        if (match.matchesType) {
            return match;
        }
        if (!firstMatch.isValid()) {
            firstMatch = match;
        }
    }

    return firstMatch;
}

int resolveExternalGeometryIndex(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
)
{
    const auto match = findExternalMatch(sketch, sourceObject, subName, intersection);
    return match.isValid() && match.matchesType ? match.index : -1;
}


LazyExternalGeometryLayer::LazyExternalGeometryLayer() = default;

LazyExternalGeometryLayer::~LazyExternalGeometryLayer()
{
    detach();
}

void LazyExternalGeometryLayer::attachTo(SoSeparator* editRoot)
{
    if (parentRoot == editRoot && root) {
        return;
    }

    detach();
    parentRoot = editRoot;
    createNodes();

    if (parentRoot && root) {
        parentRoot->addChild(root);
    }
}

void LazyExternalGeometryLayer::detach()
{
    if (parentRoot && root) {
        parentRoot->removeChild(root);
    }

    parentRoot = nullptr;
    resetNodes();
}

void LazyExternalGeometryLayer::resetNodes()
{
    if (root) {
        root->unref();
    }

    root = nullptr;
    highlightMaterial = nullptr;
    highlightPointCoordinates = nullptr;
    highlightCurveCoordinates = nullptr;
    highlightPointDrawStyle = nullptr;
    highlightCurveDrawStyle = nullptr;
    highlightPointSet = nullptr;
    highlightCurveSet = nullptr;
}

void LazyExternalGeometryLayer::createNodes()
{
    root = new SoSeparator;
    root->ref();
    root->setName("Sketch_LazyExternalGeometryRoot");
    root->renderCaching = SoSeparator::OFF;

    auto* pickStyle = new SoPickStyle;
    pickStyle->style = SoPickStyle::UNPICKABLE;
    root->addChild(pickStyle);

    highlightMaterial = new SoMaterial;
    highlightMaterial->setName("LazyExternalGeometryHighlightMaterial");
    root->addChild(highlightMaterial);

    highlightPointCoordinates = new SoCoordinate3;
    highlightPointCoordinates->setName("LazyExternalGeometryHighlightPointCoordinates");
    root->addChild(highlightPointCoordinates);

    highlightPointDrawStyle = new SoDrawStyle;
    highlightPointDrawStyle->setName("LazyExternalGeometryHighlightPointDrawStyle");
    root->addChild(highlightPointDrawStyle);

    highlightPointSet = new SoMarkerSet;
    highlightPointSet->setName("LazyExternalGeometryHighlightPointSet");
    root->addChild(highlightPointSet);

    highlightCurveCoordinates = new SoCoordinate3;
    highlightCurveCoordinates->setName("LazyExternalGeometryHighlightCurveCoordinates");
    root->addChild(highlightCurveCoordinates);

    highlightCurveDrawStyle = new SoDrawStyle;
    highlightCurveDrawStyle->setName("LazyExternalGeometryHighlightCurveDrawStyle");
    root->addChild(highlightCurveDrawStyle);

    highlightCurveSet = new SoLineSet;
    highlightCurveSet->setName("LazyExternalGeometryHighlightCurveSet");
    root->addChild(highlightCurveSet);
}

int LazyExternalGeometryLayer::preselectSourceReference(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
)
{
    if (!enabled) {
        clearPreselectedElement();
        return -1;
    }

    if (const Element* existing = findElementBySource(sourceObject, subName, intersection)) {
        preselectedElementId = existing->id;
        return existing->id;
    }

    clearPreselectedElement();

    const int id = addSourceReference(sketch, sourceObject, subName, intersection);
    if (id < 0) {
        return -1;
    }

    preselectedElementId = id;
    return id;
}

int LazyExternalGeometryLayer::addSourceReference(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
)
{
    if (!sketch || !isValidExternalSource(sourceObject)) {
        return -1;
    }

    if (!sketch->isExternalAllowed(sourceObject->getDocument(), sourceObject)) {
        return -1;
    }

    const bool isEdge = isEdgeSubName(subName);
    const bool isVertex = isVertexSubName(subName);
    if (!isEdge && !isVertex) {
        return -1;
    }

    const auto externalMatch = findExternalMatch(sketch, sourceObject, subName, intersection);
    if (externalMatch.isValid() && externalMatch.matchesType) {
        return -1;
    }

    if (const Element* existing = findElementBySource(sourceObject, subName, intersection)) {
        return existing->id;
    }

    auto preview = sketch->buildProjectedExternalGeometry(sourceObject, subName.c_str(), intersection);
    if (preview.empty()) {
        return -1;
    }

    Element element;
    element.id = nextId++;
    element.sourceObjectName = sourceObject->getNameInDocument();
    element.subName = subName;
    element.type = isEdge ? ElementType::Edge : ElementType::Vertex;
    element.intersection = intersection;
    element.geometry = std::move(preview);

    const int id = element.id;
    elements.emplace_back(std::move(element));
    return id;
}

LazyExternalGeometryLayer::Element* LazyExternalGeometryLayer::getElement(int id)
{
    return findElementById(elements, id);
}

const LazyExternalGeometryLayer::Element* LazyExternalGeometryLayer::getElement(int id) const
{
    return findElementById(elements, id);
}

const LazyExternalGeometryLayer::Element* LazyExternalGeometryLayer::findElementBySource(
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection
) const
{
    const char* sourceObjectName = sourceObject ? sourceObject->getNameInDocument() : nullptr;
    auto it = std::find_if(elements.begin(), elements.end(), [&](const Element& element) {
        return !Base::Tools::isNullOrEmpty(sourceObjectName)
            && element.sourceObjectName == sourceObjectName && element.subName == subName
            && element.intersection == intersection;
    });

    return it == elements.end() ? nullptr : &(*it);
}

const Part::Geometry* LazyExternalGeometryLayer::getPreviewGeometry(int id) const
{
    const Element* element = getElement(id);
    if (!element) {
        return nullptr;
    }

    for (const auto& geometry : element->geometry) {
        if (geometry) {
            return geometry.get();
        }
    }

    return nullptr;
}

bool LazyExternalGeometryLayer::isElementVertex(int id) const
{
    const Element* element = getElement(id);
    return element && element->isVertex();
}

bool LazyExternalGeometryLayer::getSourceReference(
    int id,
    std::string& sourceObjectName,
    std::string& subName,
    bool& intersection,
    bool& vertex
) const
{
    const Element* element = getElement(id);
    if (!element) {
        return false;
    }

    sourceObjectName = element->sourceObjectName;
    subName = element->subName;
    intersection = element->intersection;
    vertex = element->isVertex();
    return true;
}

bool LazyExternalGeometryLayer::selectElement(int id, bool selected)
{
    Element* element = getElement(id);
    if (!element) {
        return false;
    }

    element->selected = selected;
    if (!selected && preselectedElementId != id) {
        elements.erase(
            std::remove_if(
                elements.begin(),
                elements.end(),
                [id](const Element& candidate) { return candidate.id == id; }
            ),
            elements.end()
        );
    }
    return true;
}

bool LazyExternalGeometryLayer::isElementSelected(int id) const
{
    const Element* element = getElement(id);
    return element && element->selected;
}

std::vector<int> LazyExternalGeometryLayer::getSelectedElementIds() const
{
    std::vector<int> ids;
    for (const auto& element : elements) {
        if (element.selected) {
            ids.push_back(element.id);
        }
    }
    return ids;
}

void LazyExternalGeometryLayer::clearSelectedElements()
{
    elements.erase(
        std::remove_if(
            elements.begin(),
            elements.end(),
            [this](const Element& element) {
                return element.selected && element.id != preselectedElementId;
            }
        ),
        elements.end()
    );

    if (Element* element = getElement(preselectedElementId)) {
        element->selected = false;
    }
}

void LazyExternalGeometryLayer::clearPreselectedElement()
{
    const int oldPreselectedId = preselectedElementId;
    preselectedElementId = -1;

    elements.erase(
        std::remove_if(
            elements.begin(),
            elements.end(),
            [oldPreselectedId](const Element& element) {
                return element.id == oldPreselectedId && !element.selected;
            }
        ),
        elements.end()
    );
}

void LazyExternalGeometryLayer::setEnabled(bool on)
{
    if (enabled == on) {
        return;
    }

    enabled = on;
    if (!enabled) {
        clearPreselectedElement();
    }
}

bool LazyExternalGeometryLayer::isEnabled() const
{
    return enabled;
}

void LazyExternalGeometryLayer::appendElementGeometry(
    const Element& element,
    std::vector<Base::Vector3d>& points,
    std::vector<Base::Vector3d>& curveCoords,
    std::vector<int32_t>& curveVertexCounts
)
{
    for (const auto& geometry : element.geometry) {
        if (!geometry) {
            continue;
        }

        if (auto point = freecad_cast<const Part::GeomPoint*>(geometry.get())) {
            points.push_back(point->getPoint());
            continue;
        }

        appendCurveToCoin(geometry.get(), curveCoords, curveVertexCounts);
    }
}

void LazyExternalGeometryLayer::draw(const DrawingParameters& parameters, int viewOrientationFactor)
{
    if (!root) {
        return;
    }

    std::vector<Base::Vector3d> highlightPoints;
    std::vector<Base::Vector3d> highlightCurveCoords;
    std::vector<int32_t> highlightCurveVertexCounts;

    if (enabled) {
        for (const auto& element : elements) {
            if (element.selected || element.id == preselectedElementId) {
                appendElementGeometry(
                    element,
                    highlightPoints,
                    highlightCurveCoords,
                    highlightCurveVertexCounts
                );
            }
        }
    }

    const float pointz = viewOrientationFactor * parameters.zLowPoints;
    const float linez = viewOrientationFactor * parameters.zLowLines;

    highlightMaterial->diffuseColor = DrawingParameters::CurveExternalColor;
    highlightMaterial->transparency = 0.0f;
    highlightPointDrawStyle->pointSize = parameters.markerSize;
    highlightPointSet->markerIndex
        = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", parameters.markerSize);

    highlightCurveDrawStyle->lineWidth = parameters.ExternalWidth * parameters.pixelScalingFactor;
    highlightCurveDrawStyle->linePattern = parameters.ExternalPattern;

    highlightPointCoordinates->point.setNum(highlightPoints.size());
    SbVec3f* pointVertices = highlightPointCoordinates->point.startEditing();
    for (const auto& point : highlightPoints) {
        pointVertices->setValue(point.x, point.y, pointz);
        ++pointVertices;
    }
    highlightPointCoordinates->point.finishEditing();

    highlightCurveCoordinates->point.setNum(highlightCurveCoords.size());
    highlightCurveSet->numVertices.setNum(highlightCurveVertexCounts.size());

    SbVec3f* curveVertices = highlightCurveCoordinates->point.startEditing();
    for (const auto& point : highlightCurveCoords) {
        curveVertices->setValue(point.x, point.y, linez);
        ++curveVertices;
    }
    highlightCurveCoordinates->point.finishEditing();

    int32_t* vertexCounts = highlightCurveSet->numVertices.startEditing();
    for (auto count : highlightCurveVertexCounts) {
        *vertexCounts = count;
        ++vertexCounts;
    }
    highlightCurveSet->numVertices.finishEditing();
}

}  // namespace SketcherGui
