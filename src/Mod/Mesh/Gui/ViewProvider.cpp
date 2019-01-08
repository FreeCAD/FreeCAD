/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <stdlib.h>
# include <QAction>
# include <QMenu>
# include <QTimer>
# include <Inventor/SbBox2s.h>
# include <Inventor/SbLine.h>
# include <Inventor/SbPlane.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoToVRML2Action.h>
# include <Inventor/VRMLnodes/SoVRMLGroup.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCallback.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoNormalBinding.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTransform.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/ViewProj.h>

#include <App/Document.h>
#include <App/PropertyLinks.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Flag.h>
#include <Gui/SoFCOffscreenRenderer.h>
#include <Gui/SoFCSelection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/SoFCDB.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/Window.h>
#include <Gui/WaitCursor.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ActionFunction.h>

#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/Triangulation.h>
#include <Mod/Mesh/App/Core/Trim.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Visitor.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/Gui/ViewProviderMeshPy.h>
#include <zipios++/gzipoutputstream.h>
#include <boost/bind.hpp>

#include "ViewProvider.h"
#include "SoFCIndexedFaceSet.h"
#include "SoFCMeshObject.h"


using namespace MeshGui;

using Mesh::Feature;
using MeshCore::MeshKernel;
using MeshCore::MeshPointIterator;
using MeshCore::MeshFacetIterator;
using MeshCore::MeshGeomFacet;
using MeshCore::MeshFacet;

void ViewProviderMeshBuilder::buildNodes(const App::Property* prop, std::vector<SoNode*>& nodes) const
{
    SoCoordinate3 *pcPointsCoord=0;
    SoIndexedFaceSet *pcFaces=0;

    if (nodes.empty()) {
        pcPointsCoord = new SoCoordinate3();
        nodes.push_back(pcPointsCoord);
        pcFaces = new SoIndexedFaceSet();
        nodes.push_back(pcFaces);
    }
    else if (nodes.size() == 2) {
        if (nodes[0]->getTypeId() == SoCoordinate3::getClassTypeId())
            pcPointsCoord = static_cast<SoCoordinate3*>(nodes[0]);
        if (nodes[1]->getTypeId() == SoIndexedFaceSet::getClassTypeId())
            pcFaces = static_cast<SoIndexedFaceSet*>(nodes[1]);
    }

    if (pcPointsCoord && pcFaces)
        createMesh(prop, pcPointsCoord, pcFaces);
}

void ViewProviderMeshBuilder::createMesh(const App::Property* prop, SoCoordinate3* coords, SoIndexedFaceSet* faces) const
{
    const Mesh::PropertyMeshKernel* mesh = static_cast<const Mesh::PropertyMeshKernel*>(prop);
    const MeshCore::MeshKernel& rcMesh = mesh->getValue().getKernel();

    // set the point coordinates
    const MeshCore::MeshPointArray& cP = rcMesh.GetPoints();
    coords->point.setNum(rcMesh.CountPoints());
    SbVec3f* verts = coords->point.startEditing();
    unsigned long i=0;
    for (MeshCore::MeshPointArray::_TConstIterator it = cP.begin(); it != cP.end(); ++it, i++) {
        verts[i].setValue(it->x, it->y, it->z);
    }
    coords->point.finishEditing();

    // set the face indices
    unsigned long j=0;
    const MeshCore::MeshFacetArray& cF = rcMesh.GetFacets();
    faces->coordIndex.setNum(4*rcMesh.CountFacets());
    int32_t* indices = faces->coordIndex.startEditing();
    for (MeshCore::MeshFacetArray::_TConstIterator it = cF.begin(); it != cF.end(); ++it, j++) {
        for (int i=0; i<3; i++) {
            indices[4*j+i] = it->_aulPoints[i];
        }
        indices[4*j+3] = SO_END_FACE_INDEX;
    }
    faces->coordIndex.finishEditing();
}

PROPERTY_SOURCE(MeshGui::ViewProviderExport, Gui::ViewProviderDocumentObject)

ViewProviderExport::ViewProviderExport()
{
}

ViewProviderExport::~ViewProviderExport()
{
}

std::vector<std::string> ViewProviderExport::getDisplayModes(void) const
{
    std::vector<std::string> mode;
    mode.push_back("");
    return mode;
}

const char* ViewProviderExport::getDefaultDisplayMode() const
{
    return "";
}

QIcon ViewProviderExport::getIcon() const
{
    const char * Mesh_Feature_xpm[] = {
        "22 22 6 1",
        ". c None",
        "# c #000000",
        "c c #ffff00",
        "a c #808080",
        "b c #c0c0c0",
        "f c #008000",
        ".............##.......",
        ".............###......",
        ".............#f##.....",
        ".#....####...#ff##....",
        ".##.##....#..#fff##...",
        ".###.........#ffff##..",
        ".####........#fffff##.",
        ".#####.......#ffffff##",
        ".............#########",
        ".####.................",
        "#abab##########.......",
        "#babababababab#.......",
        "#ababababababa#.......",
        "#babab################",
        "#abab##cccccccccccc##.",
        "#bab##cccccccccccc##..",
        "#ab##cccccccccccc##...",
        "#b##cccccccccccc##....",
        "###cccccccccccc##.....",
        "##cccccccccccc##......",
        "###############.......",
        "......................"};
    QPixmap px(Mesh_Feature_xpm);
    return px;
}

// ------------------------------------------------------

App::PropertyFloatConstraint::Constraints ViewProviderMesh::floatRange = {1.0f,64.0f,1.0f};
App::PropertyFloatConstraint::Constraints ViewProviderMesh::angleRange = {0.0f,180.0f,1.0f};
App::PropertyIntegerConstraint::Constraints ViewProviderMesh::intPercent = {0,100,1};
const char* ViewProviderMesh::LightingEnums[]= {"One side","Two side",NULL};

PROPERTY_SOURCE(MeshGui::ViewProviderMesh, Gui::ViewProviderGeometryObject)

ViewProviderMesh::ViewProviderMesh() : pcOpenEdge(0)
{
    ADD_PROPERTY(LineTransparency,(0));
    LineTransparency.setConstraints(&intPercent);
    ADD_PROPERTY(LineWidth,(1.0f));
    LineWidth.setConstraints(&floatRange);
    ADD_PROPERTY(PointSize,(2.0f));
    PointSize.setConstraints(&floatRange);
    ADD_PROPERTY(CreaseAngle,(0.0f));
    CreaseAngle.setConstraints(&angleRange);
    ADD_PROPERTY(OpenEdges,(false));
    ADD_PROPERTY(Coloring,(false));
    ADD_PROPERTY(Lighting,(1));
    Lighting.setEnums(LightingEnums);
    ADD_PROPERTY(LineColor,(0,0,0));

    // Create the selection node
    pcHighlight = Gui::ViewProviderBuilder::createSelection();
    pcHighlight->ref();
    if (pcHighlight->selectionMode.getValue() == Gui::SoFCSelection::SEL_OFF)
        Selectable.setValue(false);

    pcShapeGroup = new SoGroup();
    pcShapeGroup->ref();
    pcHighlight->addChild(pcShapeGroup);

    pOpenColor = new SoBaseColor();
    setOpenEdgeColorFrom(ShapeColor.getValue());
    pOpenColor->ref();

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

    pcMatBinding = new SoMaterialBinding;
    pcMatBinding->value = SoMaterialBinding::OVERALL;
    pcMatBinding->ref();

    pLineColor = new SoMaterial;
    pLineColor->ref();
    LineColor.touch();

    // read the correct shape color from the preferences
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");

    // Mesh color
    App::Color color = ShapeColor.getValue();
    unsigned long current = color.getPackedValue();
    unsigned long setting = hGrp->GetUnsigned("MeshColor", current);
    if (current != setting) {
        color.setPackedValue((uint32_t)setting);
        ShapeColor.setValue(color);
    }
    Transparency.setValue(hGrp->GetInt("MeshTransparency", 0));

    // Line color
    color = LineColor.getValue();
    current = color.getPackedValue();
    setting = hGrp->GetUnsigned("LineColor", current);
    if (current != setting) {
        color.setPackedValue((uint32_t)setting);
        LineColor.setValue(color);
    }
    LineTransparency.setValue(hGrp->GetInt("LineTransparency", 0));

    bool twoside = hGrp->GetBool("TwoSideRendering", false);
    if (twoside) Lighting.setValue(1);
    else Lighting.setValue((long)0);

    bool normal_per_vertex = hGrp->GetBool("VertexPerNormals", false);
    if (normal_per_vertex) {
        double angle = hGrp->GetFloat("CreaseAngle", 0.0);
        CreaseAngle.setValue(angle);
    }

    if (hGrp->GetBool("ShowBoundingBox", false))
        pcHighlight->style = Gui::SoFCSelection::BOX;

    Coloring.setStatus(App::Property::Hidden, true);
}

ViewProviderMesh::~ViewProviderMesh()
{
    pcHighlight->unref();
    pcShapeGroup->unref();
    pOpenColor->unref();
    pcLineStyle->unref();
    pcPointStyle->unref();
    pShapeHints->unref();
    pcMatBinding->unref();
    pLineColor->unref();
}

