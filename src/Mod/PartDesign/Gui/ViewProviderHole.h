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
class SoTextureCoordinate2;
class SoTexture2;
class SoMaterial;
class SoIndexedFaceSet;
class SoCoordinate3;
class SoSeparator;
class SoNormal;
class SoNormalBinding;
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

public:
    ViewProviderHole();
    ~ViewProviderHole() override;

    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    bool onDelete(const std::vector<std::string>& s) override;

protected:
    bool setEdit(int ModNum) override;
    void updateData(const App::Property* prop) override;
    void attach(App::DocumentObject* obj) override;

private:
    void clearBoreGeometry();
    std::vector<TopoDS_Face> collectBoreFaces(
        const PartDesign::Hole* pcHole,
        const TopoDS_Shape& holeShape,
        gp_Dir& holeFeatureAxis,
        gp_Pnt& axisLocationPnt
    );
    App::Material getGlobalMaterial(const PartDesign::Hole* pcHole) const;
    bool generateBoreMeshData(
        const PartDesign::Hole* pcHole,
        const std::vector<TopoDS_Face>& boreFaces,
        const gp_Dir& holeFeatureAxis,
        const gp_Pnt& axisLocationPnt,
        double& outMinProj,
        double& outMaxProj
    );

    std::unique_ptr<Gui::ViewProviderTextureExtension> textureExtension;

    // --- Coin3D Nodes for Rendering ---
    SoTextureCoordinate2* textureCoords;
    SoTexture2* boreTextureNode;
    SoMaterial* holeMaterialNode;
    SoMaterial* boreFacesMaterialNode;
    SoTextureCoordinate2* boreFacesTextureCoords;
    SoIndexedFaceSet* boreIndexedFaceSet;
    SoCoordinate3* boreFaceCoordinates;
    SoSeparator* boreFacesSeparator;
    SoIndexedFaceSet* faceset;
    SoNormal* boreNormals;
    SoNormalBinding* boreNormalBinding;
    SoClipPlane* boreEndClipPlane;
};

} // namespace PartDesignGui

#endif // PARTGUI_ViewProviderHole_H
