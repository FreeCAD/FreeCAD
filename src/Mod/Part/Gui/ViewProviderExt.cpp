// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <juergen.riegel@web.de>             *
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

#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Version.hxx>
#include <TColgp_Array1OfDir.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <QAction>
#include <QMenu>
#include <sstream>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>

#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/TimeInfo.h>
#include <Base/Tools.h>

#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Selection/SoFCSelectionAction.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Gui/ViewParams.h>
#include <Gui/Utilities.h>

#include <Mod/Part/App/ShapeMapHasher.h>
#include <Mod/Part/App/Tools.h>

#include "ViewProviderExt.h"
#include "ViewProviderPartExtPy.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"
#include "TaskFaceAppearances.h"


FC_LOG_LEVEL_INIT("Part", true, true)

using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderPartExt, Gui::ViewProviderGeometryObject)


//**************************************************************************
// Construction/Destruction

App::PropertyFloatConstraint::Constraints ViewProviderPartExt::sizeRange = {1.0, 64.0, 1.0};
App::PropertyFloatConstraint::Constraints ViewProviderPartExt::tessRange = {0.01, 100.0, 0.01};
App::PropertyQuantityConstraint::Constraints ViewProviderPartExt::angDeflectionRange
    = {1.0, 180.0, 0.05};
const char* ViewProviderPartExt::LightingEnums[] = {"One side", "Two side", nullptr};
const char* ViewProviderPartExt::DrawStyleEnums[] = {"Solid", "Dashed", "Dotted", "Dashdot", nullptr};

ViewProviderPartExt::ViewProviderPartExt()
{
    texture.initExtension(this);

    VisualTouched = true;
    forceUpdateCount = 0;
    NormalsFromUV = true;

    // get default line color
    unsigned long lcol = Gui::ViewParams::instance()->getDefaultShapeLineColor();  // dark grey
                                                                                   // (25,25,25)
    float lr, lg, lb;
    lr = ((lcol >> 24) & 0xff) / 255.0;
    lg = ((lcol >> 16) & 0xff) / 255.0;
    lb = ((lcol >> 8) & 0xff) / 255.0;
    // get default vertex color
    unsigned long vcol = Gui::ViewParams::instance()->getDefaultShapeVertexColor();
    float vr, vg, vb;
    vr = ((vcol >> 24) & 0xff) / 255.0;
    vg = ((vcol >> 16) & 0xff) / 255.0;
    vb = ((vcol >> 8) & 0xff) / 255.0;
    int lwidth = Gui::ViewParams::instance()->getDefaultShapeLineWidth();
    int psize = Gui::ViewParams::instance()->getDefaultShapePointSize();


    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Part"
    );
    NormalsFromUV = hPart->GetBool("NormalsFromUVNodes", NormalsFromUV);

    long twoside = hPart->GetBool("TwoSideRendering", true) ? 1 : 0;

    // Let the user define a custom lower limit but a value less than
    // OCCT's epsilon is not allowed
    double lowerLimit = hPart->GetFloat("MinimumDeviation", tessRange.LowerBound);
    lowerLimit = std::max(lowerLimit, Precision::Confusion());
    tessRange.LowerBound = lowerLimit;

    static const char* osgroup = "Object Style";

    App::Material lmat;
    lmat.ambientColor.set(0.2f, 0.2f, 0.2f);
    lmat.diffuseColor.set(lr, lg, lb);
    lmat.specularColor.set(0.0f, 0.0f, 0.0f);
    lmat.emissiveColor.set(0.0f, 0.0f, 0.0f);
    lmat.shininess = 1.0f;
    lmat.transparency = 0.0f;

    App::Material vmat;
    vmat.ambientColor.set(0.2f, 0.2f, 0.2f);
    vmat.diffuseColor.set(vr, vg, vb);
    vmat.specularColor.set(0.0f, 0.0f, 0.0f);
    vmat.emissiveColor.set(0.0f, 0.0f, 0.0f);
    vmat.shininess = 1.0f;
    vmat.transparency = 0.0f;

    ADD_PROPERTY_TYPE(LineMaterial, (lmat), osgroup, App::Prop_None, "Object line material.");
    ADD_PROPERTY_TYPE(PointMaterial, (vmat), osgroup, App::Prop_None, "Object point material.");
    ADD_PROPERTY_TYPE(LineColor, (lmat.diffuseColor), osgroup, App::Prop_None, "Set object line color.");
    ADD_PROPERTY_TYPE(PointColor, (vmat.diffuseColor), osgroup, App::Prop_None, "Set object point color");
    ADD_PROPERTY_TYPE(
        PointColorArray,
        (PointColor.getValue()),
        osgroup,
        App::Prop_None,
        "Object point color array."
    );
    ADD_PROPERTY_TYPE(
        LineColorArray,
        (LineColor.getValue()),
        osgroup,
        App::Prop_None,
        "Object line color array."
    );
    ADD_PROPERTY_TYPE(LineWidth, (lwidth), osgroup, App::Prop_None, "Set object line width.");
    LineWidth.setConstraints(&sizeRange);
    PointSize.setConstraints(&sizeRange);
    ADD_PROPERTY_TYPE(PointSize, (psize), osgroup, App::Prop_None, "Set object point size.");
    ADD_PROPERTY_TYPE(
        Deviation,
        (0.5f),
        osgroup,
        App::Prop_None,
        "Sets the accuracy of the polygonal representation of the model\n"
        "in the 3D view (tessellation). Lower values indicate better quality.\n"
        "The value is in percent of object's size."
    );
    Deviation.setConstraints(&tessRange);
    ADD_PROPERTY_TYPE(
        AngularDeflection,
        (28.5),
        osgroup,
        App::Prop_None,
        "Specify how finely to generate the mesh for rendering on screen or when exporting.\n"
        "The default value is 28.5 degrees, or 0.5 radians. The smaller the value\n"
        "the smoother the appearance in the 3D view, and the finer the mesh that will be exported."
    );
    AngularDeflection.setConstraints(&angDeflectionRange);
    ADD_PROPERTY_TYPE(Lighting, (twoside), osgroup, App::Prop_None, "Set object lighting.");
    Lighting.setEnums(LightingEnums);
    ADD_PROPERTY_TYPE(
        DrawStyle,
        ((long int)0),
        osgroup,
        App::Prop_None,
        "Defines the style of the edges in the 3D view."
    );
    DrawStyle.setEnums(DrawStyleEnums);
    ADD_PROPERTY_TYPE(
        ShowPlacement,
        (false),
        "Display Options",
        App::Prop_None,
        "If true, placement of object is additionally rendered."
    );

    coords = new SoCoordinate3();
    coords->ref();
    faceset = new SoBrepFaceSet();
    faceset->setViewProvider(this);
    faceset->ref();
    norm = new SoNormal;
    norm->ref();
    normb = new SoNormalBinding;
    normb->value = SoNormalBinding::PER_VERTEX_INDEXED;
    normb->ref();
    lineset = new SoBrepEdgeSet();
    lineset->setViewProvider(this);
    lineset->ref();
    nodeset = new SoBrepPointSet();
    nodeset->setViewProvider(this);
    nodeset->ref();

    pcFaceBind = new SoMaterialBinding();
    pcFaceBind->ref();
    pcFaceBind->setName("FaceBind");

    pcLineBind = new SoMaterialBinding();
    pcLineBind->ref();
    pcLineBind->setName("LineBind");
    pcLineMaterial = new SoMaterial;
    pcLineMaterial->ref();
    pcLineMaterial->setName("LineMaterial");
    LineMaterial.touch();

    pcPointBind = new SoMaterialBinding();
    pcPointBind->ref();
    pcPointBind->setName("PointBind");
    pcPointMaterial = new SoMaterial;
    pcPointMaterial->ref();
    pcPointMaterial->setName("PointMaterial");
    PointMaterial.touch();

    pcLineStyle = new SoDrawStyle();
    pcLineStyle->ref();
    pcLineStyle->style = SoDrawStyle::LINES;
    pcLineStyle->lineWidth = LineWidth.getValue();
    pcLineStyle->setName("LineStyle");

    pcPointStyle = new SoDrawStyle();
    pcPointStyle->ref();
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = PointSize.getValue();
    pcPointStyle->setName("PointStyle");

    pShapeHints = new SoShapeHints;
    pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pShapeHints->ref();
    Lighting.touch();
    DrawStyle.touch();

    sPixmap = "Part_3D_object";
    loadParameter();
}

