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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <gp_Trsf.hxx>
# include <Precision.hxx>
# include <Poly_Array1OfTriangle.hxx>
# include <Poly_Polygon3D.hxx>
# include <Poly_PolygonOnTriangulation.hxx>
# include <Poly_Triangulation.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array1OfDir.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TopExp_Explorer.hxx>
# include <TopExp.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopTools_IndexedMapOfShape.hxx>

# include <QAction>
# include <QMenu>
# include <sstream>

# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoNormalBinding.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <QApplication>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MappedElement.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/TimeInfo.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewParams.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProviderLink.h>
#include <Gui/TaskElementColors.h>
#include <Gui/ViewParams.h>
#include <Mod/Part/App/Tools.h>

#include "ViewProviderExt.h"
#include "PartParams.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"
#include "ViewProviderPartExtPy.h"

FC_LOG_LEVEL_INIT("Part", true, true)

using namespace PartGui;
namespace bio = boost::iostreams;

PROPERTY_SOURCE(PartGui::ViewProviderPartExt, Gui::ViewProviderGeometryObject)


namespace PartGui {

// Private class used by ViewProviderExt to update its visual nodes up on
// receiving SoGetBoundingBoxAction
class SoFCCoordinate3: public SoCoordinate3
{
public:
    virtual void getBoundingBox(SoGetBoundingBoxAction * action) {
        if (vp && vp->VisualTouched)
            vp->updateVisual();
        SoCoordinate3::getBoundingBox(action);
    }

    ViewProviderPartExt *vp = nullptr;
};

} // namespace PartGui

//**************************************************************************
// Construction/Destruction

App::PropertyFloatConstraint::Constraints ViewProviderPartExt::sizeRange = {1.0,64.0,1.0};
App::PropertyFloatConstraint::Constraints ViewProviderPartExt::tessRange = {0.01,100.0,0.01};
App::PropertyQuantityConstraint::Constraints ViewProviderPartExt::angDeflectionRange = {1.0,180.0,0.05};
const char* ViewProviderPartExt::LightingEnums[]= {"One side","Two side",nullptr};
const char* ViewProviderPartExt::DrawStyleEnums[]= {"Solid","Dashed","Dotted","Dashdot",nullptr};

ViewProviderPartExt::ViewProviderPartExt() 
{
    UpdatingColor = false;
    VisualTouched = true;
    forceUpdateCount = 0;
    NormalsFromUV = true;

    // get default line color
    unsigned long lcol = Gui::ViewParams::instance()->getDefaultShapeLineColor(); // dark grey (25,25,25)
    float lr,lg,lb;
    lr = ((lcol >> 24) & 0xff) / 255.0; lg = ((lcol >> 16) & 0xff) / 255.0; lb = ((lcol >> 8) & 0xff) / 255.0;
    // get default vertex color
    unsigned long vcol = Gui::ViewParams::instance()->getDefaultShapeVertexColor(); 
    float vr,vg,vb;
    vr = ((vcol >> 24) & 0xff) / 255.0; vg = ((vcol >> 16) & 0xff) / 255.0; vb = ((vcol >> 8) & 0xff) / 255.0;
    int lwidth = Gui::ViewParams::instance()->getDefaultShapeLineWidth();
    int psize = Gui::ViewParams::instance()->getDefaultShapePointSize();
	


    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Part");
    NormalsFromUV = hPart->GetBool("NormalsFromUVNodes", NormalsFromUV);

    long twoside = hPart->GetBool("TwoSideRendering", true) ? 1 : 0;

    // Let the user define a custom lower limit but a value less than
    // OCCT's epsilon is not allowed
    double lowerLimit = hPart->GetFloat("MinimumDeviation", tessRange.LowerBound);
    lowerLimit = std::max(lowerLimit, Precision::Confusion());
    tessRange.LowerBound = lowerLimit;

    static const char *osgroup = "Object Style";

    App::Material lmat;
    lmat.ambientColor.set(0.2f,0.2f,0.2f);
    lmat.diffuseColor.set(lr,lg,lb);
    lmat.specularColor.set(0.0f,0.0f,0.0f);
    lmat.emissiveColor.set(0.0f,0.0f,0.0f);
    lmat.shininess = 1.0f;
    lmat.transparency = 0.0f;

    App::Material vmat;
    vmat.ambientColor.set(0.2f,0.2f,0.2f);
    vmat.diffuseColor.set(vr,vg,vb);
    vmat.specularColor.set(0.0f,0.0f,0.0f);
    vmat.emissiveColor.set(0.0f,0.0f,0.0f);
    vmat.shininess = 1.0f;
    vmat.transparency = 0.0f;	
	
    ADD_PROPERTY_TYPE(LineMaterial,(lmat), osgroup, App::Prop_None, "Object line material.");
    ADD_PROPERTY_TYPE(PointMaterial,(vmat), osgroup, App::Prop_None, "Object point material.");
    ADD_PROPERTY_TYPE(LineColor, (lmat.diffuseColor), osgroup, App::Prop_None, "Set object line color.");
    ADD_PROPERTY_TYPE(PointColor, (vmat.diffuseColor), osgroup, App::Prop_None, "Set object point color");
    ADD_PROPERTY_TYPE(PointColorArray, (PointColor.getValue()), osgroup, App::Prop_None, "Object point color array.");
    ADD_PROPERTY_TYPE(DiffuseColor,(ShapeColor.getValue()), osgroup, App::Prop_None, "Object diffuse color.");
    ADD_PROPERTY_TYPE(LineColorArray,(LineColor.getValue()), osgroup, App::Prop_None, "Object line color array.");
    ADD_PROPERTY_TYPE(LineWidth,(lwidth), osgroup, App::Prop_None, "Set object line width.");
    LineWidth.setConstraints(&sizeRange);
    PointSize.setConstraints(&sizeRange);
    ADD_PROPERTY_TYPE(PointSize,(psize), osgroup, App::Prop_None, "Set object point size.");
    ADD_PROPERTY_TYPE(Deviation,(PartParams::getMeshDeviation()), osgroup, App::Prop_None,
            "Sets the accuracy of the polygonal representation of the model\n"
            "in the 3D view (tessellation). Lower values indicate better quality.\n"
            "The value is in percent of object's size.");
    Deviation.setConstraints(&tessRange);
    ADD_PROPERTY_TYPE(AngularDeflection,(PartParams::getMeshAngularDeflection()), osgroup, App::Prop_None,
            "Specify how finely to generate the mesh for rendering on screen or when exporting.\n"
            "The default value is 28.5 degrees, or 0.5 radians. The smaller the value\n"
            "the smoother the appearance in the 3D view, and the finer the mesh that will be exported.");
    AngularDeflection.setConstraints(&angDeflectionRange);
    ADD_PROPERTY_TYPE(Lighting,(twoside), osgroup, App::Prop_None, "Set object lighting.");
    Lighting.setEnums(LightingEnums);
    ADD_PROPERTY_TYPE(DrawStyle,((long int)0), osgroup, App::Prop_None, "Defines the style of the edges in the 3D view.");
    DrawStyle.setEnums(DrawStyleEnums);

    ADD_PROPERTY_TYPE(MappedColors,(),"",
            (App::PropertyType)(App::Prop_Hidden|App::Prop_ReadOnly),"");

    ADD_PROPERTY(MapFaceColor,(PartParams::getMapFaceColor()));
    ADD_PROPERTY(MapLineColor,(PartParams::getMapLineColor()));
    ADD_PROPERTY(MapPointColor,(PartParams::getMapPointColor()));
    ADD_PROPERTY(MapTransparency,(PartParams::getMapTransparency()));
    ADD_PROPERTY(ForceMapColors,(false));

    coords = new SoFCCoordinate3();
    static_cast<SoFCCoordinate3*>(coords)->vp = this;
    coords->ref();
    pcoords = new SoCoordinate3();
    pcoords->ref();
    faceset = new SoBrepFaceSet();
    faceset->ref();
    norm = new SoNormal;
    norm->ref();
    normb = new SoNormalBinding;
    normb->value = SoNormalBinding::PER_VERTEX_INDEXED;
    normb->ref();
    lineset = new SoBrepEdgeSet();
    lineset->ref();
    nodeset = new SoBrepPointSet();
    nodeset->ref();

    pcFaceBind = new SoMaterialBinding();
    pcFaceBind->ref();

    pcLineBind = new SoMaterialBinding();
    pcLineBind->ref();
    pcLineMaterial = new SoMaterial;
    pcLineMaterial->ref();
    LineMaterial.touch();

    pcPointBind = new SoMaterialBinding();
    pcPointBind->ref();
    pcPointMaterial = new SoMaterial;
    pcPointMaterial->ref();
    PointMaterial.touch();

    pcLineStyle = new SoDrawStyle();
    pcLineStyle->ref();
    pcLineStyle->style = SoDrawStyle::LINES;
    pcLineStyle->lineWidth = LineWidth.getValue();

    pcPointStyle = new SoDrawStyle();
    pcPointStyle->ref();
    pcPointStyle->style = SoDrawStyle::POINTS;
    pcPointStyle->pointSize = PointSize.getValue();

    pShapeHints = new SoShapeHints;
    pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pShapeHints->ref();
    Lighting.touch();
    DrawStyle.touch();

    sPixmap = "Part_3D_object";
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
    static_cast<SoFCCoordinate3*>(coords)->vp = nullptr;
    coords->unref();
    pcoords->unref();
    faceset->unref();
    norm->unref();
    normb->unref();
    lineset->unref();
    nodeset->unref();
}