void ViewProviderMesh::onChanged(const App::Property* prop)
{
    // we're going to change the number of colors to one
    if (prop == &ShapeColor || prop == &ShapeMaterial) {
        pcMatBinding->value = SoMaterialBinding::OVERALL;
    }
    if (prop == &LineTransparency) {
        float trans = LineTransparency.getValue()/100.0f;
        pLineColor->transparency = trans;
    }
    else if (prop == &LineWidth) {
        pcLineStyle->lineWidth = LineWidth.getValue();
    }
    else if (prop == &PointSize) {
        pcPointStyle->pointSize = PointSize.getValue();
    }
    else if (prop == &CreaseAngle) {
        pShapeHints->creaseAngle = Base::toRadians<float>(CreaseAngle.getValue());
    }
    else if (prop == &OpenEdges) {
        showOpenEdges(OpenEdges.getValue());
    }
    else if (prop == &Lighting) {
        if (Lighting.getValue() == 0) {
            pShapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
            //pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        }
        else {
            pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        }
    }
    else if (prop == &LineColor) {
        const App::Color& c = LineColor.getValue();
        pLineColor->diffuseColor.setValue(c.r,c.g,c.b);
    }
    else if (prop == &Coloring) {
        tryColorPerVertex(Coloring.getValue());
    }
    else {
        // Set the inverse color for open edges
        if (prop == &ShapeColor) {
            setOpenEdgeColorFrom(ShapeColor.getValue());
        }
        else if (prop == &ShapeMaterial) {
            setOpenEdgeColorFrom(ShapeMaterial.getValue().diffuseColor);
        }
    }

    ViewProviderGeometryObject::onChanged(prop);
}

void ViewProviderMesh::setOpenEdgeColorFrom(const App::Color& c)
{
    float r=1.0f-c.r; r = r < 0.5f ? 0.0f : 1.0f;
    float g=1.0f-c.g; g = g < 0.5f ? 0.0f : 1.0f;
    float b=1.0f-c.b; b = b < 0.5f ? 0.0f : 1.0f;
    pOpenColor->rgb.setValue(r, g, b);
}

SoShape* ViewProviderMesh::getShapeNode() const
{
    return 0;
}

SoNode* ViewProviderMesh::getCoordNode() const
{
    return 0;
}

/** 
 * Extracts the mesh data from the feature \a pcFeature and creates
 * an Inventor node \a SoNode with these data. 
 */
void ViewProviderMesh::attach(App::DocumentObject *pcFeat)
{
    ViewProviderGeometryObject::attach(pcFeat);

    pcHighlight->objectName = pcFeat->getNameInDocument();
    pcHighlight->documentName = pcFeat->getDocument()->getName();
    pcHighlight->subElementName = "Main";

    // Note: Since for mesh data the SoFCSelection node has no SoSeparator but
    // an SoGroup as parent the EMISSIVE style if set has fundamentally no effect.
    // This behaviour is given due to the fact that SoFCSelection inherits from
    // SoGroup (formerly SoSeparator). If we wanted to enable emissive overlay for
    // highlighting or selection we would need an SoSeparator as parent node below.

    // faces
    SoGroup* pcFlatRoot = new SoGroup();
    pcFlatRoot->addChild(pShapeHints);
    pcFlatRoot->addChild(pcShapeMaterial);
    pcFlatRoot->addChild(pcMatBinding);
    pcFlatRoot->addChild(pcHighlight);
    addDisplayMaskMode(pcFlatRoot, "Shaded");

    // points
    SoGroup* pcPointRoot = new SoGroup();
    pcPointRoot->addChild(pcPointStyle);
    pcPointRoot->addChild(pShapeHints);
    pcPointRoot->addChild(pcShapeMaterial);
    pcPointRoot->addChild(pcMatBinding);
    pcPointRoot->addChild(pcHighlight);
    addDisplayMaskMode(pcPointRoot, "Point");

    // wires
    SoLightModel* pcLightModel = new SoLightModel();
    pcLightModel->model = SoLightModel::BASE_COLOR;
    SoGroup* pcWireRoot = new SoGroup();
    pcWireRoot->addChild(pcLineStyle);
    pcWireRoot->addChild(pcLightModel);
    SoMaterialBinding* binding = new SoMaterialBinding;
    binding->value = SoMaterialBinding::OVERALL; // doesn't set several colors
    pcWireRoot->addChild(binding);
    pcWireRoot->addChild(pLineColor);
    pcWireRoot->addChild(pcHighlight);
    addDisplayMaskMode(pcWireRoot, "Wireframe");

    // faces+wires
    // Avoid any Z-buffer artifacts, so that the lines always
    // appear on top of the faces
    SoPolygonOffset* offset = new SoPolygonOffset();
    offset->styles = SoPolygonOffset::FILLED;
    offset->factor = 1.0f;
    offset->units = 1.0f;

    SoSeparator* pcWireSep = new SoSeparator();
    pcWireSep->addChild(pcLineStyle);
    pcWireSep->addChild(pcLightModel);
    pcWireSep->addChild(binding);
    pcWireSep->addChild(pLineColor);
    pcWireSep->addChild(pcHighlight);

    SoGroup* pcFlatWireRoot = new SoGroup();
    pcFlatWireRoot->addChild(pcWireSep);
    pcFlatWireRoot->addChild(offset);
    pcFlatWireRoot->addChild(pShapeHints);
    pcFlatWireRoot->addChild(pcShapeMaterial);
    pcFlatWireRoot->addChild(pcMatBinding);
    pcFlatWireRoot->addChild(pcShapeGroup);
    addDisplayMaskMode(pcFlatWireRoot, "Flat Lines");

    if (getColorProperty()) {
        Coloring.setStatus(App::Property::Hidden, false);
    }
}

void ViewProviderMesh::updateData(const App::Property* prop)
{
    Gui::ViewProviderGeometryObject::updateData(prop);
    //if (prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
    //}
    if (prop->getTypeId() == App::PropertyColorList::getClassTypeId()) {
        Coloring.setStatus(App::Property::Hidden, false);
    }
}

QIcon ViewProviderMesh::getIcon() const
{
#if 1
    static QIcon icon = Gui::BitmapFactory().pixmap("Tree_Mesh");
    return icon;
#else
    static const char * const Mesh_Feature_xpm[] = {
        "16 16 4 1",
        ".	c None",
        "#	c #000000",
        "s	c #BEC2FC",
        "g	c #00FF00",
        ".......##.......",
        "....#######.....",
        "..##ggg#ggg#....",
        "##ggggg#gggg##..",
        "#g#ggg#gggggg##.",
        "#gg#gg#gggg###s.",
        "#gg#gg#gg##gg#s.",
        "#ggg#####ggg#ss.",
        "#gggg##gggg#ss..",
        ".#g##g#gggg#s...",
        ".##ggg#ggg#ss...",
        ".##gggg#g#ss....",
        "..s#####g#s.....",
        "....sss##ss.....",
        "........ss......",
        "................"};
    QPixmap px(Mesh_Feature_xpm);
    return px;
#endif
}

App::PropertyColorList* ViewProviderMesh::getColorProperty() const
{
    if (pcObject) {
        std::map<std::string,App::Property*> Map;
        pcObject->getPropertyMap(Map);
        for (std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it) {
            Base::Type type = it->second->getTypeId();
            if (type == App::PropertyColorList::getClassTypeId()) {
                App::PropertyColorList* colors = static_cast<App::PropertyColorList*>(it->second);
                return colors;
            }
        }
    }

    return 0; // no such property found
}

void ViewProviderMesh::tryColorPerVertex(bool on)
{
    if (on) {
        App::PropertyColorList* colors = getColorProperty();
        if (colors) {
            const Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
            const Mesh::MeshObject& mesh = meshProp.getValue();
            int numPoints = static_cast<int>(mesh.countPoints());

            if (colors->getSize() == numPoints) {
                setColorPerVertex(colors);
            }
        }
    }
    else {
        pcMatBinding->value = SoMaterialBinding::OVERALL;
        const App::Color& c = ShapeColor.getValue();
        pcShapeMaterial->diffuseColor.setValue(c.r,c.g,c.b);
    }
}

void ViewProviderMesh::setColorPerVertex(const App::PropertyColorList* prop)
{
    pcMatBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    const std::vector<App::Color>& val = prop->getValues();

    pcShapeMaterial->diffuseColor.setNum(val.size());
    SbColor* col = pcShapeMaterial->diffuseColor.startEditing();

    std::size_t i=0;
    for (std::vector<App::Color>::const_iterator it = val.begin(); it != val.end(); ++it) {
        col[i++].setValue(it->r, it->g, it->b);
    }

    pcShapeMaterial->diffuseColor.finishEditing();
}

void ViewProviderMesh::setDisplayMode(const char* ModeName)
{
    if (strcmp("Shaded",ModeName)==0) {
        setDisplayMaskMode("Shaded");
    }
    else if (strcmp("Points",ModeName)==0) {
        setDisplayMaskMode("Point");
    }
    else if (strcmp("Flat Lines",ModeName)==0) {
        setDisplayMaskMode("Flat Lines");
    }
    else if (strcmp("Wireframe",ModeName)==0) {
        setDisplayMaskMode("Wireframe");
    }

    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderMesh::getDisplayModes(void) const
{
    std::vector<std::string> StrList;

    // add your own modes
    StrList.push_back("Shaded");
    StrList.push_back("Wireframe");
    StrList.push_back("Flat Lines");
    StrList.push_back("Points");

    return StrList;
}

bool ViewProviderMesh::exportToVrml(const char* filename, const MeshCore::Material& mat, bool binary) const
{
    SoCoordinate3* coords = new SoCoordinate3();
    SoIndexedFaceSet* faces = new SoIndexedFaceSet();
    ViewProviderMeshBuilder builder;
    builder.createMesh(&static_cast<Mesh::Feature*>(pcObject)->Mesh, coords, faces);

    SoMaterialBinding* binding = new SoMaterialBinding;
    SoMaterial* material = new SoMaterial;

    if (static_cast<int>(mat.diffuseColor.size()) == coords->point.getNum()) {
        binding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    }
    else if (static_cast<int>(mat.diffuseColor.size()) == faces->coordIndex.getNum()/4) {
        binding->value = SoMaterialBinding::PER_FACE_INDEXED;
    }

    if (mat.diffuseColor.size() > 1) {
        material->diffuseColor.setNum(mat.diffuseColor.size());
        SbColor* colors = material->diffuseColor.startEditing();
        for (unsigned int i=0; i<mat.diffuseColor.size(); i++)
            colors[i].setValue(mat.diffuseColor[i].r,mat.diffuseColor[i].g,mat.diffuseColor[i].b);
        material->diffuseColor.finishEditing();
    }

    SoGroup* group = new SoGroup();
    group->addChild(material);
    group->addChild(binding);
    group->addChild(new SoTransform());
    group->addChild(coords);
    group->addChild(faces);

    SoToVRML2Action tovrml2;
    group->ref();
    tovrml2.apply(group);
    group->unref();
    SoVRMLGroup *vrmlRoot = tovrml2.getVRML2SceneGraph();
    vrmlRoot->ref();
    std::string buffer = Gui::SoFCDB::writeNodesToString(vrmlRoot);
    vrmlRoot->unref(); // release the memory as soon as possible

    Base::FileInfo fi(filename);
    if (binary) {
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        zipios::GZIPOutputStream gzip(str);
        if (gzip) {
            gzip << buffer;
            gzip.close();
            return true;
        }
    }
    else {
        Base::ofstream str(fi, std::ios::out);
        if (str) {
            str << buffer;
            str.close();
            return true;
        }
    }

    return false;
}

void ViewProviderMesh::exportMesh(const char* filename, const char* fmt) const
{
    MeshCore::MeshIO::Format format = MeshCore::MeshIO::Undefined;
    if (fmt) {
        std::string dummy = "meshfile.";
        dummy += fmt;
        format = MeshCore::MeshOutput::GetFormat(dummy.c_str());
    }

    MeshCore::Material mat;
    int numColors = pcShapeMaterial->diffuseColor.getNum();
    const SbColor* colors = pcShapeMaterial->diffuseColor.getValues(0);
    mat.diffuseColor.reserve(numColors);
    for (int i=0; i<numColors; i++) {
        const SbColor& c = colors[i];
        mat.diffuseColor.push_back(App::Color(c[0], c[1], c[2]));
    }

    Mesh::MeshObject mesh = static_cast<Mesh::Feature*>(getObject())->Mesh.getValue();
    mesh.setPlacement(static_cast<Mesh::Feature*>(getObject())->globalPlacement());
    if (mat.diffuseColor.size() == mesh.countPoints())
        mat.binding = MeshCore::MeshIO::PER_VERTEX;
    else if (mat.diffuseColor.size() == mesh.countFacets())
        mat.binding = MeshCore::MeshIO::PER_FACE;
    else
        mat.binding = MeshCore::MeshIO::OVERALL;

    mesh.save(filename, format, &mat, getObject()->Label.getValue());
}

void ViewProviderMesh::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);

    // toggle command to display components
    Gui::ActionFunction* func = new Gui::ActionFunction(menu);
    QAction* act = menu->addAction(QObject::tr("Display components"));
    act->setCheckable(true);
    act->setChecked(pcMatBinding->value.getValue() == SoMaterialBinding::PER_FACE);
    func->toggle(act, boost::bind(&ViewProviderMesh::setHighlightedComponents, this, _1));
}

