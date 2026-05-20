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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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
    bool intersection
);

struct DrawingParameters;

/// Owns the edit-mode overlay used for on-demand external geometry previews.
///
/// The layer stores temporary source references and preview geometry only; it does not add
/// anything to SketchObject::ExternalGeometry.
class LazyExternalGeometryLayer
{
public:
    LazyExternalGeometryLayer();
    ~LazyExternalGeometryLayer();

    LazyExternalGeometryLayer(const LazyExternalGeometryLayer&) = delete;
    LazyExternalGeometryLayer& operator=(const LazyExternalGeometryLayer&) = delete;

    void attachTo(SoSeparator* editRoot);
    void detach();

    /// Add or reuse a temporary preview for a source object subelement. Returns the lazy id, or -1.
    int preselectSourceReference(
        Sketcher::SketchObject* sketch,
        App::DocumentObject* sourceObject,
        const std::string& subName,
        bool intersection = false
    );

    /// Return the first preview geometry for the lazy id, or nullptr if the id is not active.
    const Part::Geometry* getPreviewGeometry(int id) const;
    bool isElementVertex(int id) const;
    /// Resolve a lazy id back to its source object name, subelement, and mode flags.
    bool getSourceReference(
        int id,
        std::string& sourceObjectName,
        std::string& subName,
        bool& intersection,
        bool& vertex
    ) const;

    bool selectElement(int id, bool selected);
    bool isElementSelected(int id) const;
    std::vector<int> getSelectedElementIds() const;
    void clearSelectedElements();
    void clearPreselectedElement();

    /// Temporarily disables preview picking/selection without destroying already tracked elements.
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /// Rebuild the Coin nodes from the currently tracked preview elements.
    void draw(const DrawingParameters& parameters, int viewOrientationFactor);

private:
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
        bool selected = false;
        std::vector<std::unique_ptr<Part::Geometry>> geometry;

        bool isVertex() const
        {
            return type == ElementType::Vertex;
        }
    };

    int addSourceReference(
        Sketcher::SketchObject* sketch,
        App::DocumentObject* sourceObject,
        const std::string& subName,
        bool intersection
    );

    Element* getElement(int id);
    const Element* getElement(int id) const;
    const Element* findElementBySource(
        App::DocumentObject* sourceObject,
        const std::string& subName,
        bool intersection
    ) const;

    void createNodes();
    void resetNodes();
    void appendElementGeometry(
        const Element& element,
        std::vector<Base::Vector3d>& points,
        std::vector<Base::Vector3d>& curveCoords,
        std::vector<int32_t>& curveVertexCounts
    );

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
    int preselectedElementId = -1;
    bool enabled = true;
    int nextId = 1;
};

}  // namespace SketcherGui
