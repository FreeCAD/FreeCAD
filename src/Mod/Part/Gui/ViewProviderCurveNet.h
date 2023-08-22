/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_VIEWPROVIDERCURVENET_H
#define PARTGUI_VIEWPROVIDERCURVENET_H

#include "ViewProvider.h"


class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbVec3f;
class SoTransform;

namespace Gui {
  class View3DInventorViewer;
  class SoFCSelection;
}

namespace PartGui {


class PartGuiExport ViewProviderCurveNet:public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderPart);

public:
    /// constructor
    ViewProviderCurveNet();
    /// destructor
    ~ViewProviderCurveNet() override;

    void attach(App::DocumentObject *) override;
    void setDisplayMode(const char* ModeName) override;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes() const override;

    /// Update the Part representation
    void updateData(const App::Property*) override;

    virtual bool handleEvent(const SoEvent * const ev,Gui::View3DInventorViewer &Viewer);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

    struct Node {
        Gui::SoFCSelection  *pcHighlight;
        SoTransform    *pcTransform;
        Node() {
            pcHighlight = nullptr;
            pcTransform = nullptr;
        }
    };

    std::list<Node> NodeList;

    bool bInEdit{false};
    bool bMovePointMode{false};
    Node PointToMove;
    /// root of the edge and vertex points
    SoSeparator *EdgeRoot{nullptr}, *VertexRoot{nullptr};

    Standard_Boolean computeEdges   (SoSeparator* root, const TopoDS_Shape &myShape);
    Standard_Boolean computeVertices(SoSeparator* root, const TopoDS_Shape &myShape);
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERCURVENET_H

