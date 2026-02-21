// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTGUI_ViewProviderHole_H
#define PARTGUI_ViewProviderHole_H

#include "ViewProvider.h"
#include <Gui/ViewProviderTextureExtension.h>
#include <App/Material.h>

#include <vector>
#include <memory>

class SoTexture2Transform;
class SoSeparator;
class SoClipPlane;
class QMenu;

namespace App
{
class DocumentObject;
class Property;
}  // namespace App

namespace PartDesign
{
class Hole;
}

// Forward declarations for OpenCascade classes
class TopoDS_Face;
class TopoDS_Shape;
class gp_Dir;
class gp_Pnt;


namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderHole: public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderHole);

    SoClipPlane* m_endThreadClipper {nullptr};
    SoTexture2Transform* m_textureTransform {nullptr};

public:
    /// constructor
    ViewProviderHole();
    /// destructor
    ~ViewProviderHole() override;
    bool onDelete(const std::vector<std::string>& arg) override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    SoSeparator* createThreadTextureSeparator();
    bool isHoleThreadVisible() const;
    void updateOverlay() override;

protected:
    TaskDlgFeatureParameters* getEditDialog() override;
    void updateData(const App::Property* prop) override;

private:
    std::unique_ptr<Gui::ViewProviderTextureExtension> textureExtension;
    std::optional<gp_Dir> getHoleNormal(const PartDesign::Hole* pcHole) const;
    std::optional<gp_Pnt> getHoleOrigin(const PartDesign::Hole* pcHole) const;
    App::Material getGlobalMaterial();
    TopoDS_Shape getCurrentlyVisibleShape(const PartDesign::Hole* pcHole) const;
    void updateThreadClipper(const PartDesign::Hole* pcHole);
    void updateThreadDirection(const PartDesign::Hole* pcHole);
    void applyThreadPhaseOffset(const PartDesign::Hole* pcHole);

    // meshing and UVs
    std::vector<TopoDS_Face> collectBoreFaces(const PartDesign::Hole* pcHole) const;
    bool generateBoreMeshData(
        const PartDesign::Hole* pcHole,
        const gp_Pnt& holeOriginPnt,
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
        double minProj,
        double initialRadius,
        double threadPitch
    );
    void handleSeamTriangle(
        std::vector<SbVec3f>& vertices,
        std::vector<SbVec3f>& normals,
        std::vector<SbVec2f>& uvs,
        std::array<int, 3>& triIndices
    );
    std::map<const PartDesign::Hole*, SoSwitch*> m_threadOverlays;
    void updateThreadTexture();
    void clearThreadTextures();
};

}  // namespace PartDesignGui

#endif  // PARTGUI_ViewProviderHole_H
