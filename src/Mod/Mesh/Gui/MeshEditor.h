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

class SoCoordinate3;
class SoFaceSet;
class SoEventCallback;
class SoPickedPoint;
class SoQtViewer;

namespace Gui { class View3DInventor; }

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
    std::vector<std::string> getDisplayModes(void) const;
    SoPickedPoint* getPickedPoint(const SbVec2s& pos, const SoQtViewer* viewer) const;

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

} // namespace MeshGui


#endif // MESHGUI_MESHEDITOR_H