ViewProviderPartExt::~ViewProviderPartExt()
{
    pcFaceBind->unref();
    pcLineBind->unref();
    pcPointBind->unref();
    pcLineMaterial->unref();
    pcPointMaterial->unref();
    pcLineStyle->unref();
    pcPointStyle->unref();
    pShapeHints->unref();
    coords->unref();
    faceset->unref();
    norm->unref();
    normb->unref();
    lineset->unref();
    nodeset->unref();
}

PyObject* ViewProviderPartExt::getPyObject()
{
    if (!pyViewObject) {
        pyViewObject = new ViewProviderPartExtPy(this);
    }
    pyViewObject->IncRef();
    return pyViewObject;
}

void ViewProviderPartExt::onChanged(const App::Property* prop)
{
    // The lower limit of the deviation has been increased to avoid
    // to freeze the GUI
    // https://forum.freecad.org/viewtopic.php?f=3&t=24912&p=195613
    if (prop == &Deviation) {
        if (isUpdateForced() || Visibility.getValue()) {
            updateVisual();
        }
        else {
            VisualTouched = true;
        }
    }
    if (prop == &AngularDeflection) {
        if (isUpdateForced() || Visibility.getValue()) {
            updateVisual();
        }
        else {
            VisualTouched = true;
        }
    }
    if (prop == &LineWidth) {
        pcLineStyle->lineWidth = LineWidth.getValue();
    }
    else if (prop == &PointSize) {
        pcPointStyle->pointSize = PointSize.getValue();
    }
    else if (prop == &LineColor) {
        const Base::Color& c = LineColor.getValue();
        pcLineMaterial->diffuseColor.setValue(c.r, c.g, c.b);
        if (c != LineMaterial.getValue().diffuseColor) {
            LineMaterial.setDiffuseColor(c);
        }
        LineColorArray.setValue(LineColor.getValue());
    }
    else if (prop == &PointColor) {
        const Base::Color& c = PointColor.getValue();
        pcPointMaterial->diffuseColor.setValue(c.r, c.g, c.b);
        if (c != PointMaterial.getValue().diffuseColor) {
            PointMaterial.setDiffuseColor(c);
        }
        PointColorArray.setValue(PointColor.getValue());
    }
    else if (prop == &LineMaterial) {
        const App::Material& Mat = LineMaterial.getValue();
        if (LineColor.getValue() != Mat.diffuseColor) {
            LineColor.setValue(Mat.diffuseColor);
        }
        pcLineMaterial->ambientColor
            .setValue(Mat.ambientColor.r, Mat.ambientColor.g, Mat.ambientColor.b);
        pcLineMaterial->diffuseColor
            .setValue(Mat.diffuseColor.r, Mat.diffuseColor.g, Mat.diffuseColor.b);
        pcLineMaterial->specularColor
            .setValue(Mat.specularColor.r, Mat.specularColor.g, Mat.specularColor.b);
        pcLineMaterial->emissiveColor
            .setValue(Mat.emissiveColor.r, Mat.emissiveColor.g, Mat.emissiveColor.b);
        pcLineMaterial->shininess.setValue(Mat.shininess);
        pcLineMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &PointMaterial) {
        const App::Material& Mat = PointMaterial.getValue();
        if (PointColor.getValue() != Mat.diffuseColor) {
            PointColor.setValue(Mat.diffuseColor);
        }
        pcPointMaterial->ambientColor
            .setValue(Mat.ambientColor.r, Mat.ambientColor.g, Mat.ambientColor.b);
        pcPointMaterial->diffuseColor
            .setValue(Mat.diffuseColor.r, Mat.diffuseColor.g, Mat.diffuseColor.b);
        pcPointMaterial->specularColor
            .setValue(Mat.specularColor.r, Mat.specularColor.g, Mat.specularColor.b);
        pcPointMaterial->emissiveColor
            .setValue(Mat.emissiveColor.r, Mat.emissiveColor.g, Mat.emissiveColor.b);
        pcPointMaterial->shininess.setValue(Mat.shininess);
        pcPointMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &PointColorArray) {
        setHighlightedPoints(PointColorArray.getValues());
    }
    else if (prop == &LineColorArray) {
        setHighlightedEdges(LineColorArray.getValues());
    }
    else if (prop == &_diffuseColor) {
        // Used to load the old DiffuseColor values asynchronously
        std::vector<Base::Color> colors = _diffuseColor.getValues();
        std::vector<float> transparencies;
        transparencies.resize(static_cast<int>(colors.size()));
        for (int i = 0; i < static_cast<int>(colors.size()); i++) {
            transparencies[i] = colors[i].transparency();
            colors[i].a = 1.0F;
        }
        ShapeAppearance.setDiffuseColors(colors);
        ShapeAppearance.setTransparencies(transparencies);
    }
    else if (prop == &ShapeAppearance) {
        setHighlightedFaces(ShapeAppearance);
        ViewProviderGeometryObject::onChanged(prop);
    }
    else if (prop == &Transparency) {
        const App::Material& Mat = ShapeAppearance[0];
        long value = Base::toPercent(Mat.transparency);
        if (value != Transparency.getValue()) {
            float trans = Base::fromPercent(Transparency.getValue());
            ShapeAppearance.setTransparency(trans);
        }
    }
    else if (prop == &Lighting) {
        if (Lighting.getValue() == 0) {
            pShapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        }
        else {
            pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        }
    }
    else if (prop == &DrawStyle) {
        if (DrawStyle.getValue() == 0) {
            pcLineStyle->linePattern = 0xffff;
        }
        else if (DrawStyle.getValue() == 1) {
            pcLineStyle->linePattern = 0xf00f;
        }
        else if (DrawStyle.getValue() == 2) {
            pcLineStyle->linePattern = 0x0f0f;
        }
        else {
            pcLineStyle->linePattern = 0xff88;
        }
    }
    else if (prop == &ShowPlacement) {
        pcPlacement->whichChild = (ShowPlacement.getValue() && Visibility.getValue())
            ? SO_SWITCH_ALL
            : SO_SWITCH_NONE;
    }
    else {
        // if the object was invisible and has been changed, recreate the visual
        if (prop == &Visibility && (isUpdateForced() || Visibility.getValue()) && VisualTouched) {
            updateVisual();
            // updateVisual() may not be triggered by any change (e.g.
            // triggered by an external object through forceUpdate()). And
            // since ShapeAppearance is not changed here either, do not falsely set
            // the document modified status
            Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
                App::Property::NoModify,
                &ShapeAppearance
            );
            // The material has to be checked again (#0001736)
            onChanged(&ShapeAppearance);
            onChanged(&ShowPlacement);
        }
    }

    ViewProviderGeometryObject::onChanged(prop);
}