bool ViewProviderMesh::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Transform)
        return ViewProviderGeometryObject::setEdit(ModNum);
    else if (ModNum == ViewProvider::Color)
        highlightComponents();
    return true;
}

void ViewProviderMesh::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Transform)
        ViewProviderGeometryObject::unsetEdit(ModNum);
    else if (ModNum == ViewProvider::Color)
        unhighlightSelection();
}

bool ViewProviderMesh::createToolMesh(const std::vector<SbVec2f>& rclPoly, const SbViewVolume& vol,
                                      const Base::Vector3f& rcNormal, std::vector<MeshCore::MeshGeomFacet>& aFaces)
{
    float fX, fY, fZ;
    SbVec3f pt1, pt2, pt3, pt4;
    MeshGeomFacet face;
    std::vector<Base::Vector3f> top, bottom, polygon;

    for (std::vector<SbVec2f>::const_iterator it = rclPoly.begin(); it != rclPoly.end(); ++it) {
        // the following element
        std::vector<SbVec2f>::const_iterator nt = it + 1;
        if (nt == rclPoly.end())
            nt = rclPoly.begin();
        else if (*it == *nt)
            continue; // two adjacent vertices are equal

        vol.projectPointToLine(*it, pt1, pt2);
        vol.projectPointToLine(*nt, pt3, pt4);

        // 1st facet
        pt1.getValue(fX, fY, fZ);
        face._aclPoints[0].Set(fX, fY, fZ);
        pt4.getValue(fX, fY, fZ);
        face._aclPoints[1].Set(fX, fY, fZ);
        pt3.getValue(fX, fY, fZ);
        face._aclPoints[2].Set(fX, fY, fZ);
        if (face.Area() > 0)
            aFaces.push_back(face);

        // 2nd facet
        pt1.getValue(fX, fY, fZ);
        face._aclPoints[0].Set(fX, fY, fZ);
        pt2.getValue(fX, fY, fZ);
        face._aclPoints[1].Set(fX, fY, fZ);
        pt4.getValue(fX, fY, fZ);
        face._aclPoints[2].Set(fX, fY, fZ);
        if (face.Area() > 0)
            aFaces.push_back(face);

        if (it+1 < rclPoly.end()) {
            pt1.getValue(fX, fY, fZ);
            top.push_back( Base::Vector3f(fX, fY, fZ) );
            pt2.getValue(fX, fY, fZ);
            bottom.push_back( Base::Vector3f(fX, fY, fZ) );
            // polygon we need to triangulate (in x,y-plane)
            it->getValue(fX, fY);
            polygon.push_back( Base::Vector3f(fX, fY, 0.0f) );
        }
    }

    // now create the lids
    std::vector<MeshGeomFacet> aLid;
    MeshCore::EarClippingTriangulator cTria;
    cTria.SetPolygon(polygon);
    bool ok = cTria.TriangulatePolygon();
  
    std::vector<MeshFacet> faces = cTria.GetFacets();
    for (std::vector<MeshFacet>::iterator itF = faces.begin(); itF != faces.end(); ++itF) {
        MeshGeomFacet topFacet;
        topFacet._aclPoints[0] = top[itF->_aulPoints[0]];
        topFacet._aclPoints[1] = top[itF->_aulPoints[1]];
        topFacet._aclPoints[2] = top[itF->_aulPoints[2]];
        if (topFacet.GetNormal() * rcNormal < 0) {
            std::swap(topFacet._aclPoints[1], topFacet._aclPoints[2]);
            topFacet.CalcNormal();
        }
        aFaces.push_back(topFacet);

        MeshGeomFacet botFacet;
        botFacet._aclPoints[0] = bottom[itF->_aulPoints[0]];
        botFacet._aclPoints[1] = bottom[itF->_aulPoints[1]];
        botFacet._aclPoints[2] = bottom[itF->_aulPoints[2]];
        if (botFacet.GetNormal() * rcNormal > 0) {
            std::swap(botFacet._aclPoints[1], botFacet._aclPoints[2]);
            botFacet.CalcNormal();
        }
        aFaces.push_back(botFacet);
    }

    return ok;
}

void ViewProviderMesh::showOpenEdges(bool show)
{
    (void)show;
}

namespace MeshGui {
class MeshSplit {
public:
    MeshSplit(ViewProviderMesh* mesh,
              const std::vector<SbVec2f>& poly,
              const Gui::ViewVolumeProjection& proj)
        : mesh(mesh)
        , poly(poly)
        , proj(proj)
    {

    }
    ~MeshSplit() {

    }
    void cutMesh() {
        Gui::Document* gui = mesh->getDocument();
        gui->openCommand("Cut");
        ViewProviderMesh* copy = makeCopy();
        mesh->cutMesh(poly, proj, false);
        copy->cutMesh(poly, proj, true);
        gui->commitCommand();
        delete this;
    }
    void trimMesh() {
        Gui::Document* gui = mesh->getDocument();
        gui->openCommand("Trim");
        ViewProviderMesh* copy = makeCopy();
        mesh->trimMesh(poly, proj, false);
        copy->trimMesh(poly, proj, true);
        gui->commitCommand();
        delete this;
    }
    ViewProviderMesh* makeCopy() const {
        Gui::Document* gui = mesh->getDocument();
        App::Document* doc = gui->getDocument();

        Mesh::Feature* cpy = static_cast<Mesh::Feature*>(doc->addObject("Mesh::Feature"));
        Mesh::Feature* org = static_cast<Mesh::Feature*>(mesh->getObject());
        cpy->Label.setValue(org->Label.getValue());
        cpy->Mesh.setValue(org->Mesh.getValue());

        return static_cast<ViewProviderMesh*>(gui->getViewProvider(cpy));
    }

private:
    ViewProviderMesh* mesh;
    std::vector<SbVec2f> poly;
    Gui::ViewVolumeProjection proj;
};
}

