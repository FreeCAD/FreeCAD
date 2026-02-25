// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QObject>

#include <Mod/Mesh/Gui/ViewProvider.h>


class SoCoordinate3;
class SoFaceSet;
class SoEventCallback;
class SoPickedPoint;
class SoGroup;
class SoSeparator;
class SoRayPickAction;
class SbLine;
class SbVec3f;

namespace Gui
{
class View3DInventor;
class View3DInventorViewer;
}  // namespace Gui
namespace Mesh
{
class MeshObject;
}
namespace Mesh
{
class Feature;
}
namespace MeshGui
{
class SoFCMeshPickNode;

/** The ViewProviderFace class is used to display a single face.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderFace: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderFace);

public:
    ViewProviderFace();
    ~ViewProviderFace() override;

    // Build up the initial Inventor node
    void attach(App::DocumentObject* obj) override;
    void setDisplayMode(const char* ModeName) override;
    const char* getDefaultDisplayMode() const override;
    std::vector<std::string> getDisplayModes() const override;
    SoPickedPoint* getPickedPoint(const SbVec2s& pos, const Gui::View3DInventorViewer* viewer) const;

    ViewProviderMesh* mesh {nullptr};
    std::vector<int> index;
    int current_index {-1};

    SoCoordinate3* pcCoords;
    SoFaceSet* pcFaces;
    SoFCMeshPickNode* pcMeshPick;

    FC_DISABLE_COPY_MOVE(ViewProviderFace)
};

/**
 * Display data of a mesh kernel.
 * \author Werner Mayer
 */
class MeshGuiExport MeshFaceAddition: public QObject
{
    Q_OBJECT

public:
    explicit MeshFaceAddition(Gui::View3DInventor* parent);
    ~MeshFaceAddition() override;

    void startEditing(ViewProviderMesh*);

public Q_SLOTS:
    void finishEditing();

private Q_SLOTS:
    void addFace();
    void clearPoints();
    void flipNormal();

private:
    bool addMarkerPoint();
    void showMarker(SoPickedPoint*);
    static void addFacetCallback(void* ud, SoEventCallback* n);

private:
    ViewProviderFace* faceView;

    Q_DISABLE_COPY_MOVE(MeshFaceAddition)
};

class MeshGuiExport MeshHoleFiller
{
public:
    MeshHoleFiller() = default;
    virtual ~MeshHoleFiller() = default;
    MeshHoleFiller(const MeshHoleFiller&) = delete;
    MeshHoleFiller(MeshHoleFiller&&) = delete;
    MeshHoleFiller& operator=(const MeshHoleFiller&) = delete;
    MeshHoleFiller& operator=(MeshHoleFiller&&) = delete;
    virtual bool fillHoles(
        Mesh::MeshObject&,
        const std::list<std::vector<Mesh::PointIndex>>&,
        Mesh::PointIndex,
        Mesh::PointIndex
    )
    {
        return false;
    }
};

/**
 * Display data of a mesh kernel.
 * \author Werner Mayer
 */
class MeshGuiExport MeshFillHole: public QObject
{
    Q_OBJECT

public:
    MeshFillHole(MeshHoleFiller& hf, Gui::View3DInventor* parent);
    ~MeshFillHole() override;

    void startEditing(ViewProviderMesh*);

public Q_SLOTS:
    void finishEditing();

private Q_SLOTS:
    void closeBridge();

private:
    using TBoundary = std::vector<Mesh::PointIndex>;
    using Connection = fastsignals::connection;

    static void fileHoleCallback(void* ud, SoEventCallback* n);
    void createPolygons();
    SoNode* getPickedPolygon(const SoRayPickAction& action) const;
    float findClosestPoint(const SbLine& ray, const TBoundary& polygon, Mesh::PointIndex&, SbVec3f&) const;
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);

private:
    SoSeparator* myBoundariesRoot;
    SoGroup* myBoundariesGroup;
    SoSeparator* myBoundaryRoot;
    SoSeparator* myBridgeRoot;
    SoCoordinate3* myVertex;
    std::map<SoNode*, TBoundary> myPolygons;
    Mesh::Feature* myMesh {nullptr};
    int myNumPoints {0};
    Mesh::PointIndex myVertex1 {0};
    Mesh::PointIndex myVertex2 {0};
    TBoundary myPolygon;
    MeshHoleFiller& myHoleFiller;
    Connection myConnection;

    Q_DISABLE_COPY_MOVE(MeshFillHole)
};

}  // namespace MeshGui
