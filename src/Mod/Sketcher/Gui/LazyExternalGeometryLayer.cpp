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
#include <cstring>
#include <limits>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <App/DocumentObject.h>
#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/GroupExtension.h>
#include <App/IndexedName.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditModeCoinManagerParameters.h"
#include "LazyExternalGeometryLayer.h"

using namespace SketcherGui;

namespace
{
constexpr int HitTestCurveSampleCount = 96;
constexpr int MinimumCurveSampleCount = 8;
constexpr int DisplayCurveMaxSubdivisionDepth = 8;
constexpr double DisplayCurveRelativeDeviation = 0.01;

bool isEdgeSubName(const std::string& subName)
{
    const Data::IndexedName indexedSubName(subName.c_str());
    return indexedSubName && indexedSubName.getIndex() > 0
        && std::strcmp(indexedSubName.getType(), "Edge") == 0;
}

bool isVertexSubName(const std::string& subName)
{
    const Data::IndexedName indexedSubName(subName.c_str());
    return indexedSubName && indexedSubName.getIndex() > 0
        && std::strcmp(indexedSubName.getType(), "Vertex") == 0;
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

bool isValidSource(App::DocumentObject* object)
{
    return object && !Base::Tools::isNullOrEmpty(object->getNameInDocument())
        && !object->isDerivedFrom<Sketcher::SketchObject>() && isSourceVisible(object);
}

std::string makeElementKey(const LazyExternalGeometryLayer::Element& element)
{
    return element.sourceObjectName + "\n" + element.subName + "\n"
        + (element.intersection ? "1" : "0");
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

double distanceToSampledCurve2d(const Part::Geometry* geometry, const Base::Vector2d& point, int segments)
{
    if (!geometry) {
        return std::numeric_limits<double>::max();
    }

    if (auto line = freecad_cast<const Part::GeomLineSegment*>(geometry)) {
        return distancePointToSegment2d(
            point,
            toVector2d(line->getStartPoint()),
            toVector2d(line->getEndPoint())
        );
    }

    if (auto curve = freecad_cast<const Part::GeomCurve*>(geometry)) {
        const int sampleCount = std::max(segments, MinimumCurveSampleCount);
        double first = 0.0;
        double last = 0.0;
        if (!getCurveParameterRange(*curve, first, last)) {
            return std::numeric_limits<double>::max();
        }

        const double step = (last - first) / sampleCount;
        Base::Vector2d previous = toVector2d(curve->value(first));
        double best = std::numeric_limits<double>::max();
        for (int i = 1; i <= sampleCount; ++i) {
            const double parameter = i == sampleCount ? last : first + i * step;
            const Base::Vector2d current = toVector2d(curve->value(parameter));
            best = std::min(best, distancePointToSegment2d(point, previous, current));
            previous = current;
        }
        return best;
    }

    return std::numeric_limits<double>::max();
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

std::vector<LazyExternalGeometryLayer::Source> LazyExternalGeometryLayer::collectSources(
    Sketcher::SketchObject* sketch
)
{
    std::vector<Source> sources;
    if (!sketch || !sketch->getDocument()) {
        return sources;
    }

    for (auto* object : sketch->getDocument()->getObjects()) {
        if (object == sketch || !isValidSource(object)) {
            continue;
        }

        try {
            const auto shape = Part::Feature::getTopoShape(
                object,
                Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
            );
            const int edgeCount = shape.countSubShapes(TopAbs_EDGE);
            const int vertexCount = shape.countSubShapes(TopAbs_VERTEX);
            if (edgeCount > 0 || vertexCount > 0) {
                sources.push_back({object, edgeCount, vertexCount});
            }
        }
        catch (const Base::Exception&) {
        }
        catch (const std::exception&) {
        }
    }

    return sources;
}

std::vector<std::string> LazyExternalGeometryLayer::getSourceNames(const std::vector<Source>& sources)
{
    std::vector<std::string> names;
    names.reserve(sources.size());

    for (const auto& source : sources) {
        names.emplace_back(source.object->getNameInDocument());
    }

    return names;
}

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
    bool intersection,
    int fallbackIndex
)
{
    const auto match = findExternalMatch(sketch, sourceObject, subName, intersection);
    if (match.isValid()) {
        return match.matchesType ? match.index : -1;
    }

    if (sketch && fallbackIndex >= 0 && fallbackIndex < sketch->getExternalGeometryCount()) {
        return fallbackIndex;
    }

    return -1;
}

}  // namespace SketcherGui

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

void LazyExternalGeometryLayer::clear()
{
    elements.clear();
    preselectedElementId = -1;
    selectedElementIds.clear();
    sourceObjectNames.clear();
    nextId = 1;

    if (highlightPointCoordinates) {
        highlightPointCoordinates->point.setNum(0);
    }
    if (highlightCurveCoordinates) {
        highlightCurveCoordinates->point.setNum(0);
    }
    if (highlightCurveSet) {
        highlightCurveSet->numVertices.setNum(0);
    }
}

void LazyExternalGeometryLayer::rebuildSources(Sketcher::SketchObject* sketch)
{
    auto sources = collectSources(sketch);
    rebuildFromSourceObjects(sketch, sources, getSourceNames(sources));
}

bool LazyExternalGeometryLayer::syncSources(Sketcher::SketchObject* sketch)
{
    auto sources = collectSources(sketch);
    auto sourceNames = getSourceNames(sources);

    if (sourceNames == sourceObjectNames) {
        return false;
    }

    rebuildFromSourceObjects(sketch, sources, std::move(sourceNames));
    return true;
}

void LazyExternalGeometryLayer::rebuildFromSourceObjects(
    Sketcher::SketchObject* sketch,
    const std::vector<Source>& sources,
    std::vector<std::string> sourceNames
)
{
    std::set<std::string> selectedKeys;
    for (int id : selectedElementIds) {
        if (const Element* element = getElement(id)) {
            selectedKeys.insert(makeElementKey(*element));
        }
    }

    std::string preselectedKey;
    if (const Element* element = getElement(preselectedElementId)) {
        preselectedKey = makeElementKey(*element);
    }

    clear();
    sourceObjectNames = std::move(sourceNames);

    if (!sketch) {
        return;
    }

    for (const auto& source : sources) {
        std::vector<std::unique_ptr<Part::Geometry>> visibleGeometry;
        if (!showHiddenEdges) {
            visibleGeometry = sketch->buildVisibleExternalGeometryOnSketchPlane(source.object);
        }
        const auto* visibleGeometryPtr = visibleGeometry.empty() ? nullptr : &visibleGeometry;

        for (int i = 1; i <= source.edgeCount; ++i) {
            addSourceReference(
                sketch,
                source.object,
                "Edge" + std::to_string(i),
                false,
                false,
                visibleGeometryPtr
            );
        }
        for (int i = 1; i <= source.vertexCount; ++i) {
            addSourceReference(sketch, source.object, "Vertex" + std::to_string(i), false, false);
        }
    }

    for (const auto& element : elements) {
        const std::string key = makeElementKey(element);
        if (selectedKeys.find(key) != selectedKeys.end()) {
            selectedElementIds.insert(element.id);
        }
        if (!preselectedKey.empty() && key == preselectedKey) {
            preselectedElementId = element.id;
        }
    }
}

int LazyExternalGeometryLayer::addSourceReference(
    Sketcher::SketchObject* sketch,
    App::DocumentObject* sourceObject,
    const std::string& subName,
    bool intersection,
    bool buildPreview,
    const std::vector<std::unique_ptr<Part::Geometry>>* visibleGeometry
)
{
    if (!sketch || !sourceObject || Base::Tools::isNullOrEmpty(sourceObject->getNameInDocument())) {
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

    std::vector<std::unique_ptr<Part::Geometry>> preview;
    const bool needsVisibilityCheck = !showHiddenEdges && isEdge && visibleGeometry;
    const bool needsPreview = buildPreview || needsVisibilityCheck;
    if (needsPreview) {
        preview = sketch->buildProjectedExternalGeometry(sourceObject, subName.c_str(), intersection);
        if (preview.empty()) {
            return -1;
        }
    }

    if (needsVisibilityCheck
        && !sketch->isProjectedExternalGeometryVisibleOnSketchPlane(preview, *visibleGeometry)) {
        return -1;
    }

    if (const Element* existing = findElementBySource(sourceObject, subName, intersection)) {
        return existing->id;
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
    return element && element->type == ElementType::Vertex;
}

bool LazyExternalGeometryLayer::isElementSelected(int id) const
{
    return selectedElementIds.find(id) != selectedElementIds.end();
}

std::vector<int> LazyExternalGeometryLayer::getSelectedElementIds() const
{
    return std::vector<int>(selectedElementIds.begin(), selectedElementIds.end());
}

void LazyExternalGeometryLayer::setPreselectedElement(int id)
{
    preselectedElementId = id >= 0 ? id : -1;
}

void LazyExternalGeometryLayer::clearPreselectedElement()
{
    preselectedElementId = -1;
}

void LazyExternalGeometryLayer::setSelectedElement(int id, bool selected)
{
    if (id < 0) {
        return;
    }

    if (selected) {
        selectedElementIds.insert(id);
    }
    else {
        selectedElementIds.erase(id);
    }
}

void LazyExternalGeometryLayer::clearSelectedElements()
{
    selectedElementIds.clear();
}

void LazyExternalGeometryLayer::setEnabled(bool on)
{
    if (enabled == on) {
        return;
    }

    enabled = on;
    if (!enabled) {
        preselectedElementId = -1;
    }
}

bool LazyExternalGeometryLayer::isEnabled() const
{
    return enabled;
}

void LazyExternalGeometryLayer::setShowHiddenEdges(bool showHidden)
{
    showHiddenEdges = showHidden;
}

bool LazyExternalGeometryLayer::ensurePreview(Sketcher::SketchObject* sketch, Element& element)
{
    if (!element.geometry.empty()) {
        return true;
    }

    if (!sketch) {
        return false;
    }

    App::Document* document = sketch->getDocument();
    App::DocumentObject* sourceObject = document
        ? document->getObject(element.sourceObjectName.c_str())
        : nullptr;
    if (!sourceObject) {
        return false;
    }

    element.geometry = sketch->buildProjectedExternalGeometry(
        sourceObject,
        element.subName.c_str(),
        element.intersection
    );
    return !element.geometry.empty();
}

LazyExternalGeometryLayer::HitResult LazyExternalGeometryLayer::findClosestElement(
    Sketcher::SketchObject* sketch,
    const Base::Vector2d& point,
    double maxDistance
)
{
    if (!enabled || !std::isfinite(maxDistance) || maxDistance <= 0.0) {
        return {};
    }

    auto hitTest = [this, sketch, &point](Element& element) -> HitResult {
        if (!ensurePreview(sketch, element)) {
            return {};
        }

        HitResult hit;
        hit.id = element.id;
        hit.isVertex = element.type == ElementType::Vertex;
        hit.distance = std::numeric_limits<double>::max();

        for (const auto& geometry : element.geometry) {
            if (!geometry) {
                continue;
            }

            if (element.type == ElementType::Vertex) {
                if (auto geomPoint = freecad_cast<const Part::GeomPoint*>(geometry.get())) {
                    hit.distance = std::min(
                        hit.distance,
                        distance2d(point, toVector2d(geomPoint->getPoint()))
                    );
                }
                continue;
            }

            hit.distance = std::min(
                hit.distance,
                distanceToSampledCurve2d(geometry.get(), point, HitTestCurveSampleCount)
            );
        }

        return hit.distance == std::numeric_limits<double>::max() ? HitResult {} : hit;
    };

    auto findClosestVertex = [&]() -> HitResult {
        HitResult bestVertex;
        bestVertex.distance = std::numeric_limits<double>::max();

        for (auto& element : elements) {
            if (element.type != ElementType::Vertex) {
                continue;
            }

            const HitResult hit = hitTest(element);
            if (hit.isValid() && hit.distance < bestVertex.distance) {
                bestVertex = hit;
            }
        }

        return bestVertex;
    };

    if (auto* lastElement = getElement(preselectedElementId)) {
        const HitResult lastHit = hitTest(*lastElement);
        const double lastMaxDistance = lastHit.isVertex ? maxDistance : maxDistance * 0.6;
        if (lastHit.isValid() && lastHit.distance <= lastMaxDistance) {
            if (lastHit.isVertex) {
                return lastHit;
            }

            const HitResult vertexHit = findClosestVertex();
            if (vertexHit.isValid() && vertexHit.distance <= maxDistance) {
                return vertexHit;
            }

            return lastHit;
        }
    }

    HitResult bestVertex;
    HitResult bestEdge;
    bestVertex.distance = std::numeric_limits<double>::max();
    bestEdge.distance = std::numeric_limits<double>::max();

    for (auto& element : elements) {
        const HitResult hit = hitTest(element);
        if (!hit.isValid()) {
            continue;
        }

        if (hit.isVertex) {
            if (hit.distance < bestVertex.distance) {
                bestVertex = hit;
            }
            continue;
        }

        if (hit.distance < bestEdge.distance) {
            bestEdge = hit;
        }
    }

    if (bestVertex.isValid() && bestVertex.distance <= maxDistance) {
        return bestVertex;
    }
    if (bestEdge.isValid() && bestEdge.distance <= maxDistance) {
        return bestEdge;
    }

    return {};
}

bool LazyExternalGeometryLayer::shouldDraw(const Element& element) const
{
    return element.id == preselectedElementId
        || selectedElementIds.find(element.id) != selectedElementIds.end();
}

void LazyExternalGeometryLayer::appendElementToCoin(
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
            if (shouldDraw(element)) {
                appendElementToCoin(
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
    SbVec3f* pverts = highlightPointCoordinates->point.startEditing();
    for (const auto& point : highlightPoints) {
        pverts->setValue(point.x, point.y, pointz);
        ++pverts;
    }
    highlightPointCoordinates->point.finishEditing();

    highlightCurveCoordinates->point.setNum(highlightCurveCoords.size());
    highlightCurveSet->numVertices.setNum(highlightCurveVertexCounts.size());

    SbVec3f* cverts = highlightCurveCoordinates->point.startEditing();
    for (const auto& point : highlightCurveCoords) {
        cverts->setValue(point.x, point.y, linez);
        ++cverts;
    }
    highlightCurveCoordinates->point.finishEditing();

    int32_t* vertexCounts = highlightCurveSet->numVertices.startEditing();
    for (auto count : highlightCurveVertexCounts) {
        *vertexCounts = count;
        ++vertexCounts;
    }
    highlightCurveSet->numVertices.finishEditing();
}
