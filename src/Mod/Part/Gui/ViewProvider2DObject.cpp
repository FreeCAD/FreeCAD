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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Standard_math.hxx>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoVertexProperty.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Parameter.h>
#include <Base/ViewProj.h>

#include "ViewProvider2DObject.h"
#include <Mod/Part/App/PartFeature.h>


//#include "Tree.h"



using namespace PartGui;
using namespace std;


//**************************************************************************
// Construction/Destruction

const char* ViewProvider2DObject::GridStyleEnums[]= {"Dashed","Light",NULL};

PROPERTY_SOURCE(PartGui::ViewProvider2DObject, PartGui::ViewProviderPart)

ViewProvider2DObject::ViewProvider2DObject()
{
    ADD_PROPERTY_TYPE(ShowGrid,(false),"Grid",(App::PropertyType)(App::Prop_None),"Switch the grid on/off");
    ADD_PROPERTY_TYPE(GridSize,(10),"Grid",(App::PropertyType)(App::Prop_None),"Gap size of the grid");
    ADD_PROPERTY_TYPE(GridStyle,((long)0),"Grid",(App::PropertyType)(App::Prop_None),"Appearence style of the grid");
    ADD_PROPERTY_TYPE(TightGrid,(true),"Grid",(App::PropertyType)(App::Prop_None),"Switch the tight grid mode on/off");
    ADD_PROPERTY_TYPE(GridSnap,(false),"Grid",(App::PropertyType)(App::Prop_None),"Switch the grid snap on/off");

    GridRoot = new SoSeparator();
    GridRoot->ref();
    MinX = MinY = -100;
    MaxX = MaxY = 100;
    GridStyle.setEnums(GridStyleEnums);

    pcRoot->addChild(GridRoot);
 
    sPixmap = "PartFeatureImport";
}

ViewProvider2DObject::~ViewProvider2DObject()
{
     GridRoot->unref();
}


// **********************************************************************************

SoSeparator* ViewProvider2DObject::createGrid(void)
{
    //double dx = 10 * GridSize.getValue();                       // carpet size
    //double dy = 10 * GridSize.getValue();
    // float Size = (MaxX-MinX > MaxY-MinY) ? MaxX-MinX : MaxY-MinY;
    float Step = GridSize.getValue(); //pow(10,floor(log10(Size/5.0)));
    float MiX, MaX, MiY, MaY;
    if (TightGrid.getValue()) {
        MiX = MinX - (MaxX-MinX)*0.2f;
        MaX = MaxX + (MaxX-MinX)*0.2f;
        MiY = MinY - (MaxY-MinY)*0.2f;
        MaY = MaxY + (MaxY-MinY)*0.2f;
    }
    else {
        MiX = -exp(ceil(log(std::abs(MinX))));
        MiX = std::min<float>(MiX,(float)-exp(ceil(log(std::abs(0.1f*MaxX)))));
        MaX = exp(ceil(log(std::abs(MaxX))));
        MaX = std::max<float>(MaX,(float)exp(ceil(log(std::abs(0.1f*MinX)))));
        MiY = -exp(ceil(log(std::abs(MinY))));
        MiY = std::min<float>(MiY,(float)-exp(ceil(log(std::abs(0.1f*MaxY)))));
        MaY = exp(ceil(log(std::abs(MaxY))));
        MaY = std::max<float>(MaY,(float)exp(ceil(log(std::abs(0.1f*MinY)))));
    }
    //Round the values otherwise grid is not aligned with center
    MiX = floor(MiX / Step) * Step;
    MaX = ceil(MaX / Step) * Step;
    MiY = floor(MiY / Step) * Step;
    MaY = ceil(MaY / Step) * Step;

    double zGrid = 0.0;                     // carpet-grid separation

    SoSeparator *parent = GridRoot;
    GridRoot->removeAllChildren();
    SoBaseColor *mycolor;
    SoVertexProperty *vts;

   // carpet
 /* mycolor = new SoBaseColor;
    mycolor->rgb.setValue(0.2f, 0.7f, 0.7f);
    parent->addChild(mycolor);

    vts = new SoVertexProperty;
    vts->vertex.set1Value(0, -0.5*dx, -1.5*dy,  0.5*zGrid);
    vts->vertex.set1Value(1, -0.5*dx, -1.5*dy, -0.5*zGrid);
    vts->vertex.set1Value(2,  0.5*dx, -1.5*dy,  0.5*zGrid);
    vts->vertex.set1Value(3,  0.5*dx, -1.5*dy, -0.5*zGrid);

    SoQuadMesh *carpet = new SoQuadMesh;
    carpet->verticesPerColumn = 2;
    carpet->verticesPerRow = 2;
    carpet->vertexProperty = vts;
    parent->addChild(carpet);*/

    // gridlines
    mycolor = new SoBaseColor;

    mycolor->rgb.setValue(0.7f, 0.7f ,0.7f);
    parent->addChild(mycolor);

    SoDrawStyle* DefaultStyle = new SoDrawStyle;
    DefaultStyle->lineWidth = 1;
    DefaultStyle->linePattern = 0x0f0f;

    SoMaterial* LightStyle = new SoMaterial;
    LightStyle->transparency = 0.7f;

    if (GridStyle.getValue() == 0)
      parent->addChild(DefaultStyle);
    else
      parent->addChild(LightStyle);

    SoPickStyle* PickStyle = new SoPickStyle;
    PickStyle->style = SoPickStyle::UNPICKABLE;
    parent->addChild(PickStyle);

    SoLineSet *grid = new SoLineSet;
    vts = new SoVertexProperty;
    grid->vertexProperty = vts;

    int vi=0, l=0;

    // vertical lines
    float i;
    for (i=MiX; i<MaX; i+=Step) {
        /*float h=-0.5*dx + float(i) / gridsize * dx;*/
        vts->vertex.set1Value(vi++, i, MiY, zGrid);
        vts->vertex.set1Value(vi++, i,  MaY, zGrid);
        grid->numVertices.set1Value(l++, 2);
    }

    // horizontal lines
    for (i=MiY; i<MaY; i+=Step) {
        //float v=-0.5*dy + float(i) / gridsize * dy;
        vts->vertex.set1Value(vi++, MiX, i, zGrid);
        vts->vertex.set1Value(vi++,  MaX, i, zGrid);
        grid->numVertices.set1Value(l++, 2);
    }
    parent->addChild(vts);
    parent->addChild(grid);

    return parent;
}