void ViewProviderPartExt::onChanged(const App::Property* prop)
{
    Gui::ColorUpdater colorUpdater;

    if (prop == &MappedColors ||
        prop == &MapFaceColor ||
        prop == &MapLineColor ||
        prop == &MapPointColor ||
        prop == &MapTransparency || 
        prop == &ForceMapColors) 
    {
        if(!prop->testStatus(App::Property::User3)) {
            if(prop == &MapFaceColor) {
                if(!MapFaceColor.getValue()) {
                    ShapeColor.touch();
                    return;
                }
            }else if(prop == &MapLineColor) {
                if(!MapLineColor.getValue()) {
                    LineColor.touch();
                    return;
                }
            }else if(prop == &MapPointColor) {
                if(!MapPointColor.getValue()) {
                    PointColor.touch();
                    return;
                }
            }
            updateColors();
        }
        return;
    }
    
    if (isRestoring()) {
        if (prop == &LineColor
                || prop == &LineMaterial
                || prop == &PointColor
                || prop == &PointMaterial
                || prop == &ShapeColor
                || prop == &ShapeMaterial)
        {
            // When restoring, rely on
            // DiffuseColor/LineColorArray/PointColorArray to setup the colors.
            // Because the order of restoring say DiffuseColor and ShapeColor
            // may be different depending on whether the PropertyColorList is
            // saved into a separate file or embedded inside xml.
            ViewProviderDocumentObject::onChanged(prop);
            return;
        }
    }

    // The lower limit of the deviation has been increased to avoid
    // to freeze the GUI
    // https://forum.freecadweb.org/viewtopic.php?f=3&t=24912&p=195613
    if (prop == &Deviation) {
        if (!prop->testStatus(App::Property::User3)) {
            if(isUpdateForced()||Visibility.getValue()) 
                updateVisual();
            else
                VisualTouched = true;
        }
    }
    if (prop == &AngularDeflection) {
        if (!prop->testStatus(App::Property::User3)) {
            if(isUpdateForced()||Visibility.getValue()) 
                updateVisual();
            else
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
        const App::Color& c = LineColor.getValue();
        pcLineMaterial->diffuseColor.setValue(c.r,c.g,c.b);
        if (c != LineMaterial.getValue().diffuseColor)
            LineMaterial.setDiffuseColor(c);
        LineColorArray.setValue(LineColor.getValue());
        if(!prop->testStatus(App::Property::User3))
            updateColors();
    }
    else if (prop == &PointColor) {
        const App::Color& c = PointColor.getValue();
        pcPointMaterial->diffuseColor.setValue(c.r,c.g,c.b);
        if (c != PointMaterial.getValue().diffuseColor)
            PointMaterial.setDiffuseColor(c);
        PointColorArray.setValue(PointColor.getValue());
        if(!prop->testStatus(App::Property::User3))
            updateColors();
    }
    else if (prop == &LineMaterial) {
        const App::Material& Mat = LineMaterial.getValue();
        if (LineColor.getValue() != Mat.diffuseColor)
            LineColor.setValue(Mat.diffuseColor);
        pcLineMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcLineMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
        pcLineMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcLineMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcLineMaterial->shininess.setValue(Mat.shininess);
        pcLineMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &PointMaterial) {
        const App::Material& Mat = PointMaterial.getValue();
        if (PointColor.getValue() != Mat.diffuseColor)
            PointColor.setValue(Mat.diffuseColor);
        pcPointMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcPointMaterial->diffuseColor.setValue(Mat.diffuseColor.r,Mat.diffuseColor.g,Mat.diffuseColor.b);
        pcPointMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcPointMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcPointMaterial->shininess.setValue(Mat.shininess);
        pcPointMaterial->transparency.setValue(Mat.transparency);
    }
    else if (prop == &PointColorArray) {
        setHighlightedPoints(PointColorArray.getValues());
        Gui::ColorUpdater::addObject(getObject());
    }
    else if (prop == &LineColorArray) {
        setHighlightedEdges(LineColorArray.getValues());
        Gui::ColorUpdater::addObject(getObject());
    }
    else if (prop == &DiffuseColor) {
        setHighlightedFaces(DiffuseColor.getValues());
        Gui::ColorUpdater::addObject(getObject());
    }
    else if(prop == &ShapeColor) {
        if(!ShapeColor.testStatus(App::Property::User3)) {
            Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                    App::Property::User3, &ShapeColor);
            ViewProviderGeometryObject::onChanged(prop);
            App::Color c = ShapeColor.getValue();
            c.a = Transparency.getValue()/100.0f;
            DiffuseColor.setValue(c);
            updateColors();
        }
        return;
    }
    else if (prop == &Transparency) {
        const App::Material& Mat = ShapeMaterial.getValue();
        long value = (long)(100*Mat.transparency);
        if (value != Transparency.getValue()) {
            float trans = Transparency.getValue()/100.0f;

            App::PropertyContainer* parent = ShapeMaterial.getContainer();
            ShapeMaterial.setContainer(nullptr);
            ShapeMaterial.setTransparency(trans);
            ShapeMaterial.setContainer(parent);

            if(!prop->testStatus(App::Property::User3)) {
                if(MapTransparency.getValue() || MappedColors.getSize()) {
                    updateColors();
                } else{
                    auto colors = DiffuseColor.getValues();
                    for (auto &c : colors)
                        c.a = trans;
                    DiffuseColor.setValues(colors);
                }
            }
        }
    }
    else if (prop == &Lighting) {
        if (Lighting.getValue() == 0)
            pShapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        else
            pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    }
    else if (prop == &DrawStyle) {
        if (DrawStyle.getValue() == 0)
            pcLineStyle->linePattern = 0xffff;
        else if (DrawStyle.getValue() == 1)
            pcLineStyle->linePattern = 0xf00f;
        else if (DrawStyle.getValue() == 2)
            pcLineStyle->linePattern = 0x0f0f;
        else
            pcLineStyle->linePattern = 0xff88;
    }
    else {
        // if the object was invisible and has been changed, recreate the visual
        if (prop == &Visibility && (isUpdateForced() || Visibility.getValue()) && VisualTouched) {
            updateVisual();
        }
    }

    ViewProviderGeometryObject::onChanged(prop);
}

bool ViewProviderPartExt::allowOverride(const App::DocumentObject &) const {
    // Many derived view providers still uses static_cast to get object
    // pointer, so check for exact type here.
    return getTypeId() == ViewProviderPartExt::getClassTypeId();
}

void ViewProviderPartExt::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);

    if (auto feat = Base::freecad_dynamic_cast<Part::Feature>(pcFeat)) {
        conn = feat->signalMapShapeColors.connect([this](App::Document *doc) {
            updateColors(doc, true);
        });
    }

    // Workaround for #0000433, i.e. use SoSeparator instead of SoGroup
    auto* pcNormalRoot = new SoSeparator();
    auto* pcFlatRoot = new SoSeparator();
    auto* pcWireframeRoot = new SoSeparator();
    auto* pcPointsRoot = new SoSeparator();
    auto* wireframe = new SoSeparator();

    // Must turn off all intermediate render caching, and let pcRoot to handle
    // cache without interference.
    pcNormalRoot->renderCaching =
        pcFlatRoot->renderCaching =
        pcWireframeRoot->renderCaching =
        pcPointsRoot->renderCaching =
        wireframe->renderCaching = SoSeparator::OFF;

    pcNormalRoot->boundingBoxCaching =
        pcFlatRoot->boundingBoxCaching =
        pcWireframeRoot->boundingBoxCaching =
        pcPointsRoot->boundingBoxCaching =
        wireframe->boundingBoxCaching = SoSeparator::OFF;

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
    pcNormalRoot->addChild(offset);
    pcNormalRoot->addChild(pcFlatRoot);
    pcNormalRoot->addChild(wireframe);
    pcNormalRoot->addChild(pcPointsRoot);

    // just faces with no edges or points
    pcFlatRoot->addChild(pShapeHints);
    pcFlatRoot->addChild(pcFaceBind);
    pcFlatRoot->addChild(pcShapeMaterial);
    SoDrawStyle* pcFaceStyle = new SoDrawStyle();
    pcFaceStyle->style = SoDrawStyle::FILLED;
    pcFlatRoot->addChild(pcFaceStyle);
    pcFlatRoot->addChild(norm);
    pcFlatRoot->addChild(normb);
    pcFlatRoot->addChild(faceset);

    // edges and points
    pcWireframeRoot->addChild(wireframe);
    pcWireframeRoot->addChild(pcPointsRoot);

    // normal viewing with edges and points
    auto pnormb = new SoNormalBinding;
    pnormb->value = SoNormalBinding::OVERALL;
    pcPointsRoot->addChild(pnormb);
    pcPointsRoot->addChild(pcoords);
    pcPointsRoot->addChild(pcPointBind);
    pcPointsRoot->addChild(pcPointMaterial);
    pcPointsRoot->addChild(pcPointStyle);
    pcPointsRoot->addChild(nodeset);

    // Move 'coords' before the switch
    pcRoot->insertChild(coords,pcRoot->findChild(pcModeSwitch));

    // putting all together with the switch
    addDisplayMaskMode(pcNormalRoot, "Flat Lines");
    addDisplayMaskMode(pcFlatRoot, "Shaded");
    addDisplayMaskMode(pcWireframeRoot, "Wireframe");
    addDisplayMaskMode(pcPointsRoot, "Point");
}