void ViewProviderMesh::clipMeshCallback(void * ud, SoEventCallback * n)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), clipMeshCallback,ud);
    n->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
    if (!views.empty()) {
        Gui::Application::Instance->activeDocument()->openCommand("Cut");
        bool commitCommand = false;
        for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
            ViewProviderMesh* self = static_cast<ViewProviderMesh*>(*it);
            if (self->getEditingMode() > -1) {
                self->finishEditing();
                SoCamera* cam = view->getSoRenderManager()->getCamera();
                SbViewVolume vv = cam->getViewVolume();
                Gui::ViewVolumeProjection proj(vv);
                proj.setTransform(static_cast<Mesh::Feature*>(self->getObject())->
                                  Placement.getValue().toMatrix());
                if (role == Gui::SelectionRole::Inner) {
                    self->cutMesh(clPoly, proj, true);
                    commitCommand = true;
                }
                else if (role == Gui::SelectionRole::Outer) {
                    self->cutMesh(clPoly, proj, false);
                    commitCommand = true;
                }
                else if (role == Gui::SelectionRole::Split) {
                    // We must delay the split because it adds a new
                    // node to the scenegraph which cannot be done while
                    // traversing it
                    Gui::TimerFunction* func = new Gui::TimerFunction();
                    func->setAutoDelete(true);
                    MeshSplit* split = new MeshSplit(self, clPoly, proj);
                    func->setFunction(boost::bind(&MeshSplit::cutMesh, split));
                    QTimer::singleShot(0, func, SLOT(timeout()));
                }
            }
        }

        if (commitCommand)
            Gui::Application::Instance->activeDocument()->commitCommand();
        else
            Gui::Application::Instance->activeDocument()->abortCommand();

        view->redraw();
    }
}

void ViewProviderMesh::trimMeshCallback(void * ud, SoEventCallback * n)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), trimMeshCallback,ud);
    n->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
    if (!views.empty()) {
        Gui::Application::Instance->activeDocument()->openCommand("Trim");
        bool commitCommand = false;
        for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
            ViewProviderMesh* self = static_cast<ViewProviderMesh*>(*it);
            if (self->getEditingMode() > -1) {
                self->finishEditing();
                SoCamera* cam = view->getSoRenderManager()->getCamera();
                SbViewVolume vv = cam->getViewVolume();
                Gui::ViewVolumeProjection proj(vv);
                proj.setTransform(static_cast<Mesh::Feature*>(self->getObject())->
                                  Placement.getValue().toMatrix());
                if (role == Gui::SelectionRole::Inner) {
                    self->trimMesh(clPoly, proj, true);
                    commitCommand = true;
                }
                else if (role == Gui::SelectionRole::Outer) {
                    self->trimMesh(clPoly, proj, false);
                    commitCommand = true;
                }
                else if (role == Gui::SelectionRole::Split) {
                    // We must delay the split because it adds a new
                    // node to the scenegraph which cannot be done while
                    // traversing it
                    Gui::TimerFunction* func = new Gui::TimerFunction();
                    func->setAutoDelete(true);
                    MeshSplit* split = new MeshSplit(self, clPoly, proj);
                    func->setFunction(boost::bind(&MeshSplit::trimMesh, split));
                    QTimer::singleShot(0, func, SLOT(timeout()));
                }
            }
        }

        if (commitCommand)
            Gui::Application::Instance->activeDocument()->commitCommand();
        else
            Gui::Application::Instance->activeDocument()->abortCommand();

        view->redraw();
    }
}

void ViewProviderMesh::partMeshCallback(void * ud, SoEventCallback * cb)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), partMeshCallback,ud);
    cb->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    // get the normal of the front clipping plane
    SbVec3f b,n;
    view->getNearPlane(b, n);
    Base::Vector3f cNormal(n[0],n[1],n[2]);
    SoCamera* pCam = view->getSoRenderManager()->getCamera();  
    SbViewVolume  vol = pCam->getViewVolume(); 

    // create a tool shape from these points
    std::vector<MeshCore::MeshGeomFacet> aFaces;
    if (!ViewProviderMesh::createToolMesh(clPoly, vol, cNormal, aFaces))
        Base::Console().Message("The picked polygon seems to have self-overlappings. This could lead to strange results.");

    MeshCore::MeshKernel toolMesh;
    bool locked = Base::Sequencer().setLocked(true);
    toolMesh = aFaces;
    Base::Sequencer().setLocked(locked);

    // Open a transaction object for the undo/redo stuff
    Gui::Application::Instance->activeDocument()->openCommand("Split");

    try {
        std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
        for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
            ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
            if (that->getEditingMode() > -1) {
                that->finishEditing();
                Base::Placement plm = static_cast<Mesh::Feature*>(that->getObject())->Placement.getValue();
                plm.invert();
                MeshCore::MeshKernel copyToolMesh(toolMesh);
                copyToolMesh.Transform(plm.toMatrix());
                if (role == Gui::SelectionRole::Inner)
                    that->splitMesh(copyToolMesh, cNormal, true);
                else
                    that->splitMesh(copyToolMesh, cNormal, false);
            }
        }
    }
    catch(...) {
        // Don't rethrow any exception
    }

    // Close the transaction
    Gui::Application::Instance->activeDocument()->commitCommand();
    view->redraw();
}

void ViewProviderMesh::segmMeshCallback(void * ud, SoEventCallback * cb)
{
    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(cb->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), segmMeshCallback,ud);
    cb->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    // get the normal of the front clipping plane
    SbVec3f b,n;
    view->getNearPlane(b, n);
    Base::Vector3f cNormal(n[0],n[1],n[2]);
    SoCamera* pCam = view->getSoRenderManager()->getCamera();  
    SbViewVolume  vol = pCam->getViewVolume(); 

    // create a tool shape from these points
    std::vector<MeshCore::MeshGeomFacet> aFaces;
    if (!ViewProviderMesh::createToolMesh(clPoly, vol, cNormal, aFaces))
        Base::Console().Message("The picked polygon seems to have self-overlappings. This could lead to strange results.");

    MeshCore::MeshKernel toolMesh;
    bool locked = Base::Sequencer().setLocked(true);
    toolMesh = aFaces;
    Base::Sequencer().setLocked(locked);

    // Open a transaction object for the undo/redo stuff
    Gui::Application::Instance->activeDocument()->openCommand("Segment");

    try {
        std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
        for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
            ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
            if (that->getEditingMode() > -1) {
                that->finishEditing();
                Base::Placement plm = static_cast<Mesh::Feature*>(that->getObject())->Placement.getValue();
                plm.invert();
                MeshCore::MeshKernel copyToolMesh(toolMesh);
                copyToolMesh.Transform(plm.toMatrix());
                if (role == Gui::SelectionRole::Inner)
                    that->segmentMesh(copyToolMesh, cNormal, true);
                else
                    that->segmentMesh(copyToolMesh, cNormal, false);
            }
        }
    }
    catch(...) {
        // Don't rethrow any exception
    }

    // Close the transaction
    Gui::Application::Instance->activeDocument()->commitCommand();
    view->redraw();
}

void ViewProviderMesh::selectGLCallback(void * ud, SoEventCallback * n)
{
    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), selectGLCallback,ud);
    n->setHandled();

    std::vector<SbVec2f> clPoly = view->getGLPolygon();
    if (clPoly.size() != 2)
        return;
    const SoEvent* ev = n->getEvent();

    SbVec2f pos = clPoly[0];
    float pX,pY; pos.getValue(pX,pY);
    const SbVec2s& sz = view->getSoRenderManager()->getViewportRegion().getViewportSizePixels();
    float fRatio = view->getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
    if (fRatio > 1.0f) {
        pX = (pX - 0.5f) / fRatio + 0.5f;
        pos.setValue(pX,pY);
    }
    else if (fRatio < 1.0f) {
        pY = (pY - 0.5f) * fRatio + 0.5f;
        pos.setValue(pX,pY);
    }

    short x1 = (short)(pX * sz[0] + 0.5f);
    short y1 = (short)(pY * sz[1] + 0.5f);
    SbVec2s loc = ev->getPosition();
    short x2 = loc[0];
    short y2 = loc[1];

    short x = (x1+x2)/2;
    short y = (y1+y2)/2;
    short w = (x2-x1);
    short h = (y2-y1);
    if (w<0) w = -w;
    if (h<0) h = -h;

    std::vector<Gui::ViewProvider*> views;
    views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
    for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
        ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
        if (that->getEditingMode() > -1) {
            that->finishEditing();
            that->selectArea(x, y, w, h, view->getSoRenderManager()->getViewportRegion(), view->getSoRenderManager()->getCamera());
        }
    }

    view->redraw();
}

void ViewProviderMesh::getFacetsFromPolygon(const std::vector<SbVec2f>& picked,
                                            const Base::ViewProjMethod& proj,
                                            SbBool inner,
                                            std::vector<unsigned long>& indices) const
{
    const bool ok = true;
    Base::Polygon2d polygon;
    for (std::vector<SbVec2f>::const_iterator it = picked.begin(); it != picked.end(); ++it)
        polygon.Add(Base::Vector2d((*it)[0],(*it)[1]));

    // Get the attached mesh property
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    MeshCore::MeshAlgorithm cAlg(meshProp.getValue().getKernel());
    cAlg.CheckFacets(&proj, polygon, true, indices);

    if (!inner) {
        // get the indices that are completely outside
        std::vector<unsigned long> complete(meshProp.getValue().countFacets());
        std::generate(complete.begin(), complete.end(), Base::iotaGen<unsigned long>(0));
        std::sort(indices.begin(), indices.end());
        std::vector<unsigned long> complementary;
        std::back_insert_iterator<std::vector<unsigned long> > biit(complementary);
        std::set_difference(complete.begin(), complete.end(), indices.begin(), indices.end(), biit);
        indices = complementary;
    }

    if (!ok) // note: the mouse grabbing needs to be released
        Base::Console().Message("The picked polygon seems to have self-overlappings. This could lead to strange results.");
}

