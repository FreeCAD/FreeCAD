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

// Forward declarations for classes to reduce include dependencies
class SoTexture2;
class SoSeparator;
class SoClipPlane;
class QMenu;

namespace App {
    class DocumentObject;
    class Property;
}

namespace PartDesign {
    class Hole;
}

// Forward declarations for OpenCascade classes
class TopoDS_Face;
class TopoDS_Shape;
class gp_Dir;
class gp_Pnt;


namespace PartDesignGui {

class PartDesignGuiExport ViewProviderHole : public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderHole);

    SoClipPlane* m_endThreadClipper {nullptr};
    SoTexture2*  m_threadTexture {nullptr};

public:
    /// constructor
    ViewProviderHole();
    /// destructor
    ~ViewProviderHole() override;

    /// grouping handling
    std::vector<App::DocumentObject*> claimChildren()const override;
    void setupContextMenu(QMenu *menu, QObject *receiver, const char *member) override;
    SoSeparator* createThreadTextureSeparator();
    void updateThreadClipper();
    void updateThreadDirection();

protected:
    TaskDlgFeatureParameters* getEditDialog() override;
    void updateData(const App::Property* prop) override;

private:
    std::unique_ptr<Gui::ViewProviderTextureExtension> textureExtension;
    std::vector<TopoDS_Face> collectBoreFaces(
        const PartDesign::Hole* pcHole,
        gp_Dir& holeFeatureAxis,
        gp_Pnt& axisLocationPnt
    ) const;
    App::Material getGlobalMaterial();
    bool generateBoreMeshData(
        const PartDesign::Hole* pcHole, const std::vector<TopoDS_Face>& boreFaces,
        const gp_Dir& holeFeatureAxis, const gp_Pnt& axisLocationPnt,
        double& outMinProj, double& outMaxProj,
        std::vector<SbVec3f>& vertices, std::vector<SbVec3f>& normals,
        std::vector<int>& indices, std::vector<SbVec2f>& uvs
    ) const;
    TopoDS_Shape getLastShownShape(const PartDesign::Hole* pcHole) const;

};

} // namespace PartDesignGui

#endif // PARTGUI_ViewProviderHole_H