void ViewProviderPartExt::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Flat Lines",ModeName)==0 )
        setDisplayMaskMode("Flat Lines");
    else if ( strcmp("Shaded",ModeName)==0 )
        setDisplayMaskMode("Shaded");
    else if ( strcmp("Wireframe",ModeName)==0 )
        setDisplayMaskMode("Wireframe");
    else if ( strcmp("Points",ModeName)==0 )
        setDisplayMaskMode("Point");

    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderPartExt::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Flat Lines");
    StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Points");

    return StrList;
}

Part::TopoShape ViewProviderPartExt::getShape() const
{
    Part::TopoShape shape;
    if (!isAttachedToDocument() || !getObject())
        return shape;

    auto prop = Base::freecad_dynamic_cast<Part::PropertyPartShape>(
            getObject()->getPropertyByName(getShapePropertyName()));
    if (prop)
        return prop->getShape();
    return Part::Feature::getTopoShape(getObject());
}

void ViewProviderPartExt::setShapePropertyName(const char *propName)
{
    if(propName)
        shapePropName = propName;
    else
        shapePropName.clear();
    if (isUpdateForced()||Visibility.getValue())
        updateVisual();
    else
        VisualTouched = true;
}

const char * ViewProviderPartExt::getShapePropertyName() const
{
    return shapePropName.empty()?"Shape":shapePropName.c_str();
}

bool ViewProviderPartExt::getElementPicked(const SoPickedPoint *pp, std::string &subname) const
{
    const SoDetail *detail = pp->getDetail();
    if (!detail)
        return inherited::getElementPicked(pp,subname);

    std::ostringstream ss;
    auto node = pp->getPath()->getTail();
    if (node == faceset && detail->isOfType(SoFaceDetail::getClassTypeId())) {
        const SoFaceDetail* face_detail = static_cast<const SoFaceDetail*>(detail);
        int face = face_detail->getPartIndex() + 1;
        ss << "Face" << face;
    } else if (node == lineset && detail->isOfType(SoLineDetail::getClassTypeId())) {
        const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
        int edge = line_detail->getLineIndex() + 1;
        ss << "Edge" << edge;
    } else if (node == nodeset && detail->isOfType(SoPointDetail::getClassTypeId())) {
        const SoPointDetail* point_detail = static_cast<const SoPointDetail*>(detail);
        int vertex = point_detail->getCoordinateIndex() - nodeset->startIndex.getValue() + 1;
        ss << "Vertex" << vertex;
    } else
        return inherited::getElementPicked(pp,subname);

    subname = ss.str();
    return true;
}

std::string ViewProviderPartExt::getElement(const SoDetail *detail) const
{
    // This function is providered here for backward compatibility. It provides
    // mostly the same function as getElementPick(), but cannot work with exotic
    // viewproviders that also group other nodes here.
    if (!detail)
        return inherited::getElement(detail);

    std::ostringstream ss;
    if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
        const SoFaceDetail* face_detail = static_cast<const SoFaceDetail*>(detail);
        int face = face_detail->getPartIndex() + 1;
        ss << "Face" << face;
    } else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
        const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
        int edge = line_detail->getLineIndex() + 1;
        ss << "Edge" << edge;
    } else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
        const SoPointDetail* point_detail = static_cast<const SoPointDetail*>(detail);
        int vertex = point_detail->getCoordinateIndex() - nodeset->startIndex.getValue() + 1;
        ss << "Vertex" << vertex;
    } else
        return inherited::getElement(detail);

    return ss.str();
}

bool ViewProviderPartExt::getDetailPath(const char *subname,
                                    SoFullPath *pPath,
                                    bool append,
                                    SoDetail *&det) const
{
    auto element = Data::ComplexGeoData::findElementName(subname);
    if (element && element == subname) {
        if (Data::ComplexGeoData::hasMissingElement(subname))
            return false;
    }
    return inherited::getDetailPath(subname, pPath, append, det);
}

SoDetail* ViewProviderPartExt::getDetail(const char* subelement) const
{
    const auto &shape = getShape();
    Data::IndexedName element = shape.getElementName(subelement).index;
    auto res = shape.shapeTypeAndIndex(element);
    if(!res.second)
        return nullptr;

    Part::TopoShape subshape = shape.getSubTopoShape(res.first, res.second, true);
    if(subshape.isNull())
        return nullptr;

    SoDetail *detail = nullptr;
    switch(res.first) {
    case TopAbs_FACE:
        detail = new SoFaceDetail();
        static_cast<SoFaceDetail*>(detail)->setPartIndex(res.second - 1);
        break;
    case TopAbs_EDGE:
        detail = new SoLineDetail();
        static_cast<SoLineDetail*>(detail)->setLineIndex(res.second - 1);
        break;
    case TopAbs_VERTEX:
        detail = new SoPointDetail();
        static_cast<SoPointDetail*>(detail)->setCoordinateIndex(res.second + nodeset->startIndex.getValue() - 1);
        break;
    default:
        break;
    }

    return detail;
}

std::vector<Base::Vector3d> ViewProviderPartExt::getModelPoints(const SoPickedPoint* pp) const
{
    try {
        std::vector<Base::Vector3d> pts;
        std::string element;
        this->getElementPicked(pp, element);
        const auto &shape = getShape();

        TopoDS_Shape subShape = shape.getSubShape(element.c_str());

        // get the point of the vertex directly
        if (subShape.ShapeType() == TopAbs_VERTEX) {
            const TopoDS_Vertex& v = TopoDS::Vertex(subShape);
            gp_Pnt p = BRep_Tool::Pnt(v);
            pts.emplace_back(p.X(),p.Y(),p.Z());
        }
        // get the nearest point on the edge
        else if (subShape.ShapeType() == TopAbs_EDGE) {
            const SbVec3f& vec = pp->getPoint();
            BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(vec[0],vec[1],vec[2]));
            BRepExtrema_DistShapeShape distSS(subShape, mkVert.Vertex(), 0.1);
            if (distSS.NbSolution() > 0) {
                gp_Pnt p = distSS.PointOnShape1(1);
                pts.emplace_back(p.X(),p.Y(),p.Z());
            }
        }
        // get the nearest point on the face
        else if (subShape.ShapeType() == TopAbs_FACE) {
            const SbVec3f& vec = pp->getPoint();
            BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(vec[0],vec[1],vec[2]));
            BRepExtrema_DistShapeShape distSS(subShape, mkVert.Vertex(), 0.1);
            if (distSS.NbSolution() > 0) {
                gp_Pnt p = distSS.PointOnShape1(1);
                pts.emplace_back(p.X(),p.Y(),p.Z());
            }
        }

        return pts;
    }
    catch (...) {
    }

    // if something went wrong returns an empty array
    return std::vector<Base::Vector3d>();
}

std::vector<Base::Vector3d> ViewProviderPartExt::getSelectionShape(const char* /*Element*/) const
{
    return std::vector<Base::Vector3d>();
}

void ViewProviderPartExt::setHighlightedFaces(const std::vector<App::Color>& colors)
{
    Gui::SoUpdateVBOAction action;
    action.apply(this->faceset);

    int size = static_cast<int>(colors.size());
    if (size > 1) {
        int numfaces = this->faceset->partIndex.getNum();
        if(size > numfaces)
            size = numfaces;
        pcFaceBind->value = SoMaterialBinding::PER_PART;
        pcShapeMaterial->diffuseColor.setNum(numfaces);
        pcShapeMaterial->transparency.setNum(numfaces);
        SbColor* ca = pcShapeMaterial->diffuseColor.startEditing();
        float *t = pcShapeMaterial->transparency.startEditing();
        int i=0;
        for (; i < size; i++) {
            ca[i].setValue(colors[i].r, colors[i].g, colors[i].b);
            t[i] = colors[i].a;
        }
        const auto &color = ShapeColor.getValue();
        float trans = ShapeMaterial.getValue().transparency;
        for (; i < numfaces; i++) { 
            ca[i].setValue(color.r, color.g, color.b);
            t[i] = trans;
        }
        pcShapeMaterial->diffuseColor.finishEditing();
        pcShapeMaterial->transparency.finishEditing();
        return;
    }

    const auto &color = colors.size()==1?colors[0]:ShapeColor.getValue();
    pcFaceBind->value = SoMaterialBinding::OVERALL;
    pcShapeMaterial->diffuseColor.setValue(color.r, color.g, color.b);
    //pcShapeMaterial->transparency = colors[0].a; do not get transparency from DiffuseColor in this case
    pcShapeMaterial->transparency.setValue(ShapeMaterial.getValue().transparency);
}

