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
# include <Python.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoDepthBuffer.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <cfloat>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <Base/ViewProj.h>
#include <App/Application.h>
#include <Gui/SoFCBoundingBox.h>

#include "ViewProvider2DObject.h"
#include <Mod/Part/App/PartFeature.h>


using namespace PartGui;
using namespace std;


//**************************************************************************
// Construction/Destruction

const char* ViewProvider2DObject::GridStyleEnums[]= {"Dashed","Light",NULL};
App::PropertyQuantityConstraint::Constraints ViewProvider2DObject::GridSizeRange = {0.001,DBL_MAX,1.0};

PROPERTY_SOURCE(PartGui::ViewProvider2DObject, PartGui::ViewProviderPart)

ViewProvider2DObject::ViewProvider2DObject()
{
    ADD_PROPERTY_TYPE(ShowGrid,(false),"Grid",(App::PropertyType)(App::Prop_None),"Switch the grid on/off");
    ADD_PROPERTY_TYPE(GridSize,(10),"Grid",(App::PropertyType)(App::Prop_None),"Gap size of the grid");
    ADD_PROPERTY_TYPE(GridStyle,((long)0),"Grid",(App::PropertyType)(App::Prop_None),"Appearance style of the grid");
    ADD_PROPERTY_TYPE(TightGrid,(true),"Grid",(App::PropertyType)(App::Prop_None),"Switch the tight grid mode on/off");
    ADD_PROPERTY_TYPE(GridSnap,(false),"Grid",(App::PropertyType)(App::Prop_None),"Switch the grid snap on/off");

    GridRoot = new SoSeparator();
    GridRoot->ref();
    GridRoot->setName("GridRoot");
    MinX = MinY = -100;
    MaxX = MaxY = 100;
    GridStyle.setEnums(GridStyleEnums);
    GridSize.setConstraints(&GridSizeRange);

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
        // make sure that nine of the numbers are exactly zero because log(0)
        // is not defined
        float xMin = std::abs(MinX) < FLT_EPSILON ? 0.01f : MinX;
        float xMax = std::abs(MaxX) < FLT_EPSILON ? 0.01f : MaxX;
        float yMin = std::abs(MinY) < FLT_EPSILON ? 0.01f : MinY;
        float yMax = std::abs(MaxY) < FLT_EPSILON ? 0.01f : MaxY;
        MiX = -exp(ceil(log(std::abs(xMin))));
        MiX = std::min<float>(MiX,(float)-exp(ceil(log(std::abs(0.1f*xMax)))));
        MaX = exp(ceil(log(std::abs(xMax))));
        MaX = std::max<float>(MaX,(float)exp(ceil(log(std::abs(0.1f*xMin)))));
        MiY = -exp(ceil(log(std::abs(yMin))));
        MiY = std::min<float>(MiY,(float)-exp(ceil(log(std::abs(0.1f*yMax)))));
        MaY = exp(ceil(log(std::abs(yMax))));
        MaY = std::max<float>(MaY,(float)exp(ceil(log(std::abs(0.1f*yMin)))));
    }
    //Round the values otherwise grid is not aligned with center
    MiX = floor(MiX / Step) * Step;
    MaX = ceil(MaX / Step) * Step;
    MiY = floor(MiY / Step) * Step;
    MaY = ceil(MaY / Step) * Step;

    double zGrid = 0.0;                     // carpet-grid separation

    SoGroup *parent = new Gui::SoSkipBoundingGroup();
    GridRoot->removeAllChildren();
    GridRoot->addChild(parent);
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

    SoDepthBuffer *depth = new SoDepthBuffer;
    depth->function = SoDepthBuffer::ALWAYS;
    parent->addChild(depth);

    // gridlines
    mycolor = new SoBaseColor;
    mycolor->rgb.setValue(0.7f, 0.7f ,0.7f);
    parent->addChild(mycolor);

    if (GridStyle.getValue() == 0) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part");
        int pattern = hGrp->GetInt("GridLinePattern", 0x0f0f);
        SoDrawStyle* DefaultStyle = new SoDrawStyle;
        DefaultStyle->lineWidth = 1;
        DefaultStyle->linePattern = pattern;
        parent->addChild(DefaultStyle);
    }
    else {
        SoMaterial* LightStyle = new SoMaterial;
        LightStyle->transparency = 0.7f;
        parent->addChild(LightStyle);
    }

    SoPickStyle* PickStyle = new SoPickStyle;
    PickStyle->style = SoPickStyle::UNPICKABLE;
    parent->addChild(PickStyle);

    SoLineSet *grid = new SoLineSet;
    vts = new SoVertexProperty;
    grid->vertexProperty = vts;

    // vertical lines
    int vlines = static_cast<int>((MaX - MiX) / Step + 0.5f);

    // horizontal lines
    int hlines = static_cast<int>((MaY - MiY) / Step + 0.5f);

    int lines = vlines + hlines;

    // set the grid indices
    grid->numVertices.setNum(lines);
    int32_t* vertices = grid->numVertices.startEditing();
    for (int i=0; i<lines; i++)
        vertices[i] = 2;
    grid->numVertices.finishEditing();

    // set the grid coordinates
    vts->vertex.setNum(2*lines);
    SbVec3f* vertex_coords = vts->vertex.startEditing();

    // vertical lines
    int i_offset_x = static_cast<int>(MiX / Step);
    for (int i=0; i<vlines; i++) {
        vertex_coords[2*i].setValue((i+i_offset_x)*Step, MiY, zGrid);
        vertex_coords[2*i+1].setValue((i+i_offset_x)*Step, MaY, zGrid);
    }

    // horizontal lines
    int i_offset_y = static_cast<int>(MiY / Step);
    for (int i=vlines; i<lines; i++) {
        vertex_coords[2*i].setValue(MiX, (i-vlines+i_offset_y)*Step, zGrid);
        vertex_coords[2*i+1].setValue(MaX, (i-vlines+i_offset_y)*Step, zGrid);
    }
    vts->vertex.finishEditing();

    parent->addChild(vts);
    parent->addChild(grid);

    return GridRoot;
}

void ViewProvider2DObject::updateData(const App::Property* prop)
{
    ViewProviderPart::updateData(prop);

    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        Base::BoundBox3d bbox = static_cast<const Part::PropertyPartShape*>(prop)->getBoundingBox();
        if (!bbox.IsValid()) return;
        GridRoot->removeAllChildren();
        Base::Placement place = static_cast<const Part::PropertyPartShape*>(prop)->getComplexData()->getPlacement();
        place.invert();
        Base::ViewProjMatrix proj(place.toMatrix());
        Base::BoundBox2d bbox2d = bbox.ProjectBox(&proj);
        this->MinX = bbox2d.MinX;
        this->MaxX = bbox2d.MaxX;
        this->MinY = bbox2d.MinY;
        this->MaxY = bbox2d.MaxY;
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

void ViewProvider2DObject::Restore(Base::XMLReader &reader)
{
    ViewProviderPart::Restore(reader);
}

void ViewProvider2DObject::handleChangedPropertyType(Base::XMLReader &reader,
                                                     const char * TypeName,
                                                     App::Property * prop)
{
    Base::Type inputType = Base::Type::fromName(TypeName);
    if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
        inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
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