bool ViewProviderPartExt::allowOverride(const App::DocumentObject&) const
{
    // Many derived view providers still uses static_cast to get object
    // pointer, so check for exact type here.
    return is<ViewProviderPartExt>();
}

void ViewProviderPartExt::attach(App::DocumentObject* pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);

    // Workaround for #0000433, i.e. use SoSeparator instead of SoGroup
    auto* pcNormalRoot = new SoSeparator();
    pcNormalRoot->setName("NormalRoot");
    auto* pcFlatRoot = new SoSeparator();
    pcFlatRoot->setName("FlatRoot");
    auto* pcWireframeRoot = new SoSeparator();
    pcWireframeRoot->setName("WireframeRoot");
    auto* pcPointsRoot = new SoSeparator();
    pcPointsRoot->setName("PointsRoot");
    auto* wireframe = new SoSeparator();

    // Must turn off all intermediate render caching, and let pcRoot to handle
    // cache without interference.
    pcNormalRoot->renderCaching = pcFlatRoot->renderCaching = pcWireframeRoot->renderCaching
        = pcPointsRoot->renderCaching = wireframe->renderCaching = SoSeparator::OFF;

    pcNormalRoot->boundingBoxCaching = pcFlatRoot->boundingBoxCaching = pcWireframeRoot->boundingBoxCaching
        = pcPointsRoot->boundingBoxCaching = wireframe->boundingBoxCaching = SoSeparator::OFF;

    // Avoid any Z-buffer artifacts, so that the lines always appear on top of the faces
    // The correct order is Edges, Polygon offset, Faces.
    SoPolygonOffset* offset = new SoPolygonOffset();

    // wireframe node
    wireframe->setName("Edge");
    wireframe->addChild(pcLineBind);
    wireframe->addChild(pcLineMaterial);
    wireframe->addChild(pcLineStyle);
    wireframe->addChild(lineset);

    // normal viewing with edges and points
    pcNormalRoot->addChild(pcPointsRoot);
    pcNormalRoot->addChild(offset);
    pcNormalRoot->addChild(pcFlatRoot);
    pcNormalRoot->addChild(wireframe);

    // just faces with no edges or points
    pcFlatRoot->addChild(pShapeHints);
    pcFlatRoot->addChild(pcFaceBind);
    pcFlatRoot->addChild(texture.getAppearance());
    texture.setup(pcShapeMaterial);
    SoDrawStyle* pcFaceStyle = new SoDrawStyle();
    pcFaceStyle->setName("FaceStyle");
    pcFaceStyle->style = SoDrawStyle::FILLED;
    pcFlatRoot->addChild(pcFaceStyle);
    pcFlatRoot->addChild(norm);
    pcFlatRoot->addChild(normb);
    pcFlatRoot->addChild(faceset);

    // edges and points
    pcWireframeRoot->addChild(wireframe);
    pcWireframeRoot->addChild(pcPointsRoot);

    // normal viewing with edges and points
    pcPointsRoot->addChild(pcPointBind);
    pcPointsRoot->addChild(pcPointMaterial);
    pcPointsRoot->addChild(pcPointStyle);
    pcPointsRoot->addChild(nodeset);

    // Move 'coords' before the switch
    pcRoot->insertChild(coords, pcRoot->findChild(pcModeSwitch));

    // putting all together with the switch
    addDisplayMaskMode(pcNormalRoot, "Flat Lines");
    addDisplayMaskMode(pcFlatRoot, "Shaded");
    addDisplayMaskMode(pcWireframeRoot, "Wireframe");
    addDisplayMaskMode(pcPointsRoot, "Point");
}

