/***************************************************************************
 *   Copyright (c) 2004 Jrgen Riegel <juergen.riegel@web.de>               *
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
# include <Poly_Polygon3D.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Tool.hxx>
# include <BRepTools.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <GeomAbs_CurveType.hxx>
# include <GeomAbs_SurfaceType.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineSurface.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomLProp_SLProps.hxx>
# include <gp_Trsf.hxx>
# include <Poly_Array1OfTriangle.hxx>
# include <Poly_Triangulation.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp_Explorer.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Poly_PolygonOnTriangulation.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TopTools_ListOfShape.hxx>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoLocateHighlight.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoNormalBinding.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoSphere.h>
# include <Inventor/nodes/SoScale.h>
# include <QWidget>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/SoFCSelection.h>
#include <Gui/Selection.h>
#include <Gui/View3DInventorViewer.h>


#include "ViewProvider.h"
#include "SoFCShapeObject.h"

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>


using namespace PartGui;

#if defined(FC_USE_FAST_SHAPE_RENDERING)

PROPERTY_SOURCE(PartGui::ViewProviderPart, PartGui::ViewProviderPartExt)


ViewProviderPart::ViewProviderPart()
{
}

ViewProviderPart::~ViewProviderPart()
{
}

bool ViewProviderPart::doubleClicked(void)
{
    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    Gui::Command::openCommand(Msg.c_str());
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.setEdit('%s',0)",
                            this->pcObject->getNameInDocument());
    return true;
}

void ViewProviderPart::applyColor(const Part::ShapeHistory& hist,
                                  const std::vector<App::Color>& colBase,
                                  std::vector<App::Color>& colBool)
{
    std::map<int, std::vector<int> >::const_iterator jt;
    // apply color from modified faces
    for (jt = hist.shapeMap.begin(); jt != hist.shapeMap.end(); ++jt) {
        std::vector<int>::const_iterator kt;
        for (kt = jt->second.begin(); kt != jt->second.end(); ++kt) {
            colBool[*kt] = colBase[jt->first];
        }
    }
}

#else
PROPERTY_SOURCE(PartGui::ViewProviderPart, PartGui::ViewProviderPartBase)


ViewProviderPart::ViewProviderPart()
{
}

ViewProviderPart::~ViewProviderPart()
{
}
#endif

// ----------------------------------------------------------------------------

void ViewProviderShapeBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& nodes) const
{
}

void ViewProviderShapeBuilder::createShape(const App::Property* prop, SoSeparator* coords) const
{
}


PROPERTY_SOURCE(PartGui::ViewProviderPartBase, Gui::ViewProviderGeometryObject)


//**************************************************************************
// Construction/Destruction

App::PropertyFloatConstraint::Constraints ViewProviderPartBase::floatRange = {1.0,64.0,1.0};
const char* ViewProviderPartBase::LightingEnums[]= {"One side","Two side",NULL};

ViewProviderPartBase::ViewProviderPartBase() : pcControlPoints(0)
{
    App::Material mat;
    mat.ambientColor.set(0.2f,0.2f,0.2f);
    mat.diffuseColor.set(0.1f,0.1f,0.1f);
    mat.specularColor.set(0.0f,0.0f,0.0f);
    mat.emissiveColor.set(0.0f,0.0f,0.0f);
    mat.shininess = 1.0f;
    mat.transparency = 0.0f;
    ADD_PROPERTY(LineMaterial,(mat));
    ADD_PROPERTY(PointMaterial,(mat));
    ADD_PROPERTY(LineColor,(mat.diffuseColor));
    ADD_PROPERTY(PointColor,(mat.diffuseColor));
    ADD_PROPERTY(LineWidth,(2.0f));
    LineWidth.setConstraints(&floatRange);
    PointSize.setConstraints(&floatRange);
    ADD_PROPERTY(PointSize,(2.0f));
    ADD_PROPERTY(ControlPoints,(false));
    ADD_PROPERTY(Lighting,(1));
    Lighting.setEnums(LightingEnums);

    EdgeRoot = new SoSeparator();
    EdgeRoot->ref();
    FaceRoot = new SoSeparator();
    FaceRoot->ref();
    VertexRoot = new SoSeparator();
    VertexRoot->ref();
    pcLineMaterial = new SoMaterial;
    pcLineMaterial->ref();
    LineMaterial.touch();

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

    sPixmap = "Tree_Part";
    loadParameter();
}

ViewProviderPartBase::~ViewProviderPartBase()
{
    EdgeRoot->unref();
    FaceRoot->unref();
    VertexRoot->unref();
    pcLineMaterial->unref();
    pcPointMaterial->unref();
    pcLineStyle->unref();
    pcPointStyle->unref();
    pShapeHints->unref();
}

void ViewProviderPartBase::onChanged(const App::Property* prop)
{
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
    }
    else if (prop == &PointColor) {
        const App::Color& c = PointColor.getValue();
        pcPointMaterial->diffuseColor.setValue(c.r,c.g,c.b);
        if (c != PointMaterial.getValue().diffuseColor)
        PointMaterial.setDiffuseColor(c);
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
    else if (prop == &ControlPoints) {
        App::DocumentObject* obj = this->pcObject;
        App::Property* shape = obj->getPropertyByName("Shape");
        showControlPoints(ControlPoints.getValue(), shape);
    }
    else if (prop == &Lighting) {
        if (Lighting.getValue() == 0)
            pShapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        else
            pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    }
    else {
        ViewProviderGeometryObject::onChanged(prop);
    }
}

void ViewProviderPartBase::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderGeometryObject::attach(pcFeat);

    SoGroup* pcNormalRoot = new SoGroup();
    SoGroup* pcFlatRoot = new SoGroup();
    SoGroup* pcWireframeRoot = new SoGroup();
    SoGroup* pcPointsRoot = new SoGroup();

    // enable two-side rendering
    pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    // Avoid any Z-buffer artefacts, so that the lines always appear on top of the faces
    // The correct order is Edges, Polygon offset, Faces.
    SoPolygonOffset* offset = new SoPolygonOffset();

    // normal viewing with edges and points
    pcNormalRoot->addChild(pShapeHints);
    pcNormalRoot->addChild(EdgeRoot);
    pcNormalRoot->addChild(offset);
    pcNormalRoot->addChild(FaceRoot);
    pcNormalRoot->addChild(VertexRoot);

    // just faces with no edges or points
    pcFlatRoot->addChild(pShapeHints);
    pcFlatRoot->addChild(FaceRoot);

    // only edges
    pcWireframeRoot->addChild(EdgeRoot);
    pcWireframeRoot->addChild(VertexRoot);

    // normal viewing with edges and points
    pcPointsRoot->addChild(VertexRoot);

    // putting all together with the switch
    addDisplayMaskMode(pcNormalRoot, "Flat Lines");
    addDisplayMaskMode(pcFlatRoot, "Shaded");
    addDisplayMaskMode(pcWireframeRoot, "Wireframe");
    addDisplayMaskMode(pcPointsRoot, "Point");
}

void ViewProviderPartBase::setDisplayMode(const char* ModeName)
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

std::vector<std::string> ViewProviderPartBase::getDisplayModes(void) const
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

void ViewProviderPartBase::shapeInfoCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent *)n->getEvent();
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
        n->setHandled();
        view->setEditing(false);
        view->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
        view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), shapeInfoCallback);
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
        const SoPickedPoint * point = n->getPickedPoint();
        if (point == NULL) {
            Base::Console().Message("No point picked.\n");
            return;
        }

        n->setHandled();

        // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
        // really from the mesh we render and not from any other geometry
        Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
        if (!vp || !vp->getTypeId().isDerivedFrom(ViewProviderPartBase::getClassTypeId()))
            return;
        ViewProviderPartBase* that = static_cast<ViewProviderPartBase*>(vp);
        TopoDS_Shape sh = that->getShape(point);
        if (!sh.IsNull()) {
            SbVec3f pt = point->getPoint();
            Base::Console().Message("(%.6f, %.6f, %.6f, %d)\n", pt[0], pt[1], pt[2], sh.HashCode(IntegerLast()));
        }
    }
}

TopoDS_Shape ViewProviderPartBase::getShape(const SoPickedPoint* point) const
{
    if (point && point->getPath()->getTail()->getTypeId().isDerivedFrom(SoVertexShape::getClassTypeId())) {
        SoVertexShape* vs = static_cast<SoVertexShape*>(point->getPath()->getTail());
        std::map<SoVertexShape*, TopoDS_Shape>::const_iterator it = vertexShapeMap.find(vs);
        if (it != vertexShapeMap.end())
            return it->second;
    }

    return TopoDS_Shape();
}

bool ViewProviderPartBase::loadParameter()
{
    bool changed = false;
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Part");
    float deviation = hGrp->GetFloat("MeshDeviation",0.2);
    bool novertexnormals = hGrp->GetBool("NoPerVertexNormals",false);
    bool qualitynormals = hGrp->GetBool("QualityNormals",false);

    if (this->meshDeviation != deviation) {
        this->meshDeviation = deviation;
        changed = true;
    }
    if (this->noPerVertexNormals != novertexnormals) {
        this->noPerVertexNormals = novertexnormals;
        changed = true;
    }
    if (this->qualityNormals != qualitynormals) {
        this->qualityNormals = qualitynormals;
        changed = true;
    }

    return changed;
}

void ViewProviderPartBase::reload()
{
    if (loadParameter()) {
        App::Property* shape     = pcObject->getPropertyByName("Shape");
        if (shape) update(shape);
    }
}

void ViewProviderPartBase::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        TopoDS_Shape cShape = static_cast<const Part::PropertyPartShape*>(prop)->getValue();

        // clear anchor nodes
        vertexShapeMap.clear();
        EdgeRoot->removeAllChildren();
        FaceRoot->removeAllChildren();
        VertexRoot->removeAllChildren();
        // do nothing if shape is empty
        if (cShape.IsNull())
            return;

        try {
            // creating the mesh on the data structure
            Bnd_Box bounds;
            BRepBndLib::Add(cShape, bounds);
            bounds.SetGap(0.0);
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            Standard_Real deflection = ((xMax-xMin)+(yMax-yMin)+(zMax-zMin))/300.0 *
                this->meshDeviation;

            BRepMesh::Mesh(cShape,deflection);
            //BRepMesh_IncrementalMesh MESH(cShape,meshDeviation);
            // We must reset the location here because the transformation data
            // are set in the placement property
            TopLoc_Location aLoc;
            cShape.Location(aLoc);
            computeFaces   (FaceRoot,cShape,deflection);
            computeEdges   (EdgeRoot,cShape);
            computeVertices(VertexRoot,cShape);
            // NOTE: Cleaning the triangulation may cause problems on some algorithms like BOP
            //BRepTools::Clean(cShape); // remove triangulation

            // update control points if there
            if (pcControlPoints) {
                pcControlPoints->removeAllChildren();
                showControlPoints(this->ControlPoints.getValue(), prop);
            }
        }
        catch (...){
            Base::Console().Error("Cannot compute Inventor representation for the shape of %s.\n", 
                                  pcObject->getNameInDocument());
            // For many 64-bit Linux systems this error is due to a missing compiler switch
            // Note: echo "" | g++ -E -dM -x c - | sort | less prints a list of gcc-internals.
#if defined(__GNUC__) && defined(__LP64__) && !defined(_OCC64)
            std::string exe = App::Application::Config()["ExeName"];
            Base::Console().Error("IMPORTANT: Apparently, %s isn't built with the OpenCASCADE-internal "
                                  "define '_OCC64'.\nReconfigure the build system with the missing define "
                                  "(e.g. ./configure CXXFLAGS=\"-D_OCC64\") and run a complete rebuild.\n",
                                  exe.c_str());
#endif
        }
    }
    Gui::ViewProviderGeometryObject::updateData(prop);
}

Standard_Boolean ViewProviderPartBase::computeEdges(SoGroup* EdgeRoot, const TopoDS_Shape &myShape)
{
    //TopExp_Explorer ex;

    EdgeRoot->addChild(pcLineMaterial);  
    EdgeRoot->addChild(pcLineStyle);

    // get a indexed map of edges
    TopTools_IndexedMapOfShape M;
    TopExp::MapShapes(myShape, TopAbs_EDGE, M);

    // build up map edge->face
    TopTools_IndexedDataMapOfShapeListOfShape edge2Face;
    TopExp::MapShapesAndAncestors(myShape, TopAbs_EDGE, TopAbs_FACE, edge2Face);

    //int i=1;
    //for (ex.Init(myShape, TopAbs_EDGE); ex.More(); ex.Next(),i++) {
    for (int i=0; i < M.Extent(); i++) {
        //if(i>12)continue;
        // get the shape and mesh it
        //const TopoDS_Edge& aEdge = TopoDS::Edge(ex.Current());
        const TopoDS_Edge& aEdge = TopoDS::Edge(M(i+1));

        // getting the transformation of the shape/face
        gp_Trsf myTransf;
        TopLoc_Location aLoc;

        // try to triangulate the edge
        Handle(Poly_Polygon3D) aPoly = BRep_Tool::Polygon3D(aEdge, aLoc);

        SbVec3f* vertices;
        Standard_Integer nbNodesInFace;

        // triangulation succeeded?
        if (!aPoly.IsNull()) {
            if (!aLoc.IsIdentity()) {
                myTransf = aLoc.Transformation();
            }
            // take the edge's triangulation
            //
            // getting size and create the array
            nbNodesInFace = aPoly->NbNodes();
            vertices = new SbVec3f[nbNodesInFace];

            const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();

            gp_Pnt V;
            for (Standard_Integer i=0;i < nbNodesInFace;i++) {
                V = Nodes(i+1);
                V.Transform(myTransf);
                vertices[i].setValue((float)(V.X()),(float)(V.Y()),(float)(V.Z()));
            }
        }
        else {
            // the edge has not its own triangulation, but then a face the edge is attached to
            // must provide this triangulation

            // Look for one face in our map (it doesn't care which one we take)
            const TopoDS_Face& aFace = TopoDS::Face(edge2Face.FindFromKey(aEdge).First());

            // take the face's triangulation instead
            Handle(Poly_Triangulation) aPolyTria = BRep_Tool::Triangulation(aFace,aLoc);
            if (!aLoc.IsIdentity()) {
                myTransf = aLoc.Transformation();
            }

            //if (aPolyTria.IsNull()) // actually this shouldn't happen at all
            //    throw Base::Exception("Empty face trianglutaion\n");
            if (aPolyTria.IsNull()) return false;

            // this holds the indices of the edge's triangulation to the actual points
            Handle(Poly_PolygonOnTriangulation) aPoly = BRep_Tool::PolygonOnTriangulation(aEdge, aPolyTria, aLoc);
            if (aPoly.IsNull())
                continue; // polygon does not exist

            // getting size and create the array
            nbNodesInFace = aPoly->NbNodes();
            vertices = new SbVec3f[nbNodesInFace];

            const TColStd_Array1OfInteger& indices = aPoly->Nodes();
            const TColgp_Array1OfPnt& Nodes = aPolyTria->Nodes();

            gp_Pnt V;
            int pos = 0;
            // go through the index array
            for (Standard_Integer i=indices.Lower();i <= indices.Upper();i++) {
                V = Nodes(indices(i));
                V.Transform(myTransf);
                vertices[pos++].setValue((float)(V.X()),(float)(V.Y()),(float)(V.Z()));
            }
        }

        // define vertices
        SoCoordinate3 * coords = new SoCoordinate3;
        coords->point.setValues(0,nbNodesInFace, vertices);
        delete [] vertices;
        EdgeRoot->addChild(coords);

        // define the indexed face set
        Gui::SoFCSelection* sel = createFromSettings();
        SbString name("Edge");
        name += SbString(i+1);
        sel->objectName = pcObject->getNameInDocument();
        sel->documentName = pcObject->getDocument()->getName();
        sel->subElementName = name;
        sel->style = Gui::SoFCSelection::EMISSIVE_DIFFUSE;
        //sel->highlightMode = Gui::SoFCSelection::AUTO;
        //sel->selectionMode = Gui::SoFCSelection::SEL_ON;

        SoLineSet * lineset = new SoLineSet;
        sel->addChild(lineset);
        EdgeRoot->addChild(sel);
        vertexShapeMap[lineset] = aEdge;
    }

    return true;
}

Standard_Boolean ViewProviderPartBase::computeVertices(SoGroup* VertexRoot, const TopoDS_Shape &myShape)
{
#if 0 // new implementation of computeVertice
    VertexRoot->addChild(pcPointMaterial);  
    VertexRoot->addChild(pcPointStyle);

    // define vertices
    SoCoordinate3 * coords = new SoCoordinate3;
    VertexRoot->addChild(coords);

    TopExp_Explorer ex;
    int iCnt=0;
    for (ex.Init(myShape, TopAbs_VERTEX); ex.More(); ex.Next()) {
        iCnt++;
    }

    coords->point.setNum(iCnt);

    int i=1;
    for (ex.Init(myShape, TopAbs_VERTEX); ex.More(); ex.Next()) {
        // get the shape
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(ex.Current());
        gp_Pnt pnt = BRep_Tool::Pnt(aVertex);
        coords->point.set1Value(i++, (float)pnt.X(), (float)pnt.Y(), (float)pnt.Z());
    }

    // use only one selection node otherwise the Inventor tree becomes too slow
    Gui::SoFCSelection* sel = createFromSettings();
    SbString name("Point");
    name += SbString(i);
    sel->objectName = pcObject->getNameInDocument();
    sel->documentName = pcObject->getDocument()->getName();
    sel->subElementName = name;
    sel->style = Gui::SoFCSelection::EMISSIVE_DIFFUSE;
    //sel->highlightMode = Gui::SoFCSelection::AUTO;
    //sel->selectionMode = Gui::SoFCSelection::SEL_ON;

    SoPointSet * pointset = new SoPointSet;
    sel->addChild(pointset);
    VertexRoot->addChild(sel);

    return true;
#else
    VertexRoot->addChild(pcPointMaterial);  
    VertexRoot->addChild(pcPointStyle);


    // get a indexed map of edges
    TopTools_IndexedMapOfShape M;
    TopExp::MapShapes(myShape, TopAbs_VERTEX, M);
    

    //int i=0;
    //for (ex.Init(myShape, TopAbs_VERTEX); ex.More(); ex.Next()) {
    for (int i=0; i<M.Extent(); i++)
    {
        const TopoDS_Vertex& aVertex = TopoDS::Vertex(M(i+1));

        // each point has its own selection node
        Gui::SoFCSelection* sel = createFromSettings();
        SbString name("Point");
        name += SbString(i+1);
        sel->objectName = pcObject->getNameInDocument();
        sel->documentName = pcObject->getDocument()->getName();
        sel->subElementName = name;
        sel->style = Gui::SoFCSelection::EMISSIVE_DIFFUSE;
        //sel->highlightMode = Gui::SoFCSelection::AUTO;
        //sel->selectionMode = Gui::SoFCSelection::SEL_ON;

        // define the vertices
        SoCoordinate3 * coords = new SoCoordinate3;
        coords->point.setNum(1);
        VertexRoot->addChild(coords);


        // get the shape
        //const TopoDS_Vertex& aVertex = TopoDS::Vertex(ex.Current());
        gp_Pnt pnt = BRep_Tool::Pnt(aVertex);
        coords->point.set1Value(0, (float)pnt.X(), (float)pnt.Y(), (float)pnt.Z());


        SoPointSet * pointset = new SoPointSet;
        sel->addChild(pointset);
        VertexRoot->addChild(sel);
        //i++;
    }


    return true;

#endif
}

Standard_Boolean ViewProviderPartBase::computeFaces(SoGroup* FaceRoot, const TopoDS_Shape &myShape, double defl)
{
    TopExp_Explorer ex;

    FaceRoot->addChild(pcShapeMaterial);

//  BRepMesh::Mesh(myShape,1.0);
    BRepMesh_IncrementalMesh MESH(myShape,defl);

    int i = 1;
    for (ex.Init(myShape, TopAbs_FACE); ex.More(); ex.Next(),i++) {
        // get the shape and mesh it
        const TopoDS_Face& aFace = TopoDS::Face(ex.Current());


        // this block mesh the face and transfers it in a C array of vertices and face indexes
        Standard_Integer nbNodesInFace,nbTriInFace;
        SbVec3f* vertices=0;
        SbVec3f* vertexnormals=0;
        int32_t* cons=0;

        transferToArray(aFace,&vertices,&vertexnormals,&cons,nbNodesInFace,nbTriInFace);

        if (!vertices)
            continue;

        if (!this->noPerVertexNormals) {
            // define normals (this is optional)
            SoNormal * norm = new SoNormal;
            norm->vector.setValues(0, nbNodesInFace, vertexnormals);
            FaceRoot->addChild(norm);

            // bind one normal per face
            SoNormalBinding * normb = new SoNormalBinding;
            normb->value = SoNormalBinding::PER_VERTEX_INDEXED;
            FaceRoot->addChild(normb);
        }

        // define vertices
        SoCoordinate3 * coords = new SoCoordinate3;
        coords->point.setValues(0,nbNodesInFace, vertices);
        FaceRoot->addChild(coords);

        // Turns on backface culling
        //      SoShapeHints * hints = new SoShapeHints;
        //      hints->vertexOrdering = SoShapeHints::CLOCKWISE ;
        //      hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE ;
        //      hints->shapeType = SoShapeHints::SOLID;
        //      hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
        //      root->addChild(hints);

        //SoDrawStyle *Stype = new SoDrawStyle();
        //Stype->pointSize.setValue(3.0);
        //Stype->style.setValue( SoDrawStyle::POINTS );

        //SoPointSet *PtSet = new SoPointSet;
        //root->addChild(PtSet);

        // define the indexed face set
        Gui::SoFCSelection* sel = createFromSettings();
        SbString name("Face");
        name += SbString(i);
        sel->objectName = pcObject->getNameInDocument();
        sel->documentName = pcObject->getDocument()->getName();
        sel->subElementName = name;
        sel->style = Gui::SoFCSelection::EMISSIVE;
        //sel->highlightMode = Gui::SoFCSelection::AUTO;
        //sel->selectionMode = Gui::SoFCSelection::SEL_ON;

        SoIndexedFaceSet * faceset = new SoIndexedFaceSet;
        faceset->coordIndex.setValues(0,4*nbTriInFace,(const int32_t*) cons);
        sel->addChild(faceset);
        FaceRoot->addChild(sel);
        vertexShapeMap[faceset] = aFace;


        //    Base::Console().Log("Inventor tree:\n%s",buffer_writeaction(root).c_str());

        delete [] vertexnormals;
        delete [] vertices;
        delete [] cons;
    } // end of face loop

    return true;
}

void ViewProviderPartBase::transferToArray(const TopoDS_Face& aFace,SbVec3f** vertices,SbVec3f** vertexnormals,
                                           int32_t** cons,int &nbNodesInFace,int &nbTriInFace )
{
    TopLoc_Location aLoc;

    // doing the meshing and checking the result
    //BRepMesh_IncrementalMesh MESH(aFace,fDeflection);
    Handle(Poly_Triangulation) aPoly = BRep_Tool::Triangulation(aFace,aLoc);
    //if (aPoly.IsNull()) throw Base::Exception("Empty face trianglutaion\n");
    if (aPoly.IsNull()) return;

    // getting the transformation of the shape/face
    gp_Trsf myTransf;
    Standard_Boolean identity = true;
    if (!aLoc.IsIdentity()) {
        identity = false;
        myTransf = aLoc.Transformation();
    }

    Standard_Integer i;
    // geting size and create the array
    nbNodesInFace = aPoly->NbNodes();
    nbTriInFace = aPoly->NbTriangles();
    *vertices = new SbVec3f[nbNodesInFace];
    *vertexnormals = new SbVec3f[nbNodesInFace];
    for(i=0;i < nbNodesInFace;i++) {
        (*vertexnormals)[i]= SbVec3f(0.0,0.0,0.0);
    }

    *cons = new int32_t[4*(nbTriInFace)];

    // check orientation
    TopAbs_Orientation orient = aFace.Orientation();

    // cycling through the poly mesh
    const Poly_Array1OfTriangle& Triangles = aPoly->Triangles();
    const TColgp_Array1OfPnt& Nodes = aPoly->Nodes();
    for (i=1;i<=nbTriInFace;i++) {
        // Get the triangle
        Standard_Integer N1,N2,N3;
        Triangles(i).Get(N1,N2,N3);

        // change orientation of the triangles
        if ( orient != TopAbs_FORWARD ) {
            Standard_Integer tmp = N1;
            N1 = N2;
            N2 = tmp;
        }

        gp_Pnt V1 = Nodes(N1);
        gp_Pnt V2 = Nodes(N2);
        gp_Pnt V3 = Nodes(N3);

        // transform the vertices to the place of the face
        if (!identity) {
            V1.Transform(myTransf);
            V2.Transform(myTransf);
            V3.Transform(myTransf);
        }

        if (!this->noPerVertexNormals) {
            // Calculate triangle normal
            gp_Vec v1(V1.X(),V1.Y(),V1.Z()),v2(V2.X(),V2.Y(),V2.Z()),v3(V3.X(),V3.Y(),V3.Z());
            gp_Vec Normal = (v2-v1)^(v3-v1); 

            //Standard_Real Area = 0.5 * Normal.Magnitude();

            // add the triangle normal to the vertex normal for all points of this triangle
            (*vertexnormals)[N1-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());
            (*vertexnormals)[N2-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());
            (*vertexnormals)[N3-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());
        }

        (*vertices)[N1-1].setValue((float)(V1.X()),(float)(V1.Y()),(float)(V1.Z()));
        (*vertices)[N2-1].setValue((float)(V2.X()),(float)(V2.Y()),(float)(V2.Z()));
        (*vertices)[N3-1].setValue((float)(V3.X()),(float)(V3.Y()),(float)(V3.Z()));

        int j = i - 1;
        N1--; N2--; N3--;
        (*cons)[4*j] = N1; (*cons)[4*j+1] = N2; (*cons)[4*j+2] = N3; (*cons)[4*j+3] = SO_END_FACE_INDEX;
    }

    // normalize all vertex normals
    for(i=0;i < nbNodesInFace;i++) {
        if (this->qualityNormals) {
            gp_Dir clNormal;

            try {
                Handle_Geom_Surface Surface = BRep_Tool::Surface(aFace);

                gp_Pnt vertex((*vertices)[i][0], (*vertices)[i][1], (*vertices)[i][2]);
                GeomAPI_ProjectPointOnSurf ProPntSrf(vertex, Surface);
                Standard_Real fU, fV; ProPntSrf.Parameters(1, fU, fV);

                GeomLProp_SLProps clPropOfFace(Surface, fU, fV, 2, gp::Resolution());

                clNormal = clPropOfFace.Normal();
                SbVec3f temp = SbVec3f(clNormal.X(),clNormal.Y(),clNormal.Z());
                //Base::Console().Log("unterschied:%.2f",temp.dot((*vertexnormals)[i]));

                if ( temp.dot((*vertexnormals)[i]) < 0 )
                    temp = -temp;
                (*vertexnormals)[i] = temp;

            }
            catch(...){}
        }
        else if ((*vertexnormals)[i].sqrLength() > 0.001){
            (*vertexnormals)[i].normalize();
        }
    }
}

void ViewProviderPartBase::showControlPoints(bool show, const App::Property* prop)
{
    if (!pcControlPoints && show) {
        pcControlPoints = new SoSwitch();
        pcRoot->addChild(pcControlPoints);
    }

    if (pcControlPoints) {
        pcControlPoints->whichChild = (show ? SO_SWITCH_ALL : SO_SWITCH_NONE);
    }

    if (!show || !pcControlPoints || pcControlPoints->getNumChildren() > 0)
        return;

    // ask for the property we are interested in
    if (prop && prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        const TopoDS_Shape& shape = static_cast<const Part::PropertyPartShape*>(prop)->getValue();
        if (shape.IsNull())
            return; // empty shape
        switch (shape.ShapeType())
        {
        case TopAbs_EDGE:
            {
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                showControlPointsOfEdge(edge);
            }   break;
        case TopAbs_WIRE:
            {
                const TopoDS_Wire& wire = TopoDS::Wire(shape);
                showControlPointsOfWire(wire);
            }   break;
        case TopAbs_FACE:
            {
                const TopoDS_Face& face = TopoDS::Face(shape);
                showControlPointsOfFace(face);
            }   break;
        default:
            break;
        }
    }
}

void ViewProviderPartBase::showControlPointsOfEdge(const TopoDS_Edge& edge)
{
    std::list<gp_Pnt> poles, knots; 
    Standard_Integer nCt=0;
    BRepAdaptor_Curve curve(edge);
    switch (curve.GetType())
    {
    case GeomAbs_BezierCurve:
        {
            Handle(Geom_BezierCurve) hBezier = curve.Bezier();
            nCt = hBezier->NbPoles();
            for (Standard_Integer i = 1; i <= nCt; i++)
                poles.push_back(hBezier->Pole(i));
            if (hBezier->IsClosed()) {
                nCt++;
                poles.push_back(hBezier->Pole(1));
            }
        }   break;
    case GeomAbs_BSplineCurve:
        {
            Handle(Geom_BSplineCurve) hBSpline = curve.BSpline();
            nCt = hBSpline->NbPoles();
            for (Standard_Integer i = 1; i <= nCt; i++)
                poles.push_back(hBSpline->Pole(i));
            if (hBSpline->IsClosed()) {
                nCt++;
                poles.push_back(hBSpline->Pole(1));
            }
            for (Standard_Integer i = hBSpline->FirstUKnotIndex()+1; i <= hBSpline->LastUKnotIndex()-1; i++)
                knots.push_back(hBSpline->Value(hBSpline->Knot(i)));
        }   break;
    default:
        break;
    }

    if (poles.empty())
        return; // nothing to do

    SoCoordinate3 * coords = new SoCoordinate3;
    coords->point.setNum(nCt + knots.size());

    int index=0;
    SbVec3f* verts = coords->point.startEditing();
    for (std::list<gp_Pnt>::iterator p = poles.begin(); p != poles.end(); ++p) {
        verts[index++].setValue((float)p->X(), (float)p->Y(), (float)p->Z());
    }
    for (std::list<gp_Pnt>::iterator k = knots.begin(); k != knots.end(); ++k) {
        verts[index++].setValue((float)k->X(), (float)k->Y(), (float)k->Z());
    }
    coords->point.finishEditing();


    SoFCControlPoints* control = new SoFCControlPoints();
    control->numPolesU = nCt;
    control->numPolesV = 1;

    SoSeparator* nodes = new SoSeparator();
    nodes->addChild(coords);
    nodes->addChild(control);

    pcControlPoints->addChild(nodes);
}

void ViewProviderPartBase::showControlPointsOfWire(const TopoDS_Wire& wire)
{
    TopoDS_Iterator it;
    for (it.Initialize(wire); it.More(); it.Next()) {
        if (it.Value().ShapeType() == TopAbs_EDGE) {
            const TopoDS_Edge& edge = TopoDS::Edge(it.Value());
            BRepAdaptor_Curve curve(edge);

            std::list<gp_Pnt> poles, knots; 
            gp_Pnt start, end;
            switch (curve.GetType())
            {
            case GeomAbs_BezierCurve:
                {
                    Handle(Geom_BezierCurve) hBezier = curve.Bezier();
                    for (Standard_Integer i = 1; i <= hBezier->NbPoles(); i++)
                        poles.push_back(hBezier->Pole(i));
                    start = hBezier->StartPoint();
                    end   = hBezier->EndPoint();
                }   break;
            case GeomAbs_BSplineCurve:
                {
                    Handle(Geom_BSplineCurve) hBSpline = curve.BSpline();
                    for (Standard_Integer i = 1; i <= hBSpline->NbPoles(); i++)
                        poles.push_back(hBSpline->Pole(i));
                    start = hBSpline->StartPoint();
                    end   = hBSpline->EndPoint();
                    for (Standard_Integer i = hBSpline->FirstUKnotIndex()+1; i <= hBSpline->LastUKnotIndex()-1; i++)
                        knots.push_back(hBSpline->Value(hBSpline->Knot(i)));
                }   break;
            default:
                break;
            }
        }
    }
}

void ViewProviderPartBase::showControlPointsOfFace(const TopoDS_Face& face)
{
    std::list<gp_Pnt> knots;
    std::vector<std::vector<gp_Pnt> > poles;
    Standard_Integer nCtU=0, nCtV=0;
    BRepAdaptor_Surface surface(face); 

    BRepAdaptor_Surface clSurface(face); 
    switch (clSurface.GetType())
    {
    case GeomAbs_BezierSurface:
        {
            Handle(Geom_BezierSurface) hBezier = surface.Bezier();
            nCtU = hBezier->NbUPoles();
            nCtV = hBezier->NbVPoles();
            poles.resize(nCtU);
            for (Standard_Integer u = 1; u <= nCtU; u++) {
                poles[u-1].resize(nCtV);
                for (Standard_Integer v = 1; v <= nCtV; v++)
                    poles[u-1][v-1] = hBezier->Pole(u, v);
            }
        }   break;
    case GeomAbs_BSplineSurface:
        {
            Handle(Geom_BSplineSurface) hBSpline = surface.BSpline();
            nCtU = hBSpline->NbUPoles();
            nCtV = hBSpline->NbVPoles();
            poles.resize(nCtU);
            for (Standard_Integer u = 1; u <= nCtU; u++) {
                poles[u-1].resize(nCtV);
                for (Standard_Integer v = 1; v <= nCtV; v++)
                    poles[u-1][v-1] = hBSpline->Pole(u, v);
            }

            //Standard_Integer nKnU = hBSpline->NbUKnots();
            //Standard_Integer nKnV = hBSpline->NbVKnots();
            for (Standard_Integer u = 1; u <= hBSpline->NbUKnots(); u++) {
                for (Standard_Integer v = 1; v <= hBSpline->NbVKnots(); v++)
                    knots.push_back(hBSpline->Value(hBSpline->UKnot(u), hBSpline->VKnot(v)));
            }
        }   break;
    default:
        break;
    }

    if (poles.empty())
        return; // nothing to do

    SoCoordinate3 * coords = new SoCoordinate3;
    coords->point.setNum(nCtU * nCtV + knots.size());

    int index=0;
    SbVec3f* verts = coords->point.startEditing();
    for (std::vector<std::vector<gp_Pnt> >::iterator u = poles.begin(); u != poles.end(); ++u) {
        for (std::vector<gp_Pnt>::iterator v = u->begin(); v != u->end(); ++v) {
            verts[index++].setValue((float)v->X(), (float)v->Y(), (float)v->Z());
        }
    }
    for (std::list<gp_Pnt>::iterator k = knots.begin(); k != knots.end(); ++k) {
        verts[index++].setValue((float)k->X(), (float)k->Y(), (float)k->Z());
    }
    coords->point.finishEditing();


    SoFCControlPoints* control = new SoFCControlPoints();
    control->numPolesU = nCtU;
    control->numPolesV = nCtV;

    //if (knots.size() > 0) {
    //    control->numKnotsU = nKnU;
    //    control->numKnotsV = nKnV;
    //}

    SoSeparator* nodes = new SoSeparator();
    nodes->addChild(coords);
    nodes->addChild(control);

    pcControlPoints->addChild(nodes);
}

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(PartGui::ViewProviderEllipsoid, PartGui::ViewProviderPartBase)

ViewProviderEllipsoid::ViewProviderEllipsoid()
{
    pSphere = new SoSphere();
    pSphere->ref();
    pScaling = new SoScale();
    pScaling->ref();
    sPixmap = "Tree_Part_Ellipsoid_Parametric.svg";
}

ViewProviderEllipsoid::~ViewProviderEllipsoid()
{
    pSphere->unref();
    pScaling->unref();
}

void ViewProviderEllipsoid::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
        const TopoDS_Shape& cShape = static_cast<const Part::PropertyPartShape*>(prop)->getValue();
        // clear anchor nodes
        //vertexShapeMap.clear();
        EdgeRoot->removeAllChildren();
        FaceRoot->removeAllChildren();
        VertexRoot->removeAllChildren();
        // do nothing if shape is empty
        if (cShape.IsNull())
            return;
        App::DocumentObject* object = this->getObject();
        if (object && object->isDerivedFrom(Part::Ellipsoid::getClassTypeId())) {
            double angle1 = static_cast<Part::Ellipsoid*>(object)->Angle1.getValue();
            double angle2 = static_cast<Part::Ellipsoid*>(object)->Angle2.getValue();
            double angle3 = static_cast<Part::Ellipsoid*>(object)->Angle3.getValue();
            float radius1 = static_cast<Part::Ellipsoid*>(object)->Radius1.getValue();
            float radius2 = static_cast<Part::Ellipsoid*>(object)->Radius2.getValue();
            if (angle1 == -90.0 && angle2 == 90.0 && angle3 == 360.0) {
                float scale = radius1/radius2;
                pScaling->scaleFactor.setValue(1,1,scale);
                pSphere->radius.setValue(radius2);
                FaceRoot->addChild(pScaling);
                FaceRoot->addChild(pSphere);
                return; // ok, done
            }
        }

        // if not a full ellipsoid do it the general way
        ViewProviderPartBase::updateData(prop);
    }
    else {
        Gui::ViewProviderGeometryObject::updateData(prop);
    }
}