void ViewProviderPartExt::setHighlightedFaces(const std::vector<App::Material>& colors)
{
    int size = static_cast<int>(colors.size());
    if (size > 1) {
        int numfaces = this->faceset->partIndex.getNum();
        if(size > numfaces)
            size = numfaces;

        pcFaceBind->value = SoMaterialBinding::PER_PART;

        pcShapeMaterial->diffuseColor.setNum(numfaces);
        pcShapeMaterial->ambientColor.setNum(numfaces);
        pcShapeMaterial->specularColor.setNum(numfaces);
        pcShapeMaterial->emissiveColor.setNum(numfaces);

        SbColor* dc = pcShapeMaterial->diffuseColor.startEditing();
        SbColor* ac = pcShapeMaterial->ambientColor.startEditing();
        SbColor* sc = pcShapeMaterial->specularColor.startEditing();
        SbColor* ec = pcShapeMaterial->emissiveColor.startEditing();

        int i=0;
        for (; i < size; i++) {
            dc[i].setValue(colors[i].diffuseColor.r, colors[i].diffuseColor.g, colors[i].diffuseColor.b);
            ac[i].setValue(colors[i].ambientColor.r, colors[i].ambientColor.g, colors[i].ambientColor.b);
            sc[i].setValue(colors[i].specularColor.r, colors[i].specularColor.g, colors[i].specularColor.b);
            ec[i].setValue(colors[i].emissiveColor.r, colors[i].emissiveColor.g, colors[i].emissiveColor.b);
        }

        const auto &material = ShapeMaterial.getValue();
        for (; i < numfaces; ++i) {
            dc[i].setValue(material.diffuseColor.r, material.diffuseColor.g, material.diffuseColor.b);
            ac[i].setValue(material.ambientColor.r, material.ambientColor.g, material.ambientColor.b);
            sc[i].setValue(material.specularColor.r, material.specularColor.g, material.specularColor.b);
            ec[i].setValue(material.emissiveColor.r, material.emissiveColor.g, material.emissiveColor.b);
        }

        pcShapeMaterial->diffuseColor.finishEditing();
        pcShapeMaterial->ambientColor.finishEditing();
        pcShapeMaterial->specularColor.finishEditing();
        pcShapeMaterial->emissiveColor.finishEditing();
        return;
    }

    const auto &material = colors.size()==1?colors[0]:ShapeMaterial.getValue();
    pcFaceBind->value = SoMaterialBinding::OVERALL;
    pcShapeMaterial->diffuseColor.setValue(material.diffuseColor.r, material.diffuseColor.g, material.diffuseColor.b);
    pcShapeMaterial->ambientColor.setValue(material.ambientColor.r, material.ambientColor.g, material.ambientColor.b);
    pcShapeMaterial->specularColor.setValue(material.specularColor.r, material.specularColor.g, material.specularColor.b);
    pcShapeMaterial->emissiveColor.setValue(material.emissiveColor.r, material.emissiveColor.g, material.emissiveColor.b);
}

static inline App::PropertyLinkSub *getColoredElements(const App::DocumentObject *obj) {
    if(!obj || !obj->getNameInDocument())
        return 0;
    return Base::freecad_dynamic_cast<App::PropertyLinkSub>(
            obj->getPropertyByName("ColoredElements"));
}

std::map<std::string,App::Color> ViewProviderPartExt::getElementColors(const char *element) const {
    std::map<std::string,App::Color> ret;

    if(!element || !element[0]) {
        auto color = ShapeColor.getValue();
        color.a = Transparency.getValue()/100.0f;
        ret["Face"] = color;
        ret["Edge"] = LineColor.getValue();
        ret["Vertex"] = PointColor.getValue();

        auto prop = getColoredElements(pcObject);
        if(prop && prop->getValue()==pcObject) {
            const auto &subs = prop->getSubValues();
            const auto &colors = MappedColors.getValues();
            if(subs.size()==colors.size()) {
                for(size_t i=0;i<subs.size();++i)
                    ret.emplace(subs[i],colors[i]);
            }
        }
        return ret;
    }

    std::string tmp;
    if(Part::TopoShape::isMappedElement(element)) {
        Data::IndexedName indexedName = getShape().getElementName(element).index;
        if (indexedName)
            element = indexedName.toString(tmp);
        else {
            for(auto &mapped : Part::Feature::getRelatedElements(getObject(),element)) {
                tmp.clear();
                for(auto &v : getElementColors(mapped.index.toString(tmp)))
                    ret.insert(v);
            }
            return ret;
        }
    }

    if(boost::starts_with(element,"Face")) {
        auto size = DiffuseColor.getSize();
        if(element[4]=='*') {
            auto color = ShapeColor.getValue();
            color.a = Transparency.getValue()/100.0f;
            bool singleColor = true;
            for(int i=0;i<size;++i) {
                if(DiffuseColor[i]!=color)
                    ret[std::string(element,4)+std::to_string(i+1)] = DiffuseColor[i];
                singleColor = singleColor && DiffuseColor[0]==DiffuseColor[i];
            }
            if(size && singleColor) {
                color = DiffuseColor[0];
                color.a = Transparency.getValue()/100.0f;
                ret.clear();
            }
            ret["Face"] = color;
        }else{
            int idx = atoi(element+4);
            if(idx>0 && idx<=size)
                ret[element] = DiffuseColor[idx-1];
            else
                ret[element] = ShapeColor.getValue();
            if(size==1)
                ret[element].a = Transparency.getValue()/100.0f;
        }
    } else if (boost::starts_with(element,"Edge")) {
        auto size = LineColorArray.getSize();
        if(element[4]=='*') {
            auto color = LineColor.getValue();
            bool singleColor = true;
            for(int i=0;i<size;++i) {
                if(LineColorArray[i]!=color)
                    ret[std::string(element,4)+std::to_string(i+1)] = LineColorArray[i];
                singleColor = singleColor && LineColorArray[0]==LineColorArray[i];
            }
            if(singleColor && size) {
                color = LineColorArray[0];
                ret.clear();
            }
            ret["Edge"] = color;
        }else{
            int idx = atoi(element+4);
            if(idx>0 && idx<=size)
                ret[element] = LineColorArray[idx-1];
            else
                ret[element] = LineColor.getValue();
        }
    } else if (boost::starts_with(element,"Vertex")) {
        auto size = PointColorArray.getSize();
        if(element[5]=='*') {
            auto color = PointColor.getValue();
            bool singleColor = true;
            for(int i=0;i<size;++i) {
                if(PointColorArray[i]!=color)
                    ret[std::string(element,5)+std::to_string(i+1)] = PointColorArray[i];
                singleColor = singleColor && PointColorArray[0]==PointColorArray[i];
            }
            if(singleColor && size) {
                color = PointColorArray[0];
                ret.clear();
            }
            ret["Vertex"] = color;
        }else{
            int idx = atoi(element+5);
            if(idx>0 && idx<=size)
                ret[element] = PointColorArray[idx-1];
            else
                ret[element] = PointColor.getValue();
        }
    }
    return ret;
}

void ViewProviderPartExt::setElementColors(const std::map<std::string,App::Color> &info) 
{
    auto propColoredElements = getColoredElements(pcObject);
    if(!propColoredElements)
        return;
    std::vector<App::Color> colors;
    std::vector<std::string> subs;
    colors.reserve(info.size());
    subs.reserve(info.size());
    bool touched = false;
    for(auto &v : info) {
        if(v.first == "Face") {
            if(ShapeColor.getValue()!=v.second) {
                touched = true;
                ShapeColor.setValue(v.second);
            }
            if(v.second.a*100 != Transparency.getValue()) {
                Transparency.setValue(v.second.a*100);
                touched = true;
            }
        } else if(v.first == "Edge") {
            if(LineColor.getValue()!=v.second) {
                LineColor.setValue(v.second);
                touched = true;
            }
        } else if(v.first == "Vertex"){
            if(PointColor.getValue()!=v.second) {
                PointColor.setValue(v.second);
                touched = true;
            }
        } else {
            subs.push_back(v.first);
            colors.push_back(v.second);
        }
    }
    if(colors!=MappedColors.getValues()) {
        touched = true;
        Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                App::Property::User3, &MappedColors);
        MappedColors.setValues(colors);
    }
    if(subs.empty())
        propColoredElements->setValue(0);
    else if(subs!=propColoredElements->getSubValues())
        propColoredElements->setValue(pcObject,subs);
    else if(touched)
        updateColors();
}