std::vector<unsigned long> ViewProviderMesh::getFacetsOfRegion(const SbViewportRegion& select,
                                                               const SbViewportRegion& region,
                                                               SoCamera* camera) const
{
    SoSeparator* root = new SoSeparator();
    root->ref();
    root->addChild(camera);
    root->addChild(const_cast<ViewProviderMesh*>(this)->getCoordNode());
    root->addChild(const_cast<ViewProviderMesh*>(this)->getShapeNode());
    Gui::SoGLSelectAction gl(region, select);
    gl.apply(root);
    root->unref();

    std::vector<unsigned long> faces;
    faces.insert(faces.end(), gl.indices.begin(), gl.indices.end());
    return faces;
}

void ViewProviderMesh::panCamera(SoCamera * cam, float aspectratio, const SbPlane & panplane,
                                 const SbVec2f & currpos, const SbVec2f & prevpos)
{
    if (cam == NULL) return; // can happen for empty scenegraph
    if (currpos == prevpos) return; // useless invocation


    // Find projection points for the last and current mouse coordinates.
    SbViewVolume vv = cam->getViewVolume(aspectratio);
    SbLine line;
    vv.projectPointToLine(currpos, line);
    SbVec3f current_planept;
    panplane.intersect(line, current_planept);
    vv.projectPointToLine(prevpos, line);
    SbVec3f old_planept;
    panplane.intersect(line, old_planept);

    // Reposition camera according to the vector difference between the
    // projected points.
    cam->position = cam->position.getValue() - (current_planept - old_planept);
}

void ViewProviderMesh::boxZoom(const SbBox2s& box, const SbViewportRegion & vp, SoCamera* cam)
{
    SbViewVolume vv = cam->getViewVolume(vp.getViewportAspectRatio());

    short sizeX,sizeY;
    box.getSize(sizeX, sizeY);
    SbVec2s size = vp.getViewportSizePixels();

    // The bbox must not be empty i.e. width and length is zero, but it is possible that
    // either width or length is zero
    if (sizeX == 0 && sizeY == 0) 
        return;

    // Get the new center in normalized pixel coordinates
    short xmin,xmax,ymin,ymax;
    box.getBounds(xmin,ymin,xmax,ymax);
    const SbVec2f center((float) ((xmin+xmax)/2) / (float) std::max((int)(size[0] - 1), 1),
                         (float) (size[1]-(ymin+ymax)/2) / (float) std::max((int)(size[1] - 1), 1));

    SbPlane plane = vv.getPlane(cam->focalDistance.getValue());
    panCamera(cam,vp.getViewportAspectRatio(),plane, SbVec2f(0.5,0.5), center);

    // Set height or height angle of the camera
    float scaleX = (float)sizeX/(float)size[0];
    float scaleY = (float)sizeY/(float)size[1];
    float scale = std::max<float>(scaleX, scaleY);
    if (cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        float height = static_cast<SoOrthographicCamera*>(cam)->height.getValue() * scale;
        static_cast<SoOrthographicCamera*>(cam)->height = height;
    }
    else if (cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        float height = static_cast<SoPerspectiveCamera*>(cam)->heightAngle.getValue() / 2.0f;
        height = 2.0f * atan(tan(height) * scale);
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = height;
    }
}

std::vector<unsigned long> ViewProviderMesh::getVisibleFacetsAfterZoom(const SbBox2s& rect,
                                                                       const SbViewportRegion& vp,
                                                                       SoCamera* camera) const
{
    // camera copy will be deleted inside getVisibleFacets()
    // because the ref counter reaches 0
    camera = static_cast<SoCamera*>(camera->copy());
    boxZoom(rect,vp,camera);
    return getVisibleFacets(vp, camera);
}

void ViewProviderMesh::renderGLCallback(void * ud, SoAction * action)
{
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        ViewProviderMesh* mesh = reinterpret_cast<ViewProviderMesh*>(ud);
        Gui::SoVisibleFaceAction fa;
        fa.apply(mesh->getRoot());
    }
}

std::vector<unsigned long> ViewProviderMesh::getVisibleFacets(const SbViewportRegion& vp,
                                                              SoCamera* camera) const
{
    const Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    const Mesh::MeshObject& mesh = meshProp.getValue();
    uint32_t count = (uint32_t)mesh.countFacets();

    SoSeparator* root = new SoSeparator;
    root->ref();
    root->addChild(camera);

#if 0
    SoCallback* cb = new SoCallback;
    cb->setCallback(renderGLCallback, const_cast<ViewProviderMesh*>(this));
    root->addChild(cb);
#else
    SoLightModel* lm = new SoLightModel();
    lm->model = SoLightModel::BASE_COLOR;
    root->addChild(lm);
    SoMaterial* mat = new SoMaterial();
    mat->diffuseColor.setNum(count);
    SbColor* diffcol = mat->diffuseColor.startEditing();
    for (uint32_t i=0; i<count; i++) {
        float t;
        diffcol[i].setPackedValue(i<<8,t);
    }

    mat->diffuseColor.finishEditing();

    // backface culling
    //SoShapeHints* hints = new SoShapeHints;
    //hints->shapeType = SoShapeHints::SOLID;
    //hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    SoMaterialBinding* bind = new SoMaterialBinding();
    bind->value = SoMaterialBinding::PER_FACE;
    //root->addChild(hints);
    root->addChild(mat);
    root->addChild(bind);
#endif
    root->addChild(this->getCoordNode());
    root->addChild(this->getShapeNode());

    Gui::SoFCOffscreenRenderer& renderer = Gui::SoFCOffscreenRenderer::instance();
    renderer.setViewportRegion(vp);
    renderer.setBackgroundColor(SbColor(0.0f, 0.0f, 0.0f));

    QImage img;
    renderer.render(root);
    renderer.writeToImage(img);
    root->unref();

    int width = img.width();
    int height = img.height();
    QRgb color=0;
    std::vector<unsigned long> faces;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            QRgb rgb = img.pixel(x,y);
            rgb = rgb-(0xff << 24);
            if (rgb != 0 && rgb != color) {
                color = rgb;
                faces.push_back((unsigned long)rgb);
            }
        }
    }

    std::sort(faces.begin(), faces.end());
    faces.erase(std::unique(faces.begin(), faces.end()), faces.end());

    return faces;
}

void ViewProviderMesh::cutMesh(const std::vector<SbVec2f>& picked, 
                               const Base::ViewProjMethod& proj, SbBool inner)
{
    // Get the facet indices inside the tool mesh
    std::vector<unsigned long> indices;
    getFacetsFromPolygon(picked, proj, inner, indices);
    removeFacets(indices);
}

void ViewProviderMesh::trimMesh(const std::vector<SbVec2f>& polygon, 
                                const Base::ViewProjMethod& proj, SbBool inner)
{
    Mesh::MeshObject* mesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.startEditing();

    Base::Polygon2d polygon2d;
    for (std::vector<SbVec2f>::const_iterator it = polygon.begin(); it != polygon.end(); ++it)
        polygon2d.Add(Base::Vector2d((*it)[0],(*it)[1]));

    Mesh::MeshObject::CutType type = inner ?
        Mesh::MeshObject::INNER :
        Mesh::MeshObject::OUTER;
    mesh->trim(polygon2d, proj, type);
    static_cast<Mesh::Feature*>(pcObject)->Mesh.finishEditing();
    pcObject->purgeTouched();
}

void ViewProviderMesh::splitMesh(const MeshCore::MeshKernel& toolMesh, const Base::Vector3f& normal, SbBool clip_inner)
{
    // Get the attached mesh property
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    const MeshCore::MeshKernel& meshPropKernel = meshProp.getValue().getKernel();

    // Get the facet indices inside the tool mesh
    std::vector<unsigned long> indices;
    MeshCore::MeshFacetGrid cGrid(meshPropKernel);
    MeshCore::MeshAlgorithm cAlg(meshPropKernel);
    cAlg.GetFacetsFromToolMesh(toolMesh, normal, cGrid, indices);
    if (!clip_inner) {
        // get the indices that are completely outside
        std::vector<unsigned long> complete(meshPropKernel.CountFacets());
        std::generate(complete.begin(), complete.end(), Base::iotaGen<unsigned long>(0));
        std::sort(indices.begin(), indices.end());
        std::vector<unsigned long> complementary;
        std::back_insert_iterator<std::vector<unsigned long> > biit(complementary);
        std::set_difference(complete.begin(), complete.end(), indices.begin(), indices.end(), biit);
        indices = complementary;
    }

    // Remove the facets from the mesh and create a new one
    Mesh::MeshObject* kernel = meshProp.getValue().meshFromSegment(indices);
    removeFacets(indices);
    Mesh::Feature* splitMesh = static_cast<Mesh::Feature*>(App::GetApplication().getActiveDocument()
        ->addObject("Mesh::Feature",pcObject->getNameInDocument()));
    // Note: deletes also kernel
    splitMesh->Mesh.setValuePtr(kernel);
    static_cast<Mesh::Feature*>(pcObject)->purgeTouched();
}

void ViewProviderMesh::segmentMesh(const MeshCore::MeshKernel& toolMesh, const Base::Vector3f& normal, SbBool clip_inner)
{
    // Get the attached mesh property
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    const MeshCore::MeshKernel& meshPropKernel = meshProp.getValue().getKernel();

    // Get the facet indices inside the tool mesh
    std::vector<unsigned long> indices;
    MeshCore::MeshFacetGrid cGrid(meshPropKernel);
    MeshCore::MeshAlgorithm cAlg(meshPropKernel);
    cAlg.GetFacetsFromToolMesh(toolMesh, normal, cGrid, indices);
    if (!clip_inner) {
        // get the indices that are completely outside
        std::vector<unsigned long> complete(meshPropKernel.CountFacets());
        std::generate(complete.begin(), complete.end(), Base::iotaGen<unsigned long>(0));
        std::sort(indices.begin(), indices.end());
        std::vector<unsigned long> complementary;
        std::back_insert_iterator<std::vector<unsigned long> > biit(complementary);
        std::set_difference(complete.begin(), complete.end(), indices.begin(), indices.end(), biit);
        indices = complementary;
    }

    Mesh::MeshObject* kernel = meshProp.startEditing();
    kernel->addSegment(indices);
    meshProp.finishEditing();
    static_cast<Mesh::Feature*>(pcObject)->purgeTouched();
}

