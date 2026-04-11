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

#include "ViewProvider.h"


class SoSeparator;
class SbVec3f;
class SoSwitch;
class SoCoordinate3;
class SoDragger;
class SoNormal;
class SoIndexedFaceSet;
class SoFaceSet;
class SoPath;
class SoLocateHighlight;
class SbRotation;
class SoTrackballDragger;
class SoTransformerManip;

namespace Gui
{
class View3DInventorViewer;
}

namespace MeshGui
{

/** Like Mesh viewprovider but with manipulator
 */
class ViewProviderMeshTransformDemolding: public ViewProviderMesh
{
    PROPERTY_HEADER_WITH_OVERRIDE(MeshGui::ViewProviderMeshTransformDemolding);

public:
    ViewProviderMeshTransformDemolding();
    ~ViewProviderMeshTransformDemolding() override;


    /**
     * Extracts the mesh data from the feature \a pcFeature and creates
     * an Inventor node \a SoNode with these data.
     */
    void attach(App::DocumentObject* obj) override;

    /// set the viewing mode
    void setDisplayMode(const char* ModeName) override;
    /// get the default display mode
    const char* getDefaultDisplayMode() const override;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;

protected:
    void setCenterPoint();
    void calcMaterialIndex(const SbRotation& rot);
    void calcNormalVector();

    static void sValueChangedCallback(void*, SoDragger*);
    void valueChangedCallback();

    static void sDragEndCallback(void*, SoDragger*);
    void DragEndCallback();

private:
    SoTrackballDragger* pcTrackballDragger;
    SoTransform* pcTransformDrag;
    SoMaterial* pcColorMat;
    std::vector<SbVec3f> normalVector;
    Base::Vector3f center;

    FC_DISABLE_COPY_MOVE(ViewProviderMeshTransformDemolding)
};

}  // namespace MeshGui