void ViewProviderPartExt::setDisplayMode(const char* ModeName)
{
    if (strcmp("Flat Lines", ModeName) == 0) {
        setDisplayMaskMode("Flat Lines");
    }
    else if (strcmp("Shaded", ModeName) == 0) {
        setDisplayMaskMode("Shaded");
    }
    else if (strcmp("Wireframe", ModeName) == 0) {
        setDisplayMaskMode("Wireframe");
    }
    else if (strcmp("Points", ModeName) == 0) {
        setDisplayMaskMode("Point");
    }

    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderPartExt::getDisplayModes() const
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

std::string ViewProviderPartExt::getElement(const SoDetail* detail) const
{
    std::stringstream str;
    if (detail) {
        if (detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
            const SoFaceDetail* face_detail = static_cast<const SoFaceDetail*>(detail);
            int face = face_detail->getPartIndex() + 1;
            str << "Face" << face;
        }
        else if (detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
            int edge = line_detail->getLineIndex() + 1;
            str << "Edge" << edge;
        }
        else if (detail->getTypeId() == SoPointDetail::getClassTypeId()) {
            const SoPointDetail* point_detail = static_cast<const SoPointDetail*>(detail);
            int vertex = point_detail->getCoordinateIndex() - nodeset->startIndex.getValue() + 1;
            str << "Vertex" << vertex;
        }
    }

    return str.str();
}

SoDetail* ViewProviderPartExt::getDetail(const char* subelement) const
{
    // 1. Try standard string parsing (FaceN, EdgeN...)
    auto type = Part::TopoShape::getElementTypeAndIndex(subelement);
    std::string element = type.first;
    int index = type.second;

    // 2. Create the Coin3D Detail
    if (element == "Face") {
        SoFaceDetail* detail = new SoFaceDetail();
        detail->setPartIndex(index - 1);
        return detail;
    }
    else if (element == "Edge") {
        SoLineDetail* detail = new SoLineDetail();
        detail->setLineIndex(index - 1);
        return detail;
    }
    else if (element == "Vertex") {
        SoPointDetail* detail = new SoPointDetail();
        static_cast<SoPointDetail*>(detail)->setCoordinateIndex(
            index + nodeset->startIndex.getValue() - 1
        );
        return detail;
    }

    return nullptr;
}

std::vector<Base::Vector3d> ViewProviderPartExt::getModelPoints(const SoPickedPoint* pp) const
{
    try {
        std::vector<Base::Vector3d> pts;
        std::string element = this->getElement(pp->getDetail());
        const auto& shape = getRenderedShape();

        TopoDS_Shape subShape = shape.getSubShape(element.c_str());

        // get the point of the vertex directly
        if (subShape.ShapeType() == TopAbs_VERTEX) {
            const TopoDS_Vertex& v = TopoDS::Vertex(subShape);
            gp_Pnt p = BRep_Tool::Pnt(v);
            pts.emplace_back(p.X(), p.Y(), p.Z());
        }
        // get the nearest point on the edge
        else if (subShape.ShapeType() == TopAbs_EDGE) {
            const SbVec3f& vec = pp->getPoint();
            BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(vec[0], vec[1], vec[2]));
            BRepExtrema_DistShapeShape distSS(subShape, mkVert.Vertex(), 0.1);
            if (distSS.NbSolution() > 0) {
                gp_Pnt p = distSS.PointOnShape1(1);
                pts.emplace_back(p.X(), p.Y(), p.Z());
            }
        }
        // get the nearest point on the face
        else if (subShape.ShapeType() == TopAbs_FACE) {
            const SbVec3f& vec = pp->getPoint();
            BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(vec[0], vec[1], vec[2]));
            BRepExtrema_DistShapeShape distSS(subShape, mkVert.Vertex(), 0.1);
            if (distSS.NbSolution() > 0) {
                gp_Pnt p = distSS.PointOnShape1(1);
                pts.emplace_back(p.X(), p.Y(), p.Z());
            }
        }

        return pts;
    }
    catch (...) {
    }

    // if something went wrong returns an empty array
    return {};
}

std::vector<Base::Vector3d> ViewProviderPartExt::getSelectionShape(const char* /*Element*/) const
{
    return {};
}

void ViewProviderPartExt::setHighlightedFaces(const std::vector<App::Material>& materials)
{
    if (getObject() && getObject()->testStatus(App::ObjectStatus::TouchOnColorChange)) {
        getObject()->touch(true);
    }

    Gui::SoUpdateVBOAction action;
    action.apply(this->faceset);

    int size = static_cast<int>(materials.size());
    if (size > 1 && size == this->faceset->partIndex.getNum()) {
        pcFaceBind->value = SoMaterialBinding::PER_PART;
        texture.activateMaterial();

        pcShapeMaterial->diffuseColor.setNum(size);
        pcShapeMaterial->ambientColor.setNum(size);
        pcShapeMaterial->specularColor.setNum(size);
        pcShapeMaterial->emissiveColor.setNum(size);
        pcShapeMaterial->shininess.setNum(size);
        pcShapeMaterial->transparency.setNum(size);

        SbColor* dc = pcShapeMaterial->diffuseColor.startEditing();
        SbColor* ac = pcShapeMaterial->ambientColor.startEditing();
        SbColor* sc = pcShapeMaterial->specularColor.startEditing();
        SbColor* ec = pcShapeMaterial->emissiveColor.startEditing();
        float* sh = pcShapeMaterial->shininess.startEditing();
        float* tr = pcShapeMaterial->transparency.startEditing();

        for (int i = 0; i < size; i++) {
            dc[i].setValue(
                materials[i].diffuseColor.r,
                materials[i].diffuseColor.g,
                materials[i].diffuseColor.b
            );
            ac[i].setValue(
                materials[i].ambientColor.r,
                materials[i].ambientColor.g,
                materials[i].ambientColor.b
            );
            sc[i].setValue(
                materials[i].specularColor.r,
                materials[i].specularColor.g,
                materials[i].specularColor.b
            );
            ec[i].setValue(
                materials[i].emissiveColor.r,
                materials[i].emissiveColor.g,
                materials[i].emissiveColor.b
            );
            sh[i] = materials[i].shininess;
            tr[i] = materials[i].transparency;
        }

        pcShapeMaterial->diffuseColor.finishEditing();
        pcShapeMaterial->ambientColor.finishEditing();
        pcShapeMaterial->specularColor.finishEditing();
        pcShapeMaterial->emissiveColor.finishEditing();
        pcShapeMaterial->shininess.finishEditing();
        pcShapeMaterial->transparency.finishEditing();
    }
    else if (size == 1) {
        pcFaceBind->value = SoMaterialBinding::OVERALL;
        setCoinAppearance(materials[0]);
    }
}

void ViewProviderPartExt::setHighlightedFaces(const App::PropertyMaterialList& appearance)
{
    setHighlightedFaces(appearance.getValues());
}