void ViewProviderMesh::faceInfoCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent *)n->getEvent();
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
        n->setHandled();
        // context-menu
        QMenu menu;
        QAction* cl = menu.addAction(QObject::tr("Leave info mode"));
        QAction* id = menu.exec(QCursor::pos());
        if (cl == id) {
            view->setEditing(false);
            view->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), faceInfoCallback,ud);
            std::list<Gui::GLGraphicsItem*> glItems = view->getGraphicsItemsOfType(Gui::GLFlagWindow::getClassTypeId());
            for (std::list<Gui::GLGraphicsItem*>::iterator it = glItems.begin(); it != glItems.end(); ++it) {
                view->removeGraphicsItem(*it);
                delete *it;
            }

            // See comment below
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
            hGrp->SetBool("ShowNaviCube", hGrp->GetBool("ShowNaviCube", true));
        }
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
        const SoPickedPoint * point = n->getPickedPoint();
        if (point == NULL) {
            Base::Console().Message("No facet picked.\n");
            return;
        }

        n->setHandled();

        // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
        // really from the mesh we render and not from any other geometry
        Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
        if (!vp || !vp->getTypeId().isDerivedFrom(ViewProviderMesh::getClassTypeId()))
            return;

        // FIXME: The Flag class doesn't work well (flickering) when the NaviCube is enabled.
        // To avoid this the NaviCube is disabled for the time the flags are shown.
        // When leaving this mode the NaviCube can be displayed again.
        // For a proper solution it's best to move the Flag class to the QGraphicsView API.
        view->setEnabledNaviCube(false);

        ViewProviderMesh* that = static_cast<ViewProviderMesh*>(vp);
        const SoDetail* detail = point->getDetail(that->getShapeNode());
        if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
            // get the boundary to the picked facet
            const SoFaceDetail* faceDetail = static_cast<const SoFaceDetail*>(detail);
            unsigned long uFacet = faceDetail->getFaceIndex();
            that->faceInfo(uFacet);
            Gui::GLFlagWindow* flags = 0;
            std::list<Gui::GLGraphicsItem*> glItems = view->getGraphicsItemsOfType(Gui::GLFlagWindow::getClassTypeId());
            if (glItems.empty()) {
                flags = new Gui::GLFlagWindow(view);
                view->addGraphicsItem(flags);
            }
            else {
                flags = static_cast<Gui::GLFlagWindow*>(glItems.front());
            }

            int point1 = static_cast<const SoPointDetail*>(faceDetail->getPoint(0))->getCoordinateIndex();
            int point2 = static_cast<const SoPointDetail*>(faceDetail->getPoint(1))->getCoordinateIndex();
            int point3 = static_cast<const SoPointDetail*>(faceDetail->getPoint(2))->getCoordinateIndex();
            Gui::Flag* flag = new Gui::Flag;
            flag->setText(QObject::tr("Index: %1").arg(uFacet));
            QString toolTip = QString::fromLatin1("Facet index: %1\n"
                                                  "Points: <%2, %3, %4>")
                    .arg(uFacet)
                    .arg(point1).arg(point2).arg(point3);
            flag->setToolTip(toolTip);
            flag->setOrigin(point->getPoint());
            flags->addFlag(flag, Gui::FlagLayout::TopRight);
        }
    }
}

void ViewProviderMesh::fillHoleCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent *)n->getEvent();
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
        n->setHandled();
        // context-menu
        QMenu menu;
        QAction* cl = menu.addAction(QObject::tr("Leave hole-filling mode"));
        QAction* id = menu.exec(QCursor::pos());
        if (cl == id) {
            view->setEditing(false);
            view->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), fillHoleCallback,ud);
        }
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
        const SoPickedPoint * point = n->getPickedPoint();
        if (point == NULL) {
            Base::Console().Message("No facet picked.\n");
            return;
        }

        n->setHandled();

        // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
        // really from the mesh we render and not from any other geometry
        Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
        if (!vp || !vp->getTypeId().isDerivedFrom(ViewProviderMesh::getClassTypeId()))
            return;
        ViewProviderMesh* that = static_cast<ViewProviderMesh*>(vp);
        const SoDetail* detail = point->getDetail(that->getShapeNode());
        if ( detail && detail->getTypeId() == SoFaceDetail::getClassTypeId() ) {
            // get the boundary to the picked facet
            unsigned long uFacet = ((SoFaceDetail*)detail)->getFaceIndex();
            that->fillHole(uFacet);
        }
    }
}

void ViewProviderMesh::markPartCallback(void * ud, SoEventCallback * n)
{
    // handle only mouse button events
    if (n->getEvent()->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
        Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

        // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
        n->getAction()->setHandled();
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            // context-menu
            QMenu menu;
            QAction* cl = menu.addAction(QObject::tr("Leave removal mode"));
            QAction* rm = menu.addAction(QObject::tr("Delete selected faces"));
            QAction* cf = menu.addAction(QObject::tr("Clear selected faces"));
            QAction* id = menu.exec(QCursor::pos());
            if (cl == id) {
                view->setEditing(false);
                view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), markPartCallback,ud);

                std::vector<ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
                for (std::vector<ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
                    static_cast<ViewProviderMesh*>(*it)->clearSelection();
                }
            }
            else if (cf == id) {
                std::vector<ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
                for (std::vector<ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
                    static_cast<ViewProviderMesh*>(*it)->clearSelection();
                }
            }
            else if (rm == id) {
                Gui::Application::Instance->activeDocument()->openCommand("Delete");
                std::vector<ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
                for (std::vector<ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
                    static_cast<ViewProviderMesh*>(*it)->deleteSelection();
                }
                view->redraw();
                Gui::Application::Instance->activeDocument()->commitCommand();
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point == NULL) {
                Base::Console().Message("No facet picked.\n");
                return;
            }

            n->setHandled();

            // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
            // really from the mesh we render and not from any other geometry
            Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
            if (!vp || !vp->getTypeId().isDerivedFrom(ViewProviderMesh::getClassTypeId()))
                return;
            ViewProviderMesh* that = static_cast<ViewProviderMesh*>(vp);
            const SoDetail* detail = point->getDetail(that->getShapeNode());
            if ( detail && detail->getTypeId() == SoFaceDetail::getClassTypeId() ) {
                // get the boundary to the picked facet
                unsigned long uFacet = static_cast<const SoFaceDetail*>(detail)->getFaceIndex();
                that->selectComponent(uFacet);
            }
        }
    }
}

void ViewProviderMesh::faceInfo(unsigned long uFacet)
{
    Mesh::Feature* fea = static_cast<Mesh::Feature*>(this->getObject());
    const MeshCore::MeshKernel& rKernel = fea->Mesh.getValue().getKernel();
    const MeshCore::MeshFacetArray& facets = rKernel.GetFacets();
    if (uFacet < facets.size()) {
        MeshCore::MeshFacet face = facets[uFacet];
        MeshCore::MeshGeomFacet tria = rKernel.GetFacet(face);
        Base::Console().Message("Mesh: %s Facet %lu: Points: <%lu, %lu, %lu>, Neighbours: <%lu, %lu, %lu>\n"
            "Triangle: <[%.6f, %.6f, %.6f], [%.6f, %.6f, %.6f], [%.6f, %.6f, %.6f]>\n", fea->getNameInDocument(), uFacet, 
            face._aulPoints[0], face._aulPoints[1], face._aulPoints[2], 
            face._aulNeighbours[0], face._aulNeighbours[1], face._aulNeighbours[2],
            tria._aclPoints[0].x, tria._aclPoints[0].y, tria._aclPoints[0].z,
            tria._aclPoints[1].x, tria._aclPoints[1].y, tria._aclPoints[1].z,
            tria._aclPoints[2].x, tria._aclPoints[2].y, tria._aclPoints[2].z);
    }
}