void ViewProviderPartExt::unsetHighlightedFaces()
{
    setHighlightedFaces(DiffuseColor.getValues());
}

void ViewProviderPartExt::setHighlightedEdges(const std::vector<App::Color>& colors)
{
    int size = static_cast<int>(colors.size());
    if (size > 1) {
        // Although indexed lineset is used the material binding must be PER_FACE!
        pcLineBind->value = SoMaterialBinding::PER_FACE;
        const int32_t* cindices = this->lineset->coordIndex.getValues(0);
        int numindices = this->lineset->coordIndex.getNum();
        int linecount = 0;
        for (int i = 0; i < numindices; ++i) {
            if (cindices[i] < 0)
                ++linecount;
        }
        pcLineMaterial->diffuseColor.setNum(linecount);
        SbColor* ca = pcLineMaterial->diffuseColor.startEditing();

        if(size > linecount)
            size = linecount;

        int i=0;
        for (; i < size; ++i) 
            ca[i].setValue(colors[i].r, colors[i].g, colors[i].b);

        const auto &color = LineColor.getValue();
        for (; i < linecount; ++i)
            ca[i].setValue(color.r, color.g, color.b);

        pcLineMaterial->diffuseColor.finishEditing();
        return;
    }

    const auto &color = colors.size()==1?colors[0]:LineColor.getValue();
    pcLineBind->value = SoMaterialBinding::OVERALL;
    pcLineMaterial->diffuseColor.setValue(color.r, color.g, color.b);
}

void ViewProviderPartExt::unsetHighlightedEdges()
{
    setHighlightedEdges(LineColorArray.getValues());
}

void ViewProviderPartExt::setHighlightedPoints(const std::vector<App::Color>& colors)
{
    int size = static_cast<int>(colors.size());
    if (size > 1) {
        int numpoints = pcoords->point.getNum();
        if(size > numpoints)
            size = numpoints;
        pcPointBind->value = SoMaterialBinding::PER_VERTEX;
        pcPointMaterial->diffuseColor.setNum(numpoints);
        SbColor* ca = pcPointMaterial->diffuseColor.startEditing();
        int i=0;
        for (; i < size; ++i)
            ca[i].setValue(colors[i].r, colors[i].g, colors[i].b);
        const auto &color = PointColor.getValue();
        for (; i < numpoints; ++i)
            ca[i].setValue(color.r, color.g, color.b);
        pcPointMaterial->diffuseColor.finishEditing();
    }

    const auto &color = size==1?colors[0]:PointColor.getValue();
    pcPointBind->value = SoMaterialBinding::OVERALL;
    pcPointMaterial->diffuseColor.setValue(color.r, color.g, color.b);
}

void ViewProviderPartExt::unsetHighlightedPoints()
{
    setHighlightedPoints(PointColorArray.getValues());
}

void ViewProviderPartExt::reload()
{
    bool update = false;
    double pointsize = PointSize.getValue();
    double linewidth = LineWidth.getValue();
    if (PartParams::getRespectSystemDPI()) {
        auto dpi = qApp->devicePixelRatio();
        pointsize = std::max(1.0, pointsize*dpi);
        linewidth = std::max(1.0, linewidth*dpi);
    }
    if (pcPointStyle->pointSize.getValue() != pointsize
            || pcLineStyle->lineWidth.getValue() != linewidth)
    {
        pcPointStyle->pointSize = pointsize;
        pcLineStyle->lineWidth = linewidth;
        update = true;
    }
    if (NormalsFromUV != PartParams::getNormalsFromUVNodes()) {
        update = true;
        NormalsFromUV = !NormalsFromUV;
    }

    tessRange.LowerBound = PartParams::getMinimumDeviation();
    angDeflectionRange.LowerBound = PartParams::getMinimumAngularDeflection();

    if (Deviation.getValue() != PartParams::getMeshDeviation()
            || Deviation.getValue() < PartParams::getMinimumDeviation()
            || AngularDeflection.getValue() != PartParams::getMeshAngularDeflection()
            || AngularDeflection.getValue() < PartParams::getMinimumAngularDeflection())
        update = true;

    if (!update)
        return;

    if (!PartParams::getOverrideTessellation()) {
        Base::ObjectStatusLocker<App::Property::Status,App::Property> guard(
                App::Property::User3, &Deviation);

        Deviation.setValue(PartParams::getMeshDeviation());
        Base::ObjectStatusLocker<App::Property::Status,App::Property> guard2(
                App::Property::User3, &AngularDeflection);
        AngularDeflection.setValue(PartParams::getMeshAngularDeflection());
    }
    updateVisual();
}

static bool getLinkColor(const Data::MappedName &mapped, App::DocumentObject *&obj, 
        ViewProviderPartExt *&svp, App::Color &color) 
{
    if(!obj)
        return false;
    bool colorFound = false;
    for(int depth=0;;++depth) {
        auto vp = Base::freecad_dynamic_cast<Gui::ViewProviderLink>(
                Gui::Application::Instance->getViewProvider(obj));
        if(vp && vp->getDefaultMode()==1) {
            svp = Base::freecad_dynamic_cast<ViewProviderPartExt>(
                    vp->ChildViewProvider.getObject().get());
            if(svp)
                return false;
        }
        auto link = obj->getExtensionByType<App::LinkBaseExtension>(true);
        if(vp && vp->OverrideMaterial.getValue()) {
            colorFound = true;
            color = vp->ShapeMaterial.getValue().diffuseColor;
            color.a = vp->ShapeMaterial.getValue().transparency;
            if(!link || !link->getElementCountValue())
                return true;
        }
        // check for link array
        if(link && !link->getShowElementValue() && link->getElementCountValue()) {
            int pos = mapped.rfind(Part::TopoShape::indexPostfix());
            if(pos >= 0) {
                int offset = pos + static_cast<int>(Part::TopoShape::indexPostfix().size());
                QByteArray bytes = mapped.toRawBytes(offset);
                bio::stream<bio::array_source> iss(bytes.constData(), bytes.size());
                int index = 0;
                char sep = 0;
                iss >> index >> sep;
                if(sep==';' && 
                    vp->OverrideMaterialList.getSize()>index && 
                    vp->OverrideMaterialList[index] &&
                    vp->MaterialList.getSize()>index)
                {
                    color = vp->MaterialList[index].diffuseColor;
                    color.a = vp->MaterialList[index].transparency;
                    return true;
                }
                if(colorFound)
                    return colorFound;
                obj = obj->getSubObject((std::to_string(index)+".").c_str());
                return false;
            }
        }
        auto linked = obj->getLinkedObject(false,0,false,depth);
        if(!linked || linked==obj)
            break;
        obj = linked;
    }
    return colorFound;
}

struct ElementCache
{
    Part::TopoShape shape;
    ViewProviderPartExt *vp;
    bool inited = false;
};

static App::Color getElementColor(App::Color color, 
                                  const Part::TopoShape &shape,
                                  App::Document *doc,
                                  int type,
                                  const Data::MappedName &name,
                                  std::map<App::DocumentObject*, ElementCache> &caches)
{
    if (!name)
        return color;

    Data::MappedName mapped(name);
    bool colorFound = false;
    std::vector<Data::MappedName> history;
    std::vector<Data::MappedName> prevHistory;
    Data::MappedName original;
    long tag = shape.getElementHistory(mapped,&original,&prevHistory);
    while(1) {
        if(!tag)
            return color;
        auto obj = doc->getObjectByID(std::abs(tag));
        if(!obj || !obj->getNameInDocument())
            return color;
        auto & cache = caches[obj];
        if (!cache.inited) {
            cache.inited = true;
            cache.shape = Part::Feature::getTopoShape(obj);
            cache.vp = Base::freecad_dynamic_cast<ViewProviderPartExt>(
                    Gui::Application::Instance->getViewProvider(obj));
        }
        const Part::TopoShape & shape = cache.shape;
        ViewProviderPartExt *vp = cache.vp;
        if(shape.isNull() || getLinkColor(original,obj,vp,color) || !obj)
            return color;
        if(!vp) {
            // Not a part view provider. No problem, just trace deeper into the
            // history until we find one.
            doc = obj->getDocument();
            mapped = original;
            prevHistory.clear();
            tag = shape.getElementHistory(mapped,&original,&prevHistory);
            continue;
        }

        if(colorFound)
            return color;

        float trans = vp->Transparency.getValue()/100.0;
        auto prop = &vp->DiffuseColor;
        if(type == TopAbs_VERTEX) 
            prop = &vp->PointColorArray;
        else if(type == TopAbs_EDGE)
            prop = &vp->LineColorArray;
        if(prop->getSize()==0)
            return color;

        mapped = original;
        // Normally, TopoShape::getElementHistory() returns the mapped element
        // name of the previous step (in 'original'), and tag is the ID of the
        // previous feature. 'mapped' is the mapped element name of the next
        // modeling step in shape history.
        //
        // However, if there are intermediate modeling steps, things get a bit
        // tricky. getElementHistory() returns intermediate element names in
        // 'history', but the last entry of 'history' actually contains the real
        // mapped element name of the 'current' modeling step. That's why we are
        // calling getElementHistory() for previous modeling step now, before
        // retrieving the element for the current step using getElementName(),
        // because we need to check intermediate history names of the previous
        // model step. The 'original' returned by getElementHistory() here may
        // or may not contain a valid element name for the previous step. We can
        // only decide after another loop hits here.
        std::swap(history, prevHistory);
        prevHistory.clear();
        tag = shape.getElementHistory(mapped,&original,&prevHistory);
        Data::IndexedName element = shape.getIndexedName(history.empty()?mapped:history.back());
        auto idx = Part::TopoShape::shapeTypeAndIndex(element);
        if(idx.second>0 && idx.second<=(int)shape.countSubShapes(idx.first)) {
            if(idx.first==type) {
                if(prop->getSize()==1) {
                    color = prop->getValues()[0];
                    color.a = trans;
                }
                else if(idx.second<=prop->getSize()) 
                    return prop->getValues()[idx.second-1];
            }else{
                // This means the element is generated from a different type of source element,
                // e.g. face generated by an edge.
                auto aidx = shape.findAncestor(shape.findShape(idx.first,idx.second),(TopAbs_ShapeEnum)type);
                if(aidx>0) {
                    if(prop->getSize()==1) {
                        color = prop->getValues()[0];
                        color.a = trans;
                    }
                    else if(aidx<=prop->getSize())
                        return prop->getValues()[aidx-1];
                }
            }
        }
        return color;
    }
}

