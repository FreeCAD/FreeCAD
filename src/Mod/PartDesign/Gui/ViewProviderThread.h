// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include "ViewProviderDressUp.h"
#include <Gui/ViewProviderTextureExtension.h>
#include <App/Material.h>

#include <vector>
#include <memory>

class SoTexture2Transform;
class SoSeparator;
class SoGroup;
class SoClipPlane;
class SoToggleSwitch;
class QMenu;

namespace App
{
class DocumentObject;
class Property;
}

namespace PartDesign
{
class Thread;
}

class TopoDS_Face;
class TopoDS_Shape;
class gp_Dir;
class gp_Pnt;


namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderThread: public ViewProviderDressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderThread);

    SoClipPlane* m_startThreadClipper {nullptr};
    SoClipPlane* m_endThreadClipper {nullptr};
    SoTexture2Transform* m_textureTransform {nullptr};

public:
    /// constructor
    ViewProviderThread();
    
    /// destructor
    ~ViewProviderThread() override;
    bool onDelete(const std::vector<std::string>& arg) override;

    /// grouping handling
    const std::string& featureName() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
    SoSeparator* createThreadTextureSeparator();
    bool isHoleThreadVisible() const;
    void updateOverlay() override;
    void showPreview(bool enable) override;

protected:
    /// Returns a newly create dialog for the part to be placed in the task view
    TaskDlgFeatureParameters* getEditDialog() override;
    void updateData(const App::Property* prop) override;
    void onChanged(const App::Property* prop) override;
    void attachPreview() override;
    void updatePreview() override;

private:
    friend class TaskDlgThreadParameters;

    std::unique_ptr<Gui::ViewProviderTextureExtension> textureExtension;
    Gui::CoinPtr<PartGui::SoPreviewShape> pcReducedBasePreview;
    Gui::CoinPtr<SoToggleSwitch> pcReducedBaseToggle;
    Gui::CoinPtr<SoGroup> overlayRoot;
    bool baseHiddenForPreview {false};

    PartDesignGui::ViewProvider* getBaseFeatureViewProvider() const;
    void syncExternalPreviewVisibility();
    void restoreBaseVisibility();
    std::optional<gp_Dir> getThreadNormal(const PartDesign::Thread* pcThread) const;
    std::optional<gp_Pnt> getThreadOrigin(const PartDesign::Thread* pcThread) const;
    std::vector<gp_Pnt> getThreadLocations(const PartDesign::Thread* pcThread) const;
    App::Material getGlobalMaterial();
    TopoDS_Shape getCurrentlyVisibleShape(const PartDesign::Thread* pcThread) const;
    void updateThreadClipper(const gp_Pnt& origin, const gp_Dir& direction, double length);
    void updateThreadDirection(const PartDesign::Thread* pcThread);
    void applyThreadPhaseOffset(const PartDesign::Thread* pcThread);

    std::vector<TopoDS_Face> collectBoreFaces(
        const PartDesign::Thread* pcThread, const gp_Pnt& threadOrigin
    ) const;
    bool generateBoreMeshData(
        const PartDesign::Thread* pcThread,
        const gp_Pnt& threadOriginPnt,
        std::vector<SbVec3f>& vertices,
        std::vector<SbVec3f>& normals,
        std::vector<int>& indices,
        std::vector<SbVec2f>& uvs
    );
    std::pair<gp_Dir, gp_Dir> buildOrthonormalFrame(const gp_Dir& axis);
    SbVec2f addVertex(
        std::vector<SbVec3f>& vertices,
        std::vector<SbVec3f>& normals,
        const gp_Pnt& pt,
        const gp_Pnt& origin,
        const gp_Dir& axis,
        const gp_Dir& x_dir,
        const gp_Dir& y_dir,
        double surfaceSlope,
        double threadPitch,
        bool reverseNormal
    );
    void handleSeamTriangle(
        std::vector<SbVec3f>& vertices,
        std::vector<SbVec3f>& normals,
        std::vector<SbVec2f>& uvs,
        std::array<int, 3>& triIndices
    );
    std::map<const PartDesign::Thread*, SoSwitch*> m_threadOverlays;
    void clearThreadTextures();
};

}