std::map<std::string, Base::Color> ViewProviderPartExt::getElementColors(const char* element) const
{
    std::map<std::string, Base::Color> ret;

    if (!element || !element[0]) {
        auto color = ShapeAppearance.getDiffuseColor();
        color.setTransparency(Base::fromPercent(Transparency.getValue()));
        ret["Face"] = color;
        ret["Edge"] = LineColor.getValue();
        ret["Vertex"] = PointColor.getValue();
        return ret;
    }

    if (boost::starts_with(element, "Face")) {
        auto size = ShapeAppearance.getSize();
        if (element[4] == '*') {
            auto color = ShapeAppearance.getDiffuseColor();
            color.setTransparency(Base::fromPercent(Transparency.getValue()));
            bool singleColor = true;
            for (int i = 0; i < size; ++i) {
                Base::Color faceColor = ShapeAppearance.getDiffuseColor(i);
                faceColor.setTransparency(ShapeAppearance.getTransparency(i));
                if (faceColor != color) {
                    ret[std::string(element, 4) + std::to_string(i + 1)] = faceColor;
                }
                Base::Color firstFaceColor = ShapeAppearance.getDiffuseColor(0);
                firstFaceColor.setTransparency(ShapeAppearance.getTransparency(0));
                singleColor = singleColor && (faceColor == firstFaceColor);
            }
            if (size > 0 && singleColor) {
                color = ShapeAppearance.getDiffuseColor(0);
                color.setTransparency(ShapeAppearance.getTransparency(0));
                ret.clear();
            }
            ret["Face"] = color;
        }
        else {
            int idx = atoi(element + 4);
            if (idx > 0 && idx <= size) {
                ret[element] = ShapeAppearance.getDiffuseColor(idx - 1);
            }
            else {
                ret[element] = ShapeAppearance.getDiffuseColor();
            }
            if (size == 1) {
                ret[element].setTransparency(Base::fromPercent(Transparency.getValue()));
            }
        }
    }
    else if (boost::starts_with(element, "Edge")) {
        auto size = LineColorArray.getSize();
        if (element[4] == '*') {
            auto color = LineColor.getValue();
            bool singleColor = true;
            for (int i = 0; i < size; ++i) {
                if (LineColorArray[i] != color) {
                    ret[std::string(element, 4) + std::to_string(i + 1)] = LineColorArray[i];
                }
                singleColor = singleColor && LineColorArray[0] == LineColorArray[i];
            }
            if (singleColor && size) {
                color = LineColorArray[0];
                ret.clear();
            }
            ret["Edge"] = color;
        }
        else {
            int idx = atoi(element + 4);
            if (idx > 0 && idx <= size) {
                ret[element] = LineColorArray[idx - 1];
            }
            else {
                ret[element] = LineColor.getValue();
            }
        }
    }
    else if (boost::starts_with(element, "Vertex")) {
        auto size = PointColorArray.getSize();
        if (element[5] == '*') {
            auto color = PointColor.getValue();
            bool singleColor = true;
            for (int i = 0; i < size; ++i) {
                if (PointColorArray[i] != color) {
                    ret[std::string(element, 5) + std::to_string(i + 1)] = PointColorArray[i];
                }
                singleColor = singleColor && PointColorArray[0] == PointColorArray[i];
            }
            if (singleColor && size) {
                color = PointColorArray[0];
                ret.clear();
            }
            ret["Vertex"] = color;
        }
        else {
            int idx = atoi(element + 5);
            if (idx > 0 && idx <= size) {
                ret[element] = PointColorArray[idx - 1];
            }
            else {
                ret[element] = PointColor.getValue();
            }
        }
    }
    return ret;
}

void ViewProviderPartExt::unsetHighlightedFaces()
{
    ShapeAppearance.touch();
    Transparency.touch();
}

void ViewProviderPartExt::setHighlightedEdges(const std::vector<Base::Color>& colors)
{
    if (getObject() && getObject()->testStatus(App::ObjectStatus::TouchOnColorChange)) {
        getObject()->touch(true);
    }
    int size = static_cast<int>(colors.size());
    if (size > 1) {
        // Although indexed lineset is used the material binding must be PER_FACE!
        pcLineBind->value = SoMaterialBinding::PER_FACE;
        const int32_t* cindices = this->lineset->coordIndex.getValues(0);
        int numindices = this->lineset->coordIndex.getNum();
        pcLineMaterial->diffuseColor.setNum(size);
        SbColor* ca = pcLineMaterial->diffuseColor.startEditing();
        int linecount = 0;

        for (int i = 0; i < numindices; ++i) {
            if (cindices[i] < 0) {
                ca[linecount].setValue(colors[linecount].r, colors[linecount].g, colors[linecount].b);
                linecount++;
                if (linecount >= size) {
                    break;
                }
            }
        }

        pcLineMaterial->diffuseColor.finishEditing();
    }
    else if (size == 1) {
        pcLineBind->value = SoMaterialBinding::OVERALL;
        pcLineMaterial->diffuseColor.setValue(colors[0].r, colors[0].g, colors[0].b);
    }
}

void ViewProviderPartExt::unsetHighlightedEdges()
{
    pcLineBind->value = SoMaterialBinding::OVERALL;
    LineMaterial.touch();
}

void ViewProviderPartExt::setHighlightedPoints(const std::vector<Base::Color>& colors)
{
    if (getObject() && getObject()->testStatus(App::ObjectStatus::TouchOnColorChange)) {
        getObject()->touch(true);
    }
    int size = static_cast<int>(colors.size());
    if (size > 1) {
        pcPointBind->value = SoMaterialBinding::PER_VERTEX;
        pcPointMaterial->diffuseColor.setNum(size);
        SbColor* ca = pcPointMaterial->diffuseColor.startEditing();
        for (int i = 0; i < size; ++i) {
            ca[i].setValue(colors[i].r, colors[i].g, colors[i].b);
        }
        pcPointMaterial->diffuseColor.finishEditing();
    }
    else if (size == 1) {
        pcPointBind->value = SoMaterialBinding::OVERALL;
        pcPointMaterial->diffuseColor.setValue(colors[0].r, colors[0].g, colors[0].b);
    }
}

void ViewProviderPartExt::unsetHighlightedPoints()
{
    PointColorArray.touch();
}

bool ViewProviderPartExt::loadParameter()
{
    bool changed = false;
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Part"
    );
    float deviation = hGrp->GetFloat("MeshDeviation", 0.2);
    float angularDeflection = hGrp->GetFloat("MeshAngularDeflection", 28.65);
    NormalsFromUV = hGrp->GetBool("NormalsFromUVNodes", NormalsFromUV);

    if (Deviation.getValue() != deviation) {
        Deviation.setValue(deviation);
        changed = true;
    }
    if (AngularDeflection.getValue() != angularDeflection) {
        AngularDeflection.setValue(angularDeflection);
    }

    return changed;
}

void ViewProviderPartExt::reload()
{
    if (loadParameter()) {
        updateVisual();
    }
}

void ViewProviderPartExt::updateData(const App::Property* prop)
{
    const char* propName = prop->getName();
    if (propName && (strcmp(propName, "Shape") == 0 || strstr(propName, "Touched"))) {
        // calculate the visual only if visible
        if (isUpdateForced() || Visibility.getValue()) {
            updateVisual();
        }
        else {
            VisualTouched = true;
        }

        if (!VisualTouched) {
            if (this->faceset->partIndex.getNum() > this->pcShapeMaterial->diffuseColor.getNum()) {
                this->pcFaceBind->value = SoMaterialBinding::OVERALL;
            }
        }
    }
    Gui::ViewProviderGeometryObject::updateData(prop);
}