std::vector<App::Color> ViewProviderPartExt::getShapeColors(const Part::TopoShape &shape, 
        App::Color &defColor, App::Document *sourceDoc, bool linkOnly)
{
    defColor.setPackedValue(Gui::ViewParams::instance()->getDefaultShapeColor());
    defColor.a = 0;

    if(!sourceDoc) {
        sourceDoc = App::GetApplication().getActiveDocument();
        if(!sourceDoc)
            return {};
    }
    size_t count = shape.countSubShapes(TopAbs_FACE);
    if(!count)
        return {};

    Data::MappedName mapped = shape.getMappedName(
            Data::IndexedName::fromConst("Face", 1), true);

    ViewProviderPartExt *vp=0;
    auto obj = sourceDoc->getObjectByID(shape.Tag);
    if(getLinkColor(mapped,obj,vp,defColor))
        return {defColor};
    else if(linkOnly || !obj)
        return {};

    if(!vp)
        vp = Base::freecad_dynamic_cast<ViewProviderPartExt>(
                Gui::Application::Instance->getViewProvider(obj));
    if(vp) {
        defColor = vp->ShapeColor.getValue();
        defColor.a = vp->Transparency.getValue()/100.0f;
        return vp->DiffuseColor.getValues();
    }

    std::vector<App::Color> colors(count,defColor);
    bool touched = false;
    std::map<App::DocumentObject*,ElementCache> caches;
    for(size_t i=0;i<=count;++i) {
        mapped = shape.getMappedName(Data::IndexedName::fromConst("Face", i+1));
        if (mapped) {
            auto color = getElementColor(defColor,shape,sourceDoc,TopAbs_FACE,mapped,caches);
            if(color!=defColor) {
                colors[i] = color;
                touched = true;
            }
        }
    }
    if(!touched)
        return {};
    return colors;
}

bool ViewProviderPartExt::hasBaseFeature() const {
    return !claimChildren().empty();
}

struct ColorInfo {
    TopAbs_ShapeEnum type;
    App::PropertyColorList *prop = 0;
    App::Color defaultColor;
    std::map<int,App::Color> colors;
    bool mapColor;

    void init(TopAbs_ShapeEnum t, ViewProviderPartExt *vp) {
        type = t;
        switch(type) {
        case TopAbs_VERTEX:
            defaultColor = vp->PointColor.getValue();
            prop = &vp->PointColorArray;
            mapColor = vp->MapPointColor.getValue();
            break;
        case TopAbs_EDGE:
            defaultColor = vp->LineColor.getValue();
            prop = &vp->LineColorArray;
            mapColor = vp->MapLineColor.getValue();
            break;
        case TopAbs_FACE:
            defaultColor = vp->ShapeColor.getValue();
            defaultColor.a = vp->Transparency.getValue()/100.0f;
            prop = &vp->DiffuseColor;
            mapColor = vp->MapFaceColor.getValue();
            break;
        default:
            assert(0);
        }
    }
};

void ViewProviderPartExt::checkColorUpdate()
{
    if (MapFaceColor.getValue()
            || MapLineColor.getValue()
            || MapPointColor.getValue()
            || MapTransparency.getValue())
        updateColors();
}

void ViewProviderPartExt::updateColors(App::Document *sourceDoc, bool forceColorMap) 
{
    if (UpdatingColor
            || !getObject()
            || !getObject()->getDocument()
            || getObject()->getDocument()->testStatus(App::Document::Restoring))
        return;

    auto geoFeature = Base::freecad_dynamic_cast<App::GeoFeature>(pcObject);

    Base::FlagToggler<> flag(UpdatingColor);
    auto prop = getColoredElements(pcObject);
    if(prop && prop->getSubValues().size()!=(size_t)MappedColors.getSize()) {
        if(prop->getSubValues().size()<(size_t)MappedColors.getSize())
            MappedColors.setSize(prop->getSubValues().size());
        else {
            auto subs = prop->getSubValues();
            subs.resize(MappedColors.getSize());
            prop->setValue(pcObject,subs);
        }
        return;
    }

    auto shape = getShape();
    if(shape.isNull())
        return;

    if(!sourceDoc)
        sourceDoc = pcObject->getDocument();

    std::vector<std::pair<std::string,std::string> > _subs;
    const auto &subs = prop?prop->getShadowSubs():_subs;

    std::array<ColorInfo,TopAbs_SHAPE> infos;
    infos[TopAbs_VERTEX].init(TopAbs_VERTEX,this);
    infos[TopAbs_EDGE].init(TopAbs_EDGE,this);
    infos[TopAbs_FACE].init(TopAbs_FACE,this);
    bool noColorMap = !ForceMapColors.getValue() && !forceColorMap && !hasBaseFeature();

    std::set<Data::MappedName> subMap;
    for(auto &v : subs) {
        if(v.first.size())
            subMap.insert(shape.getElementName(v.first.c_str()).name);
    }
    int i=-1;
    for(auto &v : subs) {
        ++i;
        Data::IndexedName element;
        if (v.first.size())
            element = shape.getElementName(v.first.c_str()).index;
        else
            element = Data::IndexedName(v.second.c_str());
        auto idx = shape.shapeTypeAndIndex(element);
        if(idx.second) {
            infos[idx.first].colors[idx.second-1] = MappedColors[i];
            continue;
        }else if(v.first.empty())
            continue;

        for(auto &names : Part::Feature::getRelatedElements(pcObject,v.first.c_str())) {
            if(!subMap.insert(names.name).second)
                continue;
            auto idx = Part::TopoShape::shapeTypeAndIndex(names.index);
            if(idx.second>0)
                infos[idx.first].colors[idx.second-1] = MappedColors[i];
        }
    }
    std::map<App::DocumentObject*,ElementCache> caches;
    for(auto &info : infos) {
        if(!info.prop) continue;
        if(noColorMap || !info.mapColor) {
            if(info.colors.empty())
                info.prop->touch();
            else {
                auto colors = info.prop->getValues();
                if(colors.size()!=shape.countSubShapes(info.type)) {
                    colors.clear();
                    colors.resize(shape.countSubShapes(info.type),info.defaultColor);
                }
                for(auto &v : info.colors) {
                    if(v.first>=(int)colors.size())
                        break;
                    colors[v.first] = v.second;
                }
                info.prop->setValue(colors);
            }
            continue;
        }
        int count = shape.countSubShapes(info.type);
        bool touched = false;
        std::vector<App::Color> colors(count,info.defaultColor);
        auto it = info.colors.begin();
        const char * typeName = shape.shapeName(info.type).c_str();
        float trans = Transparency.getValue()/100.0f;
        for(int i=0;i<count;++i) {
            if(it!=info.colors.end() && i==it->first) {
                if(colors[i]!=it->second) {
                    touched = true;
                    colors[i] = it->second;
                }
                ++it;
                continue;
            }
            Data::MappedName mapped = shape.getMappedName(
                    Data::IndexedName::fromConst(typeName, i+1));
            if(!mapped)
                continue;

            App::Document *doc = sourceDoc;
            if(geoFeature) {
                auto owner = geoFeature->getElementOwner(mapped);
                if(owner)
                    doc = owner->getDocument();
            }
            auto color = getElementColor(info.defaultColor, shape, doc,info.type,mapped,caches);
            if(!MapTransparency.getValue())
                color.a = trans;
            if(color != colors[i]) {
                touched = true;
                colors[i] = color;
            }
        }
        if(!touched) {
            colors.clear();
            colors.push_back(info.defaultColor);
        }
        info.prop->setValue(colors);
    }
}