void ViewProvider2DObject::updateData(const App::Property* prop)
{
    ViewProviderPart::updateData(prop);

    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        Base::BoundBox3d bbox = static_cast<const Part::PropertyPartShape*>(prop)->getBoundingBox();
        GridRoot->removeAllChildren();
        if (!bbox.IsValid()) return;
        Base::Placement place = static_cast<const Part::PropertyPartShape*>(prop)->getComplexData()->getPlacement();
        place.invert();
        Base::ViewProjMatrix proj(place.toMatrix());
        Base::BoundBox2D bbox2d = bbox.ProjectBox(&proj);
        this->MinX = bbox2d.fMinX;
        this->MaxX = bbox2d.fMaxX;
        this->MinY = bbox2d.fMinY;
        this->MaxY = bbox2d.fMaxY;
        if (ShowGrid.getValue()) {
            createGrid();
        }
    }
}

void ViewProvider2DObject::onChanged(const App::Property* prop)
{
    // call father
    ViewProviderPart::onChanged(prop);

    if (prop == &ShowGrid) {
        if (ShowGrid.getValue())
            createGrid();
        else
            GridRoot->removeAllChildren();
    }
    if ((prop == &GridSize) || (prop == &GridStyle) || (prop == &TightGrid)) {
        if (ShowGrid.getValue()) {
            GridRoot->removeAllChildren();
            createGrid();
        }
    }
}

void ViewProvider2DObject::attach(App::DocumentObject *pcFeat)
{
    ViewProviderPart::attach(pcFeat);

    if (ShowGrid.getValue())
        createGrid();
}

bool ViewProvider2DObject::setEdit(int)
{
    return false;
}

void ViewProvider2DObject::unsetEdit(int)
{

}

std::vector<std::string> ViewProvider2DObject::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Flat Lines");
    //StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}

const char* ViewProvider2DObject::getDefaultDisplayMode() const
{
  return "Wireframe";
}

// -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProvider2DObjectPython, PartGui::ViewProvider2DObject)
/// @endcond

// explicit template instantiation
template class PartGuiExport ViewProviderPythonFeatureT<PartGui::ViewProvider2DObject>;
}