void ViewProviderPartExt::startRestoring()
{
    Gui::ViewProviderGeometryObject::startRestoring();
}

void ViewProviderPartExt::finishRestoring()
{
    // The ShapeAppearance property is restored after DiffuseColor
    // and currently sets a single color.
    // In case DiffuseColor has defined multiple colors they will
    // be passed to the scene graph now.
    if (_diffuseColor.getSize() > 1) {
        onChanged(&_diffuseColor);
    }
    Gui::ViewProviderGeometryObject::finishRestoring();
}

void ViewProviderPartExt::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Part_ColorFace.svg"));
    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
    QAction* act = menu->addAction(iconObject, QObject::tr("Appearance per Face"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Color));
}

bool ViewProviderPartExt::changeFaceAppearances()
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        Gui::Control().showDialog(dlg);
        return false;
    }

    Gui::Selection().clearSelection();
    Gui::Control().showDialog(new TaskFaceAppearances(this));
    return true;
}

bool ViewProviderPartExt::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        return changeFaceAppearances();
    }
    else {
        return Gui::ViewProviderGeometryObject::setEdit(ModNum);
    }
}

void ViewProviderPartExt::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        // Do nothing here
    }
    else {
        Gui::ViewProviderGeometryObject::unsetEdit(ModNum);
    }
}

