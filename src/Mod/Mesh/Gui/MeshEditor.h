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

#ifndef MESHGUI_MESHEDITOR_H
#define MESHGUI_MESHEDITOR_H

#include <QObject>
#include <Mod/Mesh/Gui/ViewProvider.h>
#include <boost_signals2.hpp>

class SoCoordinate3;
class SoFaceSet;
class SoEventCallback;
class SoPickedPoint;
class SoGroup;
class SoSeparator;
class SoRayPickAction;
class SbLine;
class SbVec3f;

namespace Gui { class View3DInventor; class View3DInventorViewer;}
namespace Mesh { class MeshObject; }
namespace Mesh { class Feature; }
namespace MeshGui {
class SoFCMeshPickNode;

/** The ViewProviderFace class is used to display a single face.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderFace : public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER(MeshGui::ViewProviderFace);

public:
    ViewProviderFace();
    virtual ~ViewProviderFace();

    // Build up the initial Inventor node
    void attach(App::DocumentObject* obj);
    void setDisplayMode(const char* ModeName);
    const char* getDefaultDisplayMode() const;
    std::vector<std::string> getDisplayModes() const;
    SoPickedPoint* getPickedPoint(const SbVec2s& pos, const Gui::View3DInventorViewer* viewer) const;

    ViewProviderMesh* mesh;
    std::vector<int> index;
    int current_index;

    SoCoordinate3   * pcCoords;
    SoFaceSet       * pcFaces;
    SoFCMeshPickNode* pcMeshPick;
};

/**
 * Display data of a mesh kernel.
 * \author Werner Mayer
 */
class MeshGuiExport MeshFaceAddition : public QObject
{
    Q_OBJECT

public:
    MeshFaceAddition(Gui::View3DInventor* parent);
    ~MeshFaceAddition();

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
    static void addFacetCallback(void * ud, SoEventCallback * n);

private:
    ViewProviderFace* faceView;
};

class MeshGuiExport MeshHoleFiller
{
public:
    MeshHoleFiller()
    {
    }
    virtual ~MeshHoleFiller()
    {
    }
    virtual bool fillHoles(Mesh::MeshObject&, const std::list<std::vector<Mesh::PointIndex> >&,
                           Mesh::PointIndex, Mesh::PointIndex)
    {
        return false;
    }
};

/**
 * Display data of a mesh kernel.
 * \author Werner Mayer
 */
class MeshGuiExport MeshFillHole : public QObject
{
    Q_OBJECT

public:
    MeshFillHole(MeshHoleFiller& hf, Gui::View3DInventor* parent);
    virtual ~MeshFillHole();

    void startEditing(ViewProviderMesh*);

public Q_SLOTS:
    void finishEditing();

private Q_SLOTS:
    void closeBridge();

private:
    typedef std::vector<Mesh::PointIndex> TBoundary;
    typedef boost::signals2::connection Connection;

    static void fileHoleCallback(void * ud, SoEventCallback * n);
    void createPolygons();
    SoNode* getPickedPolygon(const SoRayPickAction& action) const;
    float findClosestPoint(const SbLine& ray, const TBoundary& polygon,
                           Mesh::PointIndex&, SbVec3f&) const;
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);

private:
    SoSeparator* myBoundariesRoot;
    SoGroup* myBoundariesGroup;
    SoSeparator* myBoundaryRoot;
    SoSeparator* myBridgeRoot;
    SoCoordinate3* myVertex;
    std::map<SoNode*, TBoundary> myPolygons;
    Mesh::Feature* myMesh;
    int myNumPoints;
    Mesh::PointIndex myVertex1;
    Mesh::PointIndex myVertex2;
    TBoundary myPolygon;
    MeshHoleFiller& myHoleFiller;
    Connection myConnection;
};

} // namespace MeshGui


#endif // MESHGUI_MESHEDITOR_H

