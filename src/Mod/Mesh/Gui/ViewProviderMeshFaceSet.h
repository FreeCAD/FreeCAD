/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHGUI_VIEWPROVIDERMESHFACESET_H
#define MESHGUI_VIEWPROVIDERMESHFACESET_H

#include <Mod/Mesh/Gui/ViewProvider.h>

namespace MeshGui
{
class SoFCIndexedFaceSet;

/**
 * The ViewProviderMeshFaceSet class creates nodes for representing the mesh
 * data structure. Depending on the size of the mesh it uses two ways to
 * render it:
 * - For huge meshes it renders directly the data structure. Rendering directly
 *   the data structure has the advantage to save memory by not creating the
 *   according OpenInventor nodes which would more or less duplicate the
 *   memory for a mesh. Especially for huge with several hundred thousands or
 *   even millions of triangles, the amount of saved memory is considerable.
 * - For all other meshes it uses the appropriate OpenInventor nodes. Although
 *   this needs more memory its usage is much more flexible. It offers several
 *   nice features like a smooth-shaded appearance of a mesh whereas the
 *   OpenInventor nodes are already capable to do everything automatically,
 *   or the usage with textures.
 *
 * For more details @see SoFCMeshNode and SoFCMeshFaceSet.
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshFaceSet: public ViewProviderMesh
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshFaceSet);

public:
    ViewProviderMeshFaceSet();
    ~ViewProviderMeshFaceSet() override;

    void attach(App::DocumentObject* pcFeat) override;
    void updateData(const App::Property*) override;

protected:
    void showOpenEdges(bool) override;
    SoShape* getShapeNode() const override;
    SoNode* getCoordNode() const override;

private:
    bool directRendering;
    unsigned long triangleCount;
    SoCoordinate3* pcMeshCoord;
    SoFCIndexedFaceSet* pcMeshFaces;
    SoFCMeshObjectNode* pcMeshNode;
    SoFCMeshObjectShape* pcMeshShape;

    FC_DISABLE_COPY_MOVE(ViewProviderMeshFaceSet)
};

}  // namespace MeshGui


#endif  // MESHGUI_VIEWPROVIDERMESHFACESET_H