void ViewProviderMesh::fillHole(unsigned long uFacet)
{
    // get parameter from user settings
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
    int level = (int)hGrp->GetInt("FillHoleLevel", 2);

    // get the boundary to the picked facet
    std::list<unsigned long> aBorder;
    Mesh::Feature* fea = reinterpret_cast<Mesh::Feature*>(this->getObject());
    const MeshCore::MeshKernel& rKernel = fea->Mesh.getValue().getKernel();
    MeshCore::MeshRefPointToFacets cPt2Fac(rKernel);
    MeshCore::MeshAlgorithm meshAlg(rKernel);
    meshAlg.GetMeshBorder(uFacet, aBorder);
    std::vector<unsigned long> boundary(aBorder.begin(), aBorder.end());
    std::list<std::vector<unsigned long> > boundaries;
    boundaries.push_back(boundary);
    meshAlg.SplitBoundaryLoops(boundaries);

    std::vector<MeshCore::MeshFacet> newFacets;
    std::vector<Base::Vector3f> newPoints;
    unsigned long numberOfOldPoints = rKernel.CountPoints();
    for (std::list<std::vector<unsigned long> >::iterator it = boundaries.begin(); it != boundaries.end(); ++it) {
        if (it->size() < 3/* || it->size() > 200*/)
            continue;
        boundary = *it;
        MeshCore::MeshFacetArray faces;
        MeshCore::MeshPointArray points;
        MeshCore::QuasiDelaunayTriangulator cTria/*(0.05f)*/;
        cTria.SetVerifier(new MeshCore::TriangulationVerifierV2);
        if (meshAlg.FillupHole(boundary, cTria, faces, points, level, &cPt2Fac)) {
            if (boundary.front() == boundary.back())
                boundary.pop_back();
            // the triangulation may produce additional points which we must take into account when appending to the mesh
            unsigned long countBoundaryPoints = boundary.size();
            unsigned long countDifference = points.size() - countBoundaryPoints;
            if (countDifference > 0) {
                MeshCore::MeshPointArray::_TIterator pt = points.begin() + countBoundaryPoints;
                for (unsigned long i=0; i<countDifference; i++, pt++) {
                    boundary.push_back(numberOfOldPoints++);
                    newPoints.push_back(*pt);
                 }
            }
            for (MeshCore::MeshFacetArray::_TIterator kt = faces.begin(); kt != faces.end(); ++kt ) {
                kt->_aulPoints[0] = boundary[kt->_aulPoints[0]];
                kt->_aulPoints[1] = boundary[kt->_aulPoints[1]];
                kt->_aulPoints[2] = boundary[kt->_aulPoints[2]];
                newFacets.push_back(*kt);
            }
        }
    }

    if (newFacets.empty())
        return; // nothing to do

    //add the facets to the mesh and open a transaction object for the undo/redo stuff
    Gui::Application::Instance->activeDocument()->openCommand("Fill hole");
    Mesh::MeshObject* kernel = fea->Mesh.startEditing();
    kernel->addFacets(newFacets, newPoints, true);
    fea->Mesh.finishEditing();
    Gui::Application::Instance->activeDocument()->commitCommand();
}

void ViewProviderMesh::setFacetTransparency(const std::vector<float>& facetTransparency)
{
    App::Color c = ShapeColor.getValue();
    pcShapeMaterial->diffuseColor.setNum(facetTransparency.size());
    SbColor* cols = pcShapeMaterial->diffuseColor.startEditing();
    for (std::size_t index = 0; index < facetTransparency.size(); ++index)
        cols[index].setValue(c.r, c.g, c.b);
    pcShapeMaterial->diffuseColor.finishEditing();

    pcShapeMaterial->transparency.setNum(facetTransparency.size());
    float* tran = pcShapeMaterial->transparency.startEditing();
    for (std::size_t index = 0; index < facetTransparency.size(); ++index)
        tran[index] = facetTransparency[index];

    pcShapeMaterial->transparency.finishEditing();
    pcMatBinding->value = SoMaterialBinding::PER_FACE;
}

void ViewProviderMesh::resetFacetTransparency()
{
    pcMatBinding->value = SoMaterialBinding::OVERALL;
    App::Color c = ShapeColor.getValue();
    pcShapeMaterial->diffuseColor.setValue(c.r, c.g, c.b);
    pcShapeMaterial->transparency.setValue(0);
}

void ViewProviderMesh::removeFacets(const std::vector<unsigned long>& facets)
{
    // Get the attached mesh property
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    Mesh::MeshObject* kernel = meshProp.startEditing();

    // get the colour property if there
    App::PropertyColorList* prop = getColorProperty();
    if (prop && prop->getSize() == static_cast<int>(kernel->countPoints())) {
        std::vector<unsigned long> pointDegree;
        unsigned long invalid = kernel->getPointDegree(facets, pointDegree);
        if (invalid > 0) {
            // switch off coloring mode
            Coloring.setValue(false);

            const std::vector<App::Color>& colors = prop->getValues();
            std::vector<App::Color> valid_colors;
            valid_colors.reserve(kernel->countPoints() - invalid);
            std::size_t numPoints = pointDegree.size();
            for (std::size_t index = 0; index < numPoints; index++) {
                if (pointDegree[index] > 0) {
                    valid_colors.push_back(colors[index]);
                }
            }

            prop->setValues(valid_colors);
        }
    }

    //Remove the facets from the mesh and open a transaction object for the undo/redo stuff
    kernel->deleteFacets(facets);
    meshProp.finishEditing();
    pcObject->purgeTouched();
}

void ViewProviderMesh::selectFacet(unsigned long facet)
{
    std::vector<unsigned long> selection;
    selection.push_back(facet);

    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.addFacetsToSelection(selection);

    // Colorize the selection
    pcMatBinding->value = SoMaterialBinding::PER_FACE;
    int uCtFacets = (int)rMesh.countFacets();

    if (uCtFacets != pcShapeMaterial->diffuseColor.getNum()) {
        highlightSelection();
    }
    else {
        pcShapeMaterial->diffuseColor.set1Value(facet,1.0f,0.0f,0.0f);
    }
}

void ViewProviderMesh::deselectFacet(unsigned long facet)
{
    std::vector<unsigned long> selection;
    selection.push_back(facet);

    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.removeFacetsFromSelection(selection);

    // Colorize the selection
    pcMatBinding->value = SoMaterialBinding::PER_FACE;
    int uCtFacets = (int)rMesh.countFacets();

    if (rMesh.hasSelectedFacets()) {
        if (uCtFacets != pcShapeMaterial->diffuseColor.getNum()) {
            highlightSelection();
        }
        else {
            App::Color c = ShapeColor.getValue();
            pcShapeMaterial->diffuseColor.set1Value(facet,c.r,c.g,c.b);
        }
    }
    else {
        unhighlightSelection();
    }
}

bool ViewProviderMesh::isFacetSelected(unsigned long facet)
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    const MeshCore::MeshFacetArray& faces = rMesh.getKernel().GetFacets();
    return faces[facet].IsFlag(MeshCore::MeshFacet::SELECTED);
}

void ViewProviderMesh::selectComponent(unsigned long uFacet)
{
    std::vector<unsigned long> selection;
    selection.push_back(uFacet);

    MeshCore::MeshTopFacetVisitor clVisitor(selection);
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    const MeshCore::MeshKernel& rKernel = rMesh.getKernel();
    MeshCore::MeshAlgorithm(rKernel).ResetFacetFlag(MeshCore::MeshFacet::VISIT);
    rKernel.VisitNeighbourFacets(clVisitor, uFacet);
    rMesh.addFacetsToSelection(selection);

    // Colorize the selection
    highlightSelection();
}

void ViewProviderMesh::deselectComponent(unsigned long uFacet)
{
    std::vector<unsigned long> selection;
    selection.push_back(uFacet);

    MeshCore::MeshTopFacetVisitor clVisitor(selection);
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    const MeshCore::MeshKernel& rKernel = rMesh.getKernel();
    MeshCore::MeshAlgorithm(rKernel).ResetFacetFlag(MeshCore::MeshFacet::VISIT);
    rKernel.VisitNeighbourFacets(clVisitor, uFacet);
    rMesh.removeFacetsFromSelection(selection);

    // Colorize the selection
    if (rMesh.hasSelectedFacets())
        highlightSelection();
    else
        unhighlightSelection();
}

void ViewProviderMesh::setSelection(const std::vector<unsigned long>& indices)
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.clearFacetSelection();
    rMesh.addFacetsToSelection(indices);

    // Colorize the selection
    if (indices.empty())
        unhighlightSelection();
    else
        highlightSelection();
}

void ViewProviderMesh::addSelection(const std::vector<unsigned long>& indices)
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.addFacetsToSelection(indices);

    // Colorize the selection
    highlightSelection();
}

void ViewProviderMesh::removeSelection(const std::vector<unsigned long>& indices)
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.removeFacetsFromSelection(indices);

    // Colorize the selection
    if (rMesh.hasSelectedFacets())
        highlightSelection();
    else
        unhighlightSelection();
}

void ViewProviderMesh::invertSelection()
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    const MeshCore::MeshFacetArray& faces = rMesh.getKernel().GetFacets();
    unsigned long num_notsel = std::count_if(faces.begin(), faces.end(),
        std::bind2nd(MeshCore::MeshIsNotFlag<MeshCore::MeshFacet>(),
        MeshCore::MeshFacet::SELECTED));
    std::vector<unsigned long> notselect;
    notselect.reserve(num_notsel);
    MeshCore::MeshFacetArray::_TConstIterator beg = faces.begin();
    MeshCore::MeshFacetArray::_TConstIterator end = faces.end();
    for (MeshCore::MeshFacetArray::_TConstIterator jt = beg; jt != end; ++jt) {
        if (!jt->IsFlag(MeshCore::MeshFacet::SELECTED))
            notselect.push_back(jt-beg);
    }
    setSelection(notselect);
}

void ViewProviderMesh::clearSelection()
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.clearFacetSelection();
    unhighlightSelection();
}

void ViewProviderMesh::deleteSelection()
{
    std::vector<unsigned long> indices;
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    const Mesh::MeshObject& rMesh = meshProp.getValue();
    rMesh.getFacetsFromSelection(indices);
    if (!indices.empty()) {
        rMesh.clearFacetSelection();
        unhighlightSelection();
        removeFacets(indices);
    }
}

bool ViewProviderMesh::hasSelection() const
{
    std::vector<unsigned long> indices;
    Mesh::PropertyMeshKernel& meshProp = static_cast<Mesh::Feature*>(pcObject)->Mesh;
    const Mesh::MeshObject& rMesh = meshProp.getValue();
    return rMesh.hasSelectedFacets();
}