void ViewProviderPartExt::setupCoinGeometry(
    TopoDS_Shape shape,
    SoCoordinate3* coords,
    SoBrepFaceSet* faceset,
    SoNormal* norm,
    SoBrepEdgeSet* lineset,
    SoBrepPointSet* nodeset,
    double deviation,
    double angularDeflection,
    bool normalsFromUV
)
{
    if (Part::Tools::isShapeEmpty(shape)) {
        coords->point.setNum(0);
        norm->vector.setNum(0);
        faceset->coordIndex.setNum(0);
        faceset->partIndex.setNum(0);
        lineset->coordIndex.setNum(0);
        nodeset->startIndex.setValue(0);
        return;
    }

    // time measurement and book keeping
    Base::TimeElapsed startTime;

    [[maybe_unused]]
    int numTriangles
        = 0,
        numNodes = 0, numNorms = 0, numFaces = 0, numEdges = 0, numLines = 0;

    std::set<int> faceEdges;

    // calculating the deflection value
    Standard_Real deflection = Part::Tools::getDeflection(shape, deviation);

    // Since OCCT 7.6 a value of equal 0 is not allowed any more, this can happen if a single
    // vertex should be displayed.
    if (deflection < gp::Resolution()) {
        deflection = Precision::Confusion();
    }

    // For very big objects the computed deflection can become very high and thus leads to a
    // useless tessellation. To avoid this the upper limit is set to 20.0 See also forum:
    // https://forum.freecad.org/viewtopic.php?t=77521
    // deflection = std::min(deflection, 20.0);

    // create or use the mesh on the data structure
    Standard_Real AngDeflectionRads = Base::toRadians(angularDeflection);

    IMeshTools_Parameters meshParams;
    meshParams.Deflection = deflection;
    meshParams.Relative = Standard_False;
    meshParams.Angle = AngDeflectionRads;
    meshParams.InParallel = Standard_True;
    meshParams.AllowQualityDecrease = Standard_True;

    // Clear triangulation and PCurves from geometry which can slow down the process
#if OCC_VERSION_HEX < 0x070600
    BRepTools::Clean(shape);
#else
    BRepTools::Clean(shape, Standard_True);
#endif

    BRepMesh_IncrementalMesh(shape, meshParams);

    // We must reset the location here because the transformation data
    // are set in the placement property
    TopLoc_Location aLoc;
    shape.Location(aLoc);

    // count triangles and nodes in the mesh
    TopTools_IndexedMapOfShape faceMap;
    TopExp::MapShapes(shape, TopAbs_FACE, faceMap);
    for (int i = 1; i <= faceMap.Extent(); i++) {
        Handle(Poly_Triangulation) mesh = BRep_Tool::Triangulation(TopoDS::Face(faceMap(i)), aLoc);

        if (mesh.IsNull()) {
            mesh = Part::Tools::triangulationOfFace(TopoDS::Face(faceMap(i)));
        }

        // Note: we must also count empty faces
        if (!mesh.IsNull()) {
            numTriangles += mesh->NbTriangles();
            numNodes += mesh->NbNodes();
            numNorms += mesh->NbNodes();
        }

        TopExp_Explorer xp;
        for (xp.Init(faceMap(i), TopAbs_EDGE); xp.More(); xp.Next()) {
            faceEdges.insert(Part::ShapeMapHasher {}(xp.Current()));
        }
        numFaces++;
    }

    // get an indexed map of edges
    TopTools_IndexedMapOfShape edgeMap;
    TopExp::MapShapes(shape, TopAbs_EDGE, edgeMap);

    // key is the edge number, value the coord indexes. This is needed to keep the same order as
    // the edges.
    std::map<int, std::vector<int32_t>> lineSetMap;
    std::set<int> edgeIdxSet;
    std::vector<int32_t> edgeVector;

    // count and index the edges
    for (int i = 1; i <= edgeMap.Extent(); i++) {
        edgeIdxSet.insert(i);
        numEdges++;

        const TopoDS_Edge& aEdge = TopoDS::Edge(edgeMap(i));
        TopLoc_Location aLoc;

        // handling of the free edge that are not associated to a face
        // Note: The assumption that if for an edge BRep_Tool::Polygon3D
        // returns a valid object is wrong. This e.g. happens for ruled
        // surfaces which gets created by two edges or wires.
        // So, we have to store the hashes of the edges associated to a face.
        // If the hash of a given edge is not in this list we know it's really
        // a free edge.
        int hash = Part::ShapeMapHasher {}(aEdge);
        if (faceEdges.find(hash) == faceEdges.end()) {
            Handle(Poly_Polygon3D) aPoly = Part::Tools::polygonOfEdge(aEdge, aLoc);
            if (!aPoly.IsNull()) {
                int nbNodesInEdge = aPoly->NbNodes();
                numNodes += nbNodesInEdge;
            }
        }
    }

    // handling of the vertices
    TopTools_IndexedMapOfShape vertexMap;
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertexMap);
    numNodes += vertexMap.Extent();

    // create memory for the nodes and indexes
    coords->point.setNum(numNodes);
    norm->vector.setNum(numNorms);
    faceset->coordIndex.setNum(numTriangles * 4);
    faceset->partIndex.setNum(numFaces);

    // get the raw memory for fast fill up
    SbVec3f* verts = coords->point.startEditing();
    SbVec3f* norms = norm->vector.startEditing();
    int32_t* index = faceset->coordIndex.startEditing();
    int32_t* parts = faceset->partIndex.startEditing();

    // preset the normal vector with null vector
    for (int i = 0; i < numNorms; i++) {
        norms[i] = SbVec3f(0.0, 0.0, 0.0);
    }

    int ii = 0, faceNodeOffset = 0, faceTriaOffset = 0;
    for (int i = 1; i <= faceMap.Extent(); i++, ii++) {
        TopLoc_Location aLoc;
        const TopoDS_Face& actFace = TopoDS::Face(faceMap(i));
        // get the mesh of the shape
        Handle(Poly_Triangulation) mesh = BRep_Tool::Triangulation(actFace, aLoc);
        if (mesh.IsNull()) {
            mesh = Part::Tools::triangulationOfFace(actFace);
        }
        if (mesh.IsNull()) {
            parts[ii] = 0;
            continue;
        }

        // getting the transformation of the shape/face
        gp_Trsf myTransf;
        Standard_Boolean identity = true;
        if (!aLoc.IsIdentity()) {
            identity = false;
            myTransf = aLoc.Transformation();
        }

        // getting size of node and triangle array of this face
        int nbNodesInFace = mesh->NbNodes();
        int nbTriInFace = mesh->NbTriangles();
        // check orientation
        TopAbs_Orientation orient = actFace.Orientation();

        // cycling through the poly mesh
#if OCC_VERSION_HEX < 0x070600
        const Poly_Array1OfTriangle& Triangles = mesh->Triangles();
        const TColgp_Array1OfPnt& Nodes = mesh->Nodes();
        TColgp_Array1OfDir Normals(Nodes.Lower(), Nodes.Upper());
#else
        int numNodes = mesh->NbNodes();
        TColgp_Array1OfDir Normals(1, numNodes);
#endif
        if (normalsFromUV) {
            Part::Tools::getPointNormals(actFace, mesh, Normals);
        }

        for (int g = 1; g <= nbTriInFace; g++) {
            // Get the triangle
            Standard_Integer N1, N2, N3;
#if OCC_VERSION_HEX < 0x070600
            Triangles(g).Get(N1, N2, N3);
#else
            mesh->Triangle(g).Get(N1, N2, N3);
#endif

            // change orientation of the triangle if the face is reversed
            if (orient != TopAbs_FORWARD) {
                Standard_Integer tmp = N1;
                N1 = N2;
                N2 = tmp;
            }

            // get the 3 points of this triangle
#if OCC_VERSION_HEX < 0x070600
            gp_Pnt V1(Nodes(N1)), V2(Nodes(N2)), V3(Nodes(N3));
#else
            gp_Pnt V1(mesh->Node(N1)), V2(mesh->Node(N2)), V3(mesh->Node(N3));
#endif

            // get the 3 normals of this triangle
            gp_Vec NV1, NV2, NV3;
            if (normalsFromUV) {
                NV1.SetXYZ(Normals(N1).XYZ());
                NV2.SetXYZ(Normals(N2).XYZ());
                NV3.SetXYZ(Normals(N3).XYZ());
            }
            else {
                gp_Vec v1 = Base::convertTo<gp_Vec>(V1);
                gp_Vec v2 = Base::convertTo<gp_Vec>(V2);
                gp_Vec v3 = Base::convertTo<gp_Vec>(V3);

                gp_Vec normal = (v2 - v1) ^ (v3 - v1);
                NV1 = normal;
                NV2 = normal;
                NV3 = normal;
            }

            // transform the vertices and normals to the place of the face
            if (!identity) {
                V1.Transform(myTransf);
                V2.Transform(myTransf);
                V3.Transform(myTransf);
                if (normalsFromUV) {
                    NV1.Transform(myTransf);
                    NV2.Transform(myTransf);
                    NV3.Transform(myTransf);
                }
            }

            // add the normals for all points of this triangle
            norms[faceNodeOffset + N1 - 1] += Base::convertTo<SbVec3f>(NV1);
            norms[faceNodeOffset + N2 - 1] += Base::convertTo<SbVec3f>(NV2);
            norms[faceNodeOffset + N3 - 1] += Base::convertTo<SbVec3f>(NV3);

            // set the vertices
            verts[faceNodeOffset + N1 - 1] = Base::convertTo<SbVec3f>(V1);
            verts[faceNodeOffset + N2 - 1] = Base::convertTo<SbVec3f>(V2);
            verts[faceNodeOffset + N3 - 1] = Base::convertTo<SbVec3f>(V3);

            // set the index vector with the 3 point indexes and the end delimiter
            index[faceTriaOffset * 4 + 4 * (g - 1)] = faceNodeOffset + N1 - 1;
            index[faceTriaOffset * 4 + 4 * (g - 1) + 1] = faceNodeOffset + N2 - 1;
            index[faceTriaOffset * 4 + 4 * (g - 1) + 2] = faceNodeOffset + N3 - 1;
            index[faceTriaOffset * 4 + 4 * (g - 1) + 3] = SO_END_FACE_INDEX;
        }

        parts[ii] = nbTriInFace;  // new part

        // handling the edges lying on this face
        TopExp_Explorer Exp;
        for (Exp.Init(actFace, TopAbs_EDGE); Exp.More(); Exp.Next()) {
            const TopoDS_Edge& curEdge = TopoDS::Edge(Exp.Current());
            // get the overall index of this edge
            int edgeIndex = edgeMap.FindIndex(curEdge);
            edgeVector.push_back((int32_t)edgeIndex - 1);
            // already processed this index ?
            if (edgeIdxSet.find(edgeIndex) != edgeIdxSet.end()) {

                // this holds the indices of the edge's triangulation to the current polygon
                Handle(Poly_PolygonOnTriangulation) aPoly
                    = BRep_Tool::PolygonOnTriangulation(curEdge, mesh, aLoc);
                if (aPoly.IsNull()) {
                    continue;  // polygon does not exist
                }

                // getting the indexes of the edge polygon
                const TColStd_Array1OfInteger& indices = aPoly->Nodes();
                for (Standard_Integer i = indices.Lower(); i <= indices.Upper(); i++) {
                    int nodeIndex = indices(i);
                    int index = faceNodeOffset + nodeIndex - 1;
                    lineSetMap[edgeIndex].push_back(index);

                    // usually the coordinates for this edge are already set by the
                    // triangles of the face this edge belongs to. However, there are
                    // rare cases where some points are only referenced by the polygon
                    // but not by any triangle. Thus, we must apply the coordinates to
                    // make sure that everything is properly set.
#if OCC_VERSION_HEX < 0x070600
                    gp_Pnt p(Nodes(nodeIndex));
#else
                    gp_Pnt p(mesh->Node(nodeIndex));
#endif
                    if (!identity) {
                        p.Transform(myTransf);
                    }
                    verts[index] = Base::convertTo<SbVec3f>(p);
                }

                // remove the handled edge index from the set
                edgeIdxSet.erase(edgeIndex);
            }
        }

        edgeVector.push_back(-1);

        // counting up the per Face offsets
        faceNodeOffset += nbNodesInFace;
        faceTriaOffset += nbTriInFace;
    }

    // handling of the free edges
    for (int i = 1; i <= edgeMap.Extent(); i++) {
        const TopoDS_Edge& aEdge = TopoDS::Edge(edgeMap(i));
        Standard_Boolean identity = true;
        gp_Trsf myTransf;
        TopLoc_Location aLoc;

        // handling of the free edge that are not associated to a face
        int hash = Part::ShapeMapHasher {}(aEdge);
        if (faceEdges.find(hash) == faceEdges.end()) {
            Handle(Poly_Polygon3D) aPoly = Part::Tools::polygonOfEdge(aEdge, aLoc);
            if (!aPoly.IsNull()) {
                if (!aLoc.IsIdentity()) {
                    identity = false;
                    myTransf = aLoc.Transformation();
                }

                const TColgp_Array1OfPnt& aNodes = aPoly->Nodes();
                int nbNodesInEdge = aPoly->NbNodes();

                gp_Pnt pnt;
                for (Standard_Integer j = 1; j <= nbNodesInEdge; j++) {
                    pnt = aNodes(j);
                    if (!identity) {
                        pnt.Transform(myTransf);
                    }
                    int index = faceNodeOffset + j - 1;
                    verts[index] = Base::convertTo<SbVec3f>(pnt);
                    lineSetMap[i].push_back(index);
                }

                faceNodeOffset += nbNodesInEdge;
            }
        }
    }

    nodeset->startIndex.setValue(faceNodeOffset);
    for (int i = 0; i < vertexMap.Extent(); i++) {
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(vertexMap(i + 1));
        gp_Pnt pnt = BRep_Tool::Pnt(aVertex);

        verts[faceNodeOffset + i] = Base::convertTo<SbVec3f>(pnt);
    }

    // normalize all normals
    for (int i = 0; i < numNorms; i++) {
        norms[i].normalize();
    }

    std::vector<int32_t> lineSetCoords;
    for (const auto& it : lineSetMap) {
        lineSetCoords.insert(lineSetCoords.end(), it.second.begin(), it.second.end());
        lineSetCoords.push_back(-1);
    }

    // preset the index vector size
    numLines = lineSetCoords.size();
    lineset->coordIndex.setNum(numLines);
    int32_t* lines = lineset->coordIndex.startEditing();

    int l = 0;
    for (auto it = lineSetCoords.begin(); it != lineSetCoords.end(); ++it, l++) {
        lines[l] = *it;
    }

    // end the editing of the nodes
    coords->point.finishEditing();
    norm->vector.finishEditing();
    faceset->coordIndex.finishEditing();
    faceset->partIndex.finishEditing();
    lineset->coordIndex.finishEditing();