void ViewProviderPartExt::updateData(const App::Property* prop)
{
    const char *shapeProp = shapePropName.empty()?"Shape":shapePropName.c_str();
    const char *propName = prop?prop->getName():"";
    if(strcmp(propName,"ColoredElements")==0
            || strcmp(propName,shapeProp)==0
            || strstr(propName,"Touched")!=0)
    {
        TopoDS_Shape cShape = getShape().getShape();
        if(cachedShape.getShape().IsPartner(cShape)) {
            updateColors();
            Gui::ViewProviderGeometryObject::updateData(prop);
            return;
        }

        // calculate the visual only if visible
        if (isUpdateForced() || Visibility.getValue())
            updateVisual();
        else 
            VisualTouched = true;

        updateColors();

        if (!VisualTouched) {
            if (this->faceset->partIndex.getNum() > 
                this->pcShapeMaterial->diffuseColor.getNum()) {
                this->pcFaceBind->value = SoMaterialBinding::OVERALL;
            }
        }
    }
    Gui::ViewProviderGeometryObject::updateData(prop);
}

void ViewProviderPartExt::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QIcon iconObject = mergeGreyableOverlayIcons(Gui::BitmapFactory().pixmap("Part_ColorFace.svg"));
    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
    QAction* act = menu->addAction(iconObject, QObject::tr("Set colors..."), receiver, member);
    act->setData(QVariant((int)ViewProvider::Color));
}

void ViewProviderPartExt::setEditViewer(Gui::View3DInventorViewer *viewer, int ModNum) {
    if (ModNum == ViewProvider::Color)
        Gui::Control().showDialog(new Gui::TaskElementColors(this,true));
    else
        Gui::ViewProviderGeometryObject::setEditViewer(viewer,ModNum);
}

bool ViewProviderPartExt::changeFaceColors()
{
    return Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Color);
}