void ViewProviderMesh::selectArea(short x, short y, short w, short h,
                                  const SbViewportRegion& region,
                                  SoCamera* camera)
{
    SbViewportRegion vp;
    vp.setViewportPixels (x, y, w, h);
    std::vector<unsigned long> faces = getFacetsOfRegion(vp, region, camera);

    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.addFacetsToSelection(faces);

    // Colorize the selected part
    highlightSelection();
}

void ViewProviderMesh::highlightSelection()
{
    std::vector<unsigned long> selection;
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    rMesh.getFacetsFromSelection(selection);
    if (selection.empty()) {
        // If no faces are selected then simply return even
        // without calling unhighlightSelection()
        return;
    }

    // Colorize the selection
    pcMatBinding->value = SoMaterialBinding::PER_FACE;
    App::Color c = ShapeColor.getValue();
    int uCtFacets = (int)rMesh.countFacets();
    pcShapeMaterial->diffuseColor.setNum(uCtFacets);

    SbColor* cols = pcShapeMaterial->diffuseColor.startEditing();
    for (int i=0; i<uCtFacets; i++)
        cols[i].setValue(c.r,c.g,c.b);
    for (std::vector<unsigned long>::iterator it = selection.begin(); it != selection.end(); ++it)
        cols[*it].setValue(1.0f,0.0f,0.0f);
    pcShapeMaterial->diffuseColor.finishEditing();
}

void ViewProviderMesh::unhighlightSelection()
{
    App::Color c = ShapeColor.getValue();
    pcMatBinding->value = SoMaterialBinding::OVERALL;
    pcShapeMaterial->diffuseColor.setNum(1);
    pcShapeMaterial->diffuseColor.setValue(c.r,c.g,c.b);
}

void ViewProviderMesh::setHighlightedComponents(bool on)
{
    if (on) {
        highlightComponents();
    }
    else {
        unhighlightSelection();
    }
}

void ViewProviderMesh::highlightComponents()
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    std::vector<std::vector<unsigned long> > comps = rMesh.getComponents();

    // Colorize the components
    pcMatBinding->value = SoMaterialBinding::PER_FACE;
    int uCtFacets = (int)rMesh.countFacets();
    pcShapeMaterial->diffuseColor.setNum(uCtFacets);

    SbColor* cols = pcShapeMaterial->diffuseColor.startEditing();
    for (std::vector<std::vector<unsigned long> >::iterator it = comps.begin(); it != comps.end(); ++it) {
        float fMax = (float)RAND_MAX;
        float fRed = (float)rand()/fMax;
        float fGrn = (float)rand()/fMax;
        float fBlu = (float)rand()/fMax;
        for (std::vector<unsigned long>::iterator jt = it->begin(); jt != it->end(); ++jt) {
            cols[*jt].setValue(fRed,fGrn,fBlu);
        }
    }
    pcShapeMaterial->diffuseColor.finishEditing();
}

void ViewProviderMesh::highlightSegments(const std::vector<App::Color>& colors)
{
    const Mesh::MeshObject& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue();
    unsigned long numSegm = rMesh.countSegments();
    if (numSegm == colors.size()) {
        // Colorize the components
        pcMatBinding->value = SoMaterialBinding::PER_FACE;
        int uCtFacets = (int)rMesh.countFacets();

        pcShapeMaterial->diffuseColor.setNum(uCtFacets);
        SbColor* cols = pcShapeMaterial->diffuseColor.startEditing();
        for (unsigned long i=0; i<numSegm; i++) {
            std::vector<unsigned long> segm = rMesh.getSegment(i).getIndices();
            float fRed = colors[i].r;
            float fGrn = colors[i].g;
            float fBlu = colors[i].b;
            for (std::vector<unsigned long>::iterator it = segm.begin(); it != segm.end(); ++it) {
                cols[*it].setValue(fRed,fGrn,fBlu);
            }
        }
        pcShapeMaterial->diffuseColor.finishEditing();
    }
    else if (colors.size() == 1) {
        pcMatBinding->value = SoMaterialBinding::OVERALL;
        float fRed = colors[0].r;
        float fGrn = colors[0].g;
        float fBlu = colors[0].b;
        pcShapeMaterial->diffuseColor.setValue(fRed,fGrn,fBlu);
    }
}

PyObject* ViewProviderMesh::getPyObject()
{
    if (!pyViewObject)
        pyViewObject = new ViewProviderMeshPy(this);
    pyViewObject->IncRef();
    return pyViewObject;
}

// ------------------------------------------------------

PROPERTY_SOURCE(MeshGui::ViewProviderIndexedFaceSet, MeshGui::ViewProviderMesh)

ViewProviderIndexedFaceSet::ViewProviderIndexedFaceSet()
{
    pcMeshCoord = 0;
    pcMeshFaces = 0;
}

ViewProviderIndexedFaceSet::~ViewProviderIndexedFaceSet()
{
}

/** 
 * Extracts the mesh data from the feature \a pcFeature and creates
 * an Inventor node \a SoNode with these data. 
 */
void ViewProviderIndexedFaceSet::attach(App::DocumentObject *pcFeat)
{
    ViewProviderMesh::attach(pcFeat);

    pcMeshCoord = new SoCoordinate3;
    pcHighlight->addChild(pcMeshCoord);

    pcMeshFaces = new SoFCIndexedFaceSet;
    pcHighlight->addChild(pcMeshFaces);

    // read the threshold from the preferences
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
    int size = hGrp->GetInt("RenderTriangleLimit", -1);
    if (size > 0) static_cast<SoFCIndexedFaceSet*>(pcMeshFaces)->renderTriangleLimit = (unsigned int)(pow(10.0f,size));
}

void ViewProviderIndexedFaceSet::updateData(const App::Property* prop)
{
    ViewProviderMesh::updateData(prop);
    if (prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        ViewProviderMeshBuilder builder;
        builder.createMesh(prop, pcMeshCoord, pcMeshFaces);
        showOpenEdges(OpenEdges.getValue());
        highlightSelection();
    }
}

void ViewProviderIndexedFaceSet::showOpenEdges(bool show)
{
    if (pcOpenEdge) {
        // remove the node and destroy the data
        pcRoot->removeChild(pcOpenEdge);
        pcOpenEdge = 0;
    }

    if (show) {
        pcOpenEdge = new SoSeparator();
        pcOpenEdge->addChild(pcLineStyle);
        pcOpenEdge->addChild(pOpenColor);

        pcOpenEdge->addChild(pcMeshCoord);
        SoIndexedLineSet* lines = new SoIndexedLineSet;
        pcOpenEdge->addChild(lines);

        // add to the highlight node
        pcRoot->addChild(pcOpenEdge);

        // Build up the lines with indices to the list of vertices 'pcMeshCoord'
        int index=0;
        const MeshCore::MeshKernel& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue().getKernel();
        const MeshCore::MeshFacetArray& rFaces = rMesh.GetFacets();
        for (MeshCore::MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it) {
            for (int i=0; i<3; i++) {
                if (it->_aulNeighbours[i] == ULONG_MAX) {
                    lines->coordIndex.set1Value(index++,it->_aulPoints[i]);
                    lines->coordIndex.set1Value(index++,it->_aulPoints[(i+1)%3]);
                    lines->coordIndex.set1Value(index++,SO_END_LINE_INDEX);
                }
            }
        }
    }
}

SoShape* ViewProviderIndexedFaceSet::getShapeNode() const
{
    return this->pcMeshFaces;
}

SoNode* ViewProviderIndexedFaceSet::getCoordNode() const
{
    return this->pcMeshCoord;
}

// ------------------------------------------------------

PROPERTY_SOURCE(MeshGui::ViewProviderMeshObject, MeshGui::ViewProviderMesh)

ViewProviderMeshObject::ViewProviderMeshObject()
{
    pcMeshNode = 0;
    pcMeshShape = 0;
}

ViewProviderMeshObject::~ViewProviderMeshObject()
{
}

void ViewProviderMeshObject::attach(App::DocumentObject *pcFeat)
{
    ViewProviderMesh::attach(pcFeat);

    pcMeshNode = new SoFCMeshObjectNode;
    pcHighlight->addChild(pcMeshNode);

    pcMeshShape = new SoFCMeshObjectShape;
    pcHighlight->addChild(pcMeshShape);

    // read the threshold from the preferences
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
    int size = hGrp->GetInt("RenderTriangleLimit", -1);
    if (size > 0) pcMeshShape->renderTriangleLimit = (unsigned int)(pow(10.0f,size));
}

void ViewProviderMeshObject::updateData(const App::Property* prop)
{
    ViewProviderMesh::updateData(prop);
    if (prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        const Mesh::PropertyMeshKernel* mesh = static_cast<const Mesh::PropertyMeshKernel*>(prop);
        this->pcMeshNode->mesh.setValue(mesh->getValuePtr());
        // Needs to update internal bounding box caches
        this->pcMeshShape->touch();
    }
}

void ViewProviderMeshObject::showOpenEdges(bool show)
{
    if (pcOpenEdge) {
        // remove the node and destroy the data
        pcRoot->removeChild(pcOpenEdge);
        pcOpenEdge = 0;
    }

    if (show) {
        pcOpenEdge = new SoSeparator();
        pcOpenEdge->addChild(pcLineStyle);
        pcOpenEdge->addChild(pOpenColor);

        pcOpenEdge->addChild(pcMeshNode);
        pcOpenEdge->addChild(new SoFCMeshObjectBoundary);

        // add to the highlight node
        pcRoot->addChild(pcOpenEdge);
    }
}

SoShape* ViewProviderMeshObject::getShapeNode() const
{
    return this->pcMeshShape;
}

SoNode* ViewProviderMeshObject::getCoordNode() const
{
    return this->pcMeshNode;
}