#ifdef FC_DEBUG
    Base::Console().log(
        "ViewProvider update time: %f s\n",
        Base::TimeElapsed::diffTimeF(startTime, Base::TimeElapsed())
    );
    Base::Console().log(
        "Shape mesh info: Faces:%d Edges:%d Nodes:%d Triangles:%d IdxVec:%d\n",
        numFaces,
        numEdges,
        numNodes,
        numTriangles,
        numLines
    );
#endif
}

void ViewProviderPartExt::setupCoinGeometry(
    TopoDS_Shape shape,
    SoFCShape* node,
    double deviation,
    double angularDeflection,
    bool normalsFromUV
)
{
    setupCoinGeometry(
        shape,
        node->coords,
        node->faceset,
        node->norm,
        node->lineset,
        node->nodeset,
        deviation,
        angularDeflection,
        normalsFromUV
    );
}

void ViewProviderPartExt::updateVisual()
{
    Gui::SoUpdateVBOAction action;
    action.apply(this->faceset);

    // Clear selection
    Gui::SoSelectionElementAction saction(Gui::SoSelectionElementAction::None);
    saction.apply(this->faceset);
    saction.apply(this->lineset);
    saction.apply(this->nodeset);

    // Clear highlighting
    Gui::SoHighlightElementAction haction;
    haction.apply(this->faceset);
    haction.apply(this->lineset);
    haction.apply(this->nodeset);

    try {
        TopoDS_Shape cShape = getRenderedShape().getShape();

        setupCoinGeometry(
            cShape,
            coords,
            faceset,
            norm,
            lineset,
            nodeset,
            Deviation.getValue(),
            AngularDeflection.getValue(),
            NormalsFromUV
        );

        VisualTouched = false;
    }
    catch (const Standard_Failure& e) {
        FC_ERR(
            "Cannot compute Inventor representation for the shape of "
            << pcObject->getFullName() << ": " << e.GetMessageString()
        );
    }
    catch (...) {
        FC_ERR("Cannot compute Inventor representation for the shape of " << pcObject->getFullName());
    }

    // The material has to be checked again
    setHighlightedFaces(ShapeAppearance.getValues());
    setHighlightedEdges(LineColorArray.getValues());
    setHighlightedPoints(PointColorArray.getValue());
}

void ViewProviderPartExt::forceUpdate(bool enable)
{
    if (enable) {
        if (++forceUpdateCount == 1) {
            if (!isShow() && VisualTouched) {
                updateVisual();
            }
        }
    }
    else if (forceUpdateCount) {
        --forceUpdateCount;
    }
}


void ViewProviderPartExt::handleChangedPropertyName(
    Base::XMLReader& reader,
    const char* TypeName,
    const char* PropName
)
{
    if (strcmp(PropName, "DiffuseColor") == 0
        && strcmp(TypeName, App::PropertyColorList::getClassTypeId().getName()) == 0) {

        // PropertyColorLists are loaded asynchronously as they're stored in separate files
        _diffuseColor.Restore(reader);
    }
    else {
        Gui::ViewProviderGeometryObject::handleChangedPropertyName(reader, TypeName, PropName);
    }
}
