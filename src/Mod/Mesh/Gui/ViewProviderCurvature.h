// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObserver.h>
#include <Base/Observer.h>

#include "ViewProvider.h"


class SoSeparator;
class SbVec3f;
class SoSwitch;
class SoCoordinate3;
class SoNormal;
class SoIndexedFaceSet;
class SoFaceSet;
class SoPath;
class SoLocateHighlight;
class SoTransformerManip;

namespace Gui
{
class SoFCColorBar;
class View3DInventorViewer;
}  // namespace Gui

namespace Mesh
{
class PropertyCurvatureList;
}

namespace MeshGui
{

/** The ViewProviderMeshCurvature class is associated to the mesh curvature feature. It allows one
 * to display the most known types of curvatures, such as Gaussian curvature, mean curvature,
 * minimum and maximum curvature. Moreover a color bar is also added to the scene.
 *
 * @author Werner Mayer
 */
class MeshGuiExport ViewProviderMeshCurvature: public Gui::ViewProviderDocumentObject,
                                               public App::DocumentObserver,
                                               public Base::Observer<int>
{
    using inherited = Gui::ViewProviderDocumentObject;

    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshCurvature);

public:
    ViewProviderMeshCurvature();
    ~ViewProviderMeshCurvature() override;

    // NOLINTBEGIN
    App::PropertyMaterial TextureMaterial;
    // NOLINTEND

    /// Extracts the mesh data from the feature \a pcFeature and creates an Inventor node \a SoNode
    /// with these data.
    void attach(App::DocumentObject* pcFeature) override;
    bool useNewSelectionModel() const override
    {
        return false;
    }
    /// Sets the viewing mode
    void setDisplayMode(const char* ModeName) override;
    /// get the default display mode
    const char* getDefaultDisplayMode() const override;
    /// Returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;
    /// Updates the mesh feature representation
    void updateData(const App::Property* prop) override;
    /// Returns a pixmap for the associated feature type
    QIcon getIcon() const override;
    /// Once the color bar settinhs has been changed this method gets called to update the feature's
    /// representation
    void OnChange(Base::Subject<int>& rCaller, int rcReason) override;
    /// Returns a color bar
    SoSeparator* getFrontRoot() const override;
    /// Hide the object in the view
    void hide() override;
    /// Show the object in the view
    void show() override;

public:
    static void curvatureInfoCallback(void* ud, SoEventCallback* n);

protected:
    void onChanged(const App::Property* prop) override;
    void setVertexCurvatureMode(int mode);
    std::string curvatureInfo(bool detail, int index1, int index2, int index3) const;
    void touchShapeNode();

private:
    void init(const Mesh::PropertyCurvatureList* prop);
    void deleteColorBar();
    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;

private:
    SoMaterial* pcColorMat;
    SoGroup* pcLinkRoot;
    Gui::SoFCColorBar* pcColorBar;
    SoDrawStyle* pcColorStyle;
    SoSeparator* pcColorRoot;

private:
    static bool addflag;

    FC_DISABLE_COPY_MOVE(ViewProviderMeshCurvature)
};

}  // namespace MeshGui
