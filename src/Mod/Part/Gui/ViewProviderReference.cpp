/***************************************************************************
 *   Copyright (c) 2004 Juergen Riegel <juergen.riegel@web.de>             *
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
// to avoid compiler warnings of redefining contents of basic.h
// later by #include "ViewProvider.h"
# define _USE_MATH_DEFINES
# include <cmath>

# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoShapeHints.h>
#endif

#include <App/Document.h>
#include <Base/Parameter.h>

#include "ViewProvider.h"
#include "ViewProviderReference.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderPartReference, Gui::ViewProviderGeometryObject)

//**************************************************************************
// Construction/Destruction

ViewProviderPartReference::ViewProviderPartReference()
{
    App::Material mat;
    mat.ambientColor.set(0.2f,0.2f,0.2f);
    mat.diffuseColor.set(0.1f,0.1f,0.1f);
    mat.specularColor.set(0.0f,0.0f,0.0f);
    mat.emissiveColor.set(0.0f,0.0f,0.0f);
    mat.shininess = 0.0f;
    mat.transparency = 0.0f;
    //ADD_PROPERTY(LineMaterial,(mat));
    //ADD_PROPERTY(PointMaterial,(mat));
    //ADD_PROPERTY(LineColor,(mat.diffuseColor));
    //ADD_PROPERTY(PointColor,(mat.diffuseColor));
    //ADD_PROPERTY(LineWidth,(2.0f));
    //LineWidth.setConstraints(&floatRange);
    //PointSize.setConstraints(&floatRange);
    //ADD_PROPERTY(PointSize,(2.0f));
    //ADD_PROPERTY(ControlPoints,(false));
    //ADD_PROPERTY(Lighting,(1));
    //Lighting.setEnums(LightingEnums);

    //EdgeRoot = new SoSeparator();
    //EdgeRoot->ref();
    //FaceRoot = new SoSeparator();
    //FaceRoot->ref();
    //VertexRoot = new SoSeparator();
    //VertexRoot->ref();
    //pcLineMaterial = new SoMaterial;
    //pcLineMaterial->ref();
    //LineMaterial.touch();

    //pcPointMaterial = new SoMaterial;
    //pcPointMaterial->ref();
    //PointMaterial.touch();

    //pcLineStyle = new SoDrawStyle();
    //pcLineStyle->ref();
    //pcLineStyle->style = SoDrawStyle::LINES;
    //pcLineStyle->lineWidth = LineWidth.getValue();

    //pcPointStyle = new SoDrawStyle();
    //pcPointStyle->ref();
    //pcPointStyle->style = SoDrawStyle::POINTS;
    //pcPointStyle->pointSize = PointSize.getValue();

    //pShapeHints = new SoShapeHints;
    //pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    //pShapeHints->ref();
    //Lighting.touch();

    sPixmap = "Part_3D_object";
    //loadParameter();
}

ViewProviderPartReference::~ViewProviderPartReference()
{
    //EdgeRoot->unref();
    //FaceRoot->unref();
    //VertexRoot->unref();
    //pcLineMaterial->unref();
    //pcPointMaterial->unref();
    //pcLineStyle->unref();
    //pcPointStyle->unref();
    //pShapeHints->unref();
}

void ViewProviderPartReference::onChanged(const App::Property* /*prop*/)
{
    //if (prop == &LineWidth) {
    //    pcLineStyle->lineWidth = LineWidth.getValue();
    //}
    //else if (prop == &PointSize) {
    //    pcPointStyle->pointSize = PointSize.getValue();
    //}
    //else if (prop == &LineColor) {
    //    const App::Color& c = LineColor.getValue();
    //    pcLineMaterial->diffuseColor.setValue(c.r,c.g,c.b);
    //    if (c != LineMaterial.getValue().diffuseColor)
    //    LineMaterial.setDiffuseColor(c);
    //}
    //else if (prop == &PointColor) {
    //    const App::Color& c = PointColor.getValue();
    //    pcPointMaterial->diffuseColor.setValue(c.r,c.g,c.b);
    //    if (c != PointMaterial.getValue().diffuseColor)
    //    PointMaterial.setDiffuseColor(c);
    //}
    //else if (prop == &LineMaterial) {
    //    const App::Material& Mat = LineMaterial.getValue();
    //    if (LineColor.getValue() != Mat.diffuseColor)
    //    LineColor.setValue(Mat.diffuseColor);
    //    pcLineMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
    //    pcLineMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
    //    pcLineMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
    //    pcLineMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
    //    pcLineMaterial->shininess.setValue(Mat.shininess);
    //    pcLineMaterial->transparency.setValue(Mat.transparency);
    //}
    //else if (prop == &PointMaterial) {
    //    const App::Material& Mat = PointMaterial.getValue();
    //    if (PointColor.getValue() != Mat.diffuseColor)
    //    PointColor.setValue(Mat.diffuseColor);
    //    pcPointMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
    //    pcPointMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
    //    pcPointMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
    //    pcPointMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
    //    pcPointMaterial->shininess.setValue(Mat.shininess);
    //    pcPointMaterial->transparency.setValue(Mat.transparency);
    //}
    //else if (prop == &ControlPoints) {
    //    App::DocumentObject* obj = this->pcObject;
    //    App::Property* shape = obj->getPropertyByName("Shape");
    //    showControlPoints(ControlPoints.getValue(), shape);
    //}
    //else if (prop == &Lighting) {
    //    if (Lighting.getValue() == 0)
    //        pShapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
    //    else
    //        pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    //}
    //else {
    //    ViewProviderGeometryObject::onChanged(prop);
    //}
}

void ViewProviderPartReference::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);

    SoGroup* pcNormalRoot = new SoGroup();
    //SoGroup* pcFlatRoot = new SoGroup();
    //SoGroup* pcWireframeRoot = new SoGroup();
    //SoGroup* pcPointsRoot = new SoGroup();

    // enable two-side rendering
    pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    // normal viewing with edges and points
    pcNormalRoot->addChild(pShapeHints);
    pcNormalRoot->addChild(FaceRoot);
    pcNormalRoot->addChild(EdgeRoot);
    pcNormalRoot->addChild(VertexRoot);

    // just faces with no edges or points
    //pcFlatRoot->addChild(pShapeHints);
    //pcFlatRoot->addChild(FaceRoot);

    // only edges
    //pcWireframeRoot->addChild(EdgeRoot);
    //pcWireframeRoot->addChild(VertexRoot);

    // normal viewing with edges and points
    //pcPointsRoot->addChild(VertexRoot);

    // putting all together with the switch
    addDisplayMaskMode(pcNormalRoot, "Reference");
}

void ViewProviderPartReference::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Reference",ModeName)==0 )
        setDisplayMaskMode("Reference");

    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderPartReference::getDisplayModes() const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.emplace_back("Flat Lines");
    StrList.emplace_back("Shaded");
    StrList.emplace_back("Wireframe");
    StrList.emplace_back("Points");

    return StrList;
}


void ViewProviderPartReference::updateData(const App::Property* )
{
}