bool ViewProviderPartExt::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Color) {
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        if (dlg) {
            Gui::Control().showDialog(dlg);
            return false;
        }

        // TaskFaceColors is replaced by TaskElementColors, and is inited in 
        // setEditViewer() in order to handle editing context
        //
        // Gui::Control().showDialog(new TaskFaceColors(this));
        return true;
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

void ViewProviderPartExt::updateVisual()
{
    if (!getObject()
            || !getObject()->getDocument()
            || isRestoring())
    {
        VisualTouched = true;
        return;
    }

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

    const Part::TopoShape & toposhape = getShape();
    TopoDS_Shape cShape = toposhape.getShape();
    cachedShape = cShape;
    if (cShape.IsNull()) {
        coords  ->point      .setNum(0);
        pcoords ->point      .setNum(0);
        norm    ->vector     .setNum(0);
        faceset ->coordIndex .setNum(0);
        faceset ->partIndex  .setNum(0);
        lineset ->coordIndex .setNum(0);
        nodeset ->startIndex .setValue(0);
        VisualTouched = false;
        return;
    }

    // copy edge sub shape to work around OCC trangulation bug (in
    // case the edge is part of a face of some other shape in a
    // different location). Seems OCC 7.4 has fixed problem.
#if OCC_VERSION_HEX < 0x070400
    if (!toposhape.hasSubShape(TopAbs_FACE) && toposhape.hasSubShape(TopAbs_EDGE))
        cShape = BRepBuilderAPI_Copy(cShape).Shape();
#endif

    // time measurement and book keeping
    Base::TimeInfo start_time;
    int numTriangles=0,numNodes=0,numPoints=0,numNorms=0,numFaces=0,numEdges=0,numLines=0;
    std::unordered_map<TopoDS_Shape, TopoDS_Face, Part::ShapeHasher, Part::ShapeHasher> faceEdges;

    try {
        // calculating the deflection value
        Bnd_Box bounds;
        BRepBndLib::Add(cShape, bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        Standard_Real deflection = ((xMax-xMin)+(yMax-yMin)+(zMax-zMin))/300.0 * Deviation.getValue();

        // Since OCCT 7.6 a value of equal 0 is not allowed any more, this can happen if a single vertex
        // should be displayed.
        if (deflection < gp::Resolution())
            deflection = Precision::Confusion();

        // create or use the mesh on the data structure
        Standard_Real AngDeflectionRads = AngularDeflection.getValue() / 180.0 * M_PI;
        BRepMesh_IncrementalMesh(cShape, deflection, Standard_False, AngDeflectionRads, Standard_True);

        // We must reset the location here because the transformation data
        // are set in the placement property
        TopLoc_Location aLoc;
        cShape.Location(aLoc);

        // count triangles and nodes in the mesh
        TopTools_IndexedMapOfShape faceMap;
        TopExp::MapShapes(cShape, TopAbs_FACE, faceMap);
        for (int i=1; i <= faceMap.Extent(); i++) {
            TopoDS_Face face = TopoDS::Face(faceMap(i));
            Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(face, aLoc);
            if (mesh.IsNull()) {
                mesh = Part::Tools::triangulationOfFace(TopoDS::Face(faceMap(i)));
            }
            // Note: we must also count empty faces
            if (!mesh.IsNull()) {
                numTriangles += mesh->NbTriangles();
                numNodes     += mesh->NbNodes();
                numNorms     += mesh->NbNodes();
            }

            TopExp_Explorer xp;
            for (xp.Init(face,TopAbs_EDGE);xp.More();xp.Next())
                faceEdges.emplace(xp.Current(), face);
            numFaces++;
        }

        // get an indexed map of edges
        TopTools_IndexedMapOfShape edgeMap;
        TopExp::MapShapes(cShape, TopAbs_EDGE, edgeMap);

         // key is the edge number, value the coord indexes. This is needed to keep the same order as the edges.
        std::map<int, std::vector<int32_t> > lineSetMap;
        std::set<int>          edgeIdxSet;
        std::vector<int32_t>   edgeVector;

        // count and index the edges
        for (int i=1; i <= edgeMap.Extent(); i++) {
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
            auto it = faceEdges.find(aEdge);
            if (it == faceEdges.end()) {
                Handle(Poly_Polygon3D) aPoly = Part::Tools::polygonOfEdge(aEdge, aLoc);
                if (!aPoly.IsNull()) {
                    int nbNodesInEdge = aPoly->NbNodes();
                    numNodes += nbNodesInEdge;
                }
            }
        }

        // create memory for the nodes and indexes
        coords  ->point      .setNum(numNodes);
        norm    ->vector     .setNum(numNorms);
        faceset ->coordIndex .setNum(numTriangles*4);
        faceset ->partIndex  .setNum(numFaces);
        // get the raw memory for fast fill up
        SbVec3f* verts = coords  ->point       .startEditing();
        SbVec3f* norms = norm    ->vector      .startEditing();
        int32_t* index = faceset ->coordIndex  .startEditing();
        int32_t* parts = faceset ->partIndex   .startEditing();

        // preset the normal vector with null vector
        for (int i=0;i < numNorms;i++)
            norms[i]= SbVec3f(0.0,0.0,0.0);

        int ii = 0,faceNodeOffset=0,faceTriaOffset=0;
        for (int i=1; i <= faceMap.Extent(); i++, ii++) {
            TopLoc_Location aLoc;
            const TopoDS_Face &actFace = TopoDS::Face(faceMap(i));
            // get the mesh of the shape
            Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(actFace,aLoc);
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
            int nbTriInFace   = mesh->NbTriangles();
            // check orientation
            TopAbs_Orientation orient = actFace.Orientation();


            // cycling through the poly mesh
#if OCC_VERSION_HEX < 0x070600
            const Poly_Array1OfTriangle& Triangles = mesh->Triangles();
            const TColgp_Array1OfPnt& Nodes = mesh->Nodes();
            TColgp_Array1OfDir Normals (Nodes.Lower(), Nodes.Upper());
#else
            int numNodes =  mesh->NbNodes();
            TColgp_Array1OfDir Normals (1, numNodes);
#endif
            if (NormalsFromUV)
                Part::Tools::getPointNormals(actFace, mesh, Normals);
            
            for (int g=1;g<=nbTriInFace;g++) {
                // Get the triangle
                Standard_Integer N1,N2,N3;
#if OCC_VERSION_HEX < 0x070600
                Triangles(g).Get(N1,N2,N3);
#else
                mesh->Triangle(g).Get(N1,N2,N3);
#endif

                // change orientation of the triangle if the face is reversed
                if ( orient != TopAbs_FORWARD ) {
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
                if (NormalsFromUV) {
                    NV1.SetXYZ(Normals(N1).XYZ());
                    NV2.SetXYZ(Normals(N2).XYZ());
                    NV3.SetXYZ(Normals(N3).XYZ());
                }
                else {
                    gp_Vec v1(V1.X(),V1.Y(),V1.Z()),
                           v2(V2.X(),V2.Y(),V2.Z()),
                           v3(V3.X(),V3.Y(),V3.Z());
                    gp_Vec normal = (v2-v1)^(v3-v1);
                    NV1 = normal;
                    NV2 = normal;
                    NV3 = normal;
                }

                // transform the vertices and normals to the place of the face
                if (!identity) {
                    V1.Transform(myTransf);
                    V2.Transform(myTransf);
                    V3.Transform(myTransf);
                    if (NormalsFromUV) {
                        NV1.Transform(myTransf);
                        NV2.Transform(myTransf);
                        NV3.Transform(myTransf);
                    }
                }

                // add the normals for all points of this triangle
                norms[faceNodeOffset+N1-1] += SbVec3f(NV1.X(),NV1.Y(),NV1.Z());
                norms[faceNodeOffset+N2-1] += SbVec3f(NV2.X(),NV2.Y(),NV2.Z());
                norms[faceNodeOffset+N3-1] += SbVec3f(NV3.X(),NV3.Y(),NV3.Z());

                // set the vertices
                verts[faceNodeOffset+N1-1].setValue((float)(V1.X()),(float)(V1.Y()),(float)(V1.Z()));
                verts[faceNodeOffset+N2-1].setValue((float)(V2.X()),(float)(V2.Y()),(float)(V2.Z()));
                verts[faceNodeOffset+N3-1].setValue((float)(V3.X()),(float)(V3.Y()),(float)(V3.Z()));

                // set the index vector with the 3 point indexes and the end delimiter
                index[faceTriaOffset*4+4*(g-1)]   = faceNodeOffset+N1-1;
                index[faceTriaOffset*4+4*(g-1)+1] = faceNodeOffset+N2-1;
                index[faceTriaOffset*4+4*(g-1)+2] = faceNodeOffset+N3-1;
                index[faceTriaOffset*4+4*(g-1)+3] = SO_END_FACE_INDEX;
            }

            parts[ii] = nbTriInFace; // new part

            // handling the edges lying on this face
            TopExp_Explorer Exp;
            for(Exp.Init(actFace,TopAbs_EDGE);Exp.More();Exp.Next()) {
                const TopoDS_Edge &curEdge = TopoDS::Edge(Exp.Current());
                // get the overall index of this edge
                int edgeIndex = edgeMap.FindIndex(curEdge);
                edgeVector.push_back((int32_t)edgeIndex-1);
                // already processed this index ?
                if (edgeIdxSet.find(edgeIndex)!=edgeIdxSet.end()) {
                    
                    // this holds the indices of the edge's triangulation to the current polygon
                    Handle(Poly_PolygonOnTriangulation) aPoly = BRep_Tool::PolygonOnTriangulation(curEdge, mesh, aLoc);
                    if (aPoly.IsNull())
                        continue; // polygon does not exist
                    
                    // getting the indexes of the edge polygon
                    const TColStd_Array1OfInteger& indices = aPoly->Nodes();
                    for (Standard_Integer i=indices.Lower();i <= indices.Upper();i++) {
                        int nodeIndex = indices(i);
                        int index = faceNodeOffset+nodeIndex-1;
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
                        if (!identity)
                            p.Transform(myTransf);
                        verts[index].setValue((float)(p.X()),(float)(p.Y()),(float)(p.Z()));
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
        for (int i=1; i <= edgeMap.Extent(); i++) {
            const TopoDS_Edge& aEdge = TopoDS::Edge(edgeMap(i));
            Standard_Boolean identity = true;
            gp_Trsf myTransf;
            TopLoc_Location aLoc;

            // handling of the free edge that are not associated to a face
            if (!faceEdges.count(aEdge)) {
                Handle(Poly_Polygon3D) aPoly = Part::Tools::polygonOfEdge(aEdge, aLoc);
                if (!aPoly.IsNull()) {
                    if (!aLoc.IsIdentity()) {
                        identity = false;
                        myTransf = aLoc.Transformation();
                    }

                    const TColgp_Array1OfPnt& aNodes = aPoly->Nodes();
                    int nbNodesInEdge = aPoly->NbNodes();

                    gp_Pnt pnt;
                    for (Standard_Integer j=1;j <= nbNodesInEdge;j++) {
                        pnt = aNodes(j);
                        if (!identity)
                            pnt.Transform(myTransf);
                        int index = faceNodeOffset+j-1;
                        verts[index].setValue((float)(pnt.X()),(float)(pnt.Y()),(float)(pnt.Z()));
                        lineSetMap[i].push_back(index);
                    }

                    faceNodeOffset += nbNodesInEdge;
                }
            }
        }

        // handling of the vertices
        TopTools_IndexedMapOfShape vertexMap;
        TopExp::MapShapes(cShape, TopAbs_VERTEX, vertexMap);

        numPoints = vertexMap.Extent();
        pcoords->point.setNum(numPoints);
        verts = pcoords->point.startEditing();

        for (int i=0; i<numPoints; i++) {
            const TopoDS_Vertex& aVertex = TopoDS::Vertex(vertexMap(i+1));
            gp_Pnt pnt = BRep_Tool::Pnt(aVertex);
            verts[i].setValue((float)(pnt.X()),(float)(pnt.Y()),(float)(pnt.Z()));
        }

        // normalize all normals 
        for (int i = 0; i< numNorms ;i++)
            norms[i].normalize();
        
        std::vector<int32_t> lineSetCoords;
        for (std::map<int, std::vector<int32_t> >::iterator it = lineSetMap.begin(); it != lineSetMap.end(); ++it) {
            lineSetCoords.insert(lineSetCoords.end(), it->second.begin(), it->second.end());
            lineSetCoords.push_back(-1);
        }

        // preset the index vector size
        numLines =  lineSetCoords.size();
        lineset ->coordIndex .setNum(numLines);
        int32_t* lines = lineset ->coordIndex  .startEditing();

        int l=0;
        for (std::vector<int32_t>::const_iterator it=lineSetCoords.begin();it!=lineSetCoords.end();++it,l++)
            lines[l] = *it;

        // end the editing of the nodes
        coords  ->point       .finishEditing();
        pcoords ->point       .finishEditing();
        norm    ->vector      .finishEditing();
        faceset ->coordIndex  .finishEditing();
        faceset ->partIndex   .finishEditing();
        lineset ->coordIndex  .finishEditing();
    }
    catch (Base::Exception &e) {
        FC_ERR("Failed to compute Inventor representation for the shape of " << pcObject->getFullName() << ": " << e.what());
    }
    catch (const Standard_Failure& e) {
        FC_ERR("Cannot compute Inventor representation for the shape of "
               << pcObject->getFullName() << ": " << e.GetMessageString());
    }
    catch (...) {
        FC_ERR("Cannot compute Inventor representation for the shape of " << pcObject->getFullName());
    }

    // printing some information
    FC_TRACE(getFullName() << " update time: " << Base::TimeInfo::diffTimeF(start_time,Base::TimeInfo()));
    FC_TRACE("Shape tria info: Faces:" << numFaces << " Edges:" << numEdges 
             << " Points:" << numPoints << " Nodes:" << numNodes
             << " Triangles:" << numTriangles << " IdxVec:" << numLines);
    VisualTouched = false;

    // The material has to be checked again (#0001736)
    setHighlightedFaces(DiffuseColor.getValues());
    setHighlightedEdges(LineColorArray.getValues());
    setHighlightedPoints(PointColorArray.getValue());
}

void ViewProviderPartExt::forceUpdate(bool enable) {
    if(enable) {
        if(++forceUpdateCount == 1) {
            if (!isShow() && VisualTouched)
                updateVisual();
        }
    }else if(forceUpdateCount)
        --forceUpdateCount;
}

PyObject* ViewProviderPartExt::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderPartExtPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

void ViewProviderPartExt::beforeDelete()
{
    setStatus(Gui::Detach, true);
    inherited::beforeDelete();
    // clear coin nodes to free up some memory
    updateVisual();
}

void ViewProviderPartExt::reattach(App::DocumentObject *obj)
{
    inherited::reattach(obj);
    if(isUpdateForced() || Visibility.getValue()) 
        updateVisual();
}

void ViewProviderPartExt::finishRestoring()
{
    inherited::finishRestoring();

    auto syncMaterial = [](const App::Material &Mat, SoMaterial *pcMaterial) {
        pcMaterial->ambientColor.setValue(Mat.ambientColor.r,Mat.ambientColor.g,Mat.ambientColor.b);
        pcMaterial->specularColor.setValue(Mat.specularColor.r,Mat.specularColor.g,Mat.specularColor.b);
        pcMaterial->emissiveColor.setValue(Mat.emissiveColor.r,Mat.emissiveColor.g,Mat.emissiveColor.b);
        pcMaterial->shininess.setValue(Mat.shininess);
        if (pcMaterial->diffuseColor.getNum() == 1)
            pcMaterial->transparency.setValue(Mat.transparency);
    };
    syncMaterial(LineMaterial.getValue(), pcLineMaterial);
    syncMaterial(PointMaterial.getValue(), pcPointMaterial);
    syncMaterial(ShapeMaterial.getValue(), pcShapeMaterial);

    if(VisualTouched && (isUpdateForced() || Visibility.getValue()))
        updateVisual();
}
