/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <qmessagebox.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Tools2D.h>
#include <Base/ViewProj.h>

#include <App/Document.h>
#include <App/PropertyLinks.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/SoFCSelection.h>
#include <Gui/MainWindow.h>
#include <Gui/MouseModel.h>
#include <Gui/Selection.h>
#include <Gui/Window.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/Visitor.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/Gui/SoFCMeshNode.h>

#include "ViewProvider.h"
#include "ViewProviderMeshNode.h"


using namespace MeshGui;


App::PropertyFloatConstraint::Constraints ViewProviderMeshNode::floatRange = {1.0f,64.0f,1.0f};

PROPERTY_SOURCE(MeshGui::ViewProviderMeshNode, Gui::ViewProviderGeometryObject)

ViewProviderMeshNode::ViewProviderMeshNode() : pcOpenEdge(0), m_bEdit(false)
{
  ADD_PROPERTY(LineWidth,(2.0f));
  LineWidth.setConstraints(&floatRange);
  ADD_PROPERTY(PointSize,(2.0f));
  PointSize.setConstraints(&floatRange);
  ADD_PROPERTY(OpenEdges,(false));

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

  // read the correct shape color from the preferences
  Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
  App::Color color = ShapeColor.getValue();
  unsigned long current = color.getPackedValue();
  unsigned long setting = hGrp->GetUnsigned("MeshColor", current);
  if ( current != setting )
  {
    color.setPackedValue((uint32_t)setting);
    ShapeColor.setValue(color);
  }
}

ViewProviderMeshNode::~ViewProviderMeshNode()
{
  pOpenColor->unref();
  pcLineStyle->unref();
  pcPointStyle->unref();
}

void ViewProviderMeshNode::onChanged(const App::Property* prop)
{
  if ( prop == &LineWidth ) {
    pcLineStyle->lineWidth = LineWidth.getValue();
  } else if ( prop == &PointSize ) {
    pcPointStyle->pointSize = PointSize.getValue();
  } else if ( prop == &OpenEdges ) {
    showOpenEdges( OpenEdges.getValue() );
  } else {
    // Set the inverse color for open edges
    if ( prop == &ShapeColor ) {
      setOpenEdgeColorFrom(ShapeColor.getValue());
    } else if ( prop == &ShapeMaterial ) {
      setOpenEdgeColorFrom(ShapeMaterial.getValue().diffuseColor);
    }
    ViewProviderGeometryObject::onChanged(prop);
  }
}

void ViewProviderMeshNode::setOpenEdgeColorFrom( const App::Color& c )
{
  float r=1.0f-c.r; r = r < 0.5f ? 0.0f : 1.0f;
  float g=1.0f-c.g; g = g < 0.5f ? 0.0f : 1.0f;
  float b=1.0f-c.b; b = b < 0.5f ? 0.0f : 1.0f;
  pOpenColor->rgb.setValue(r, g, b);
}

void ViewProviderMeshNode::attach(App::DocumentObject *pcFeat)
{
  ViewProviderGeometryObject::attach(pcFeat);

  // only one selection node for the mesh
  const Mesh::Feature* meshFeature = dynamic_cast<Mesh::Feature*>(pcFeat);
  MeshGui::SoFCMeshNode* mesh = new MeshGui::SoFCMeshNode();
  mesh->setMesh(meshFeature->Mesh.getValuePtr());
  pcHighlight->addChild(mesh);


  // faces
  SoGroup* pcFlatRoot = new SoGroup();

  // read the correct shape color from the preferences
  Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
  bool twoSide = hGrp->GetBool("TwoSideRendering", true);
  if ( twoSide )
  {
    // enable two-side rendering
    SoShapeHints * flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcFlatRoot->addChild(flathints);
  }

  pcFlatRoot->addChild(pcShapeMaterial);
  pcFlatRoot->addChild(pcHighlight);
  addDisplayMaskMode(pcFlatRoot, "Flat");

  // points
  SoGroup* pcPointRoot = new SoGroup();
  pcPointRoot->addChild(pcPointStyle);
  pcPointRoot->addChild(pcFlatRoot);
  addDisplayMaskMode(pcPointRoot, "Point");

  // wires
  SoLightModel* pcLightModel = new SoLightModel();
  pcLightModel->model = SoLightModel::BASE_COLOR;
  SoGroup* pcWireRoot = new SoGroup();
  pcWireRoot->addChild(pcLineStyle);
  pcWireRoot->addChild(pcLightModel);
  pcWireRoot->addChild(pcShapeMaterial);
  pcWireRoot->addChild(pcHighlight);
  addDisplayMaskMode(pcWireRoot, "Wireframe");

  // faces+wires
  SoGroup* pcFlatWireRoot = new SoGroup();
  pcFlatWireRoot->addChild(pcFlatRoot);
  pcFlatWireRoot->addChild(pcWireRoot);
  addDisplayMaskMode(pcFlatWireRoot, "FlatWireframe");
}

void ViewProviderMeshNode::updateData(const App::Property*)
{
  // Needs to update internal bounding box caches
  pcHighlight->touch();
}

QIcon ViewProviderMeshNode::getIcon() const
{
  const char * Mesh_Feature_xpm[] = {
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
}

void ViewProviderMeshNode::setDisplayMode(const char* ModeName)
{
  if ( strcmp("Shaded",ModeName)==0 )
    setDisplayMaskMode("Flat");
  else if ( strcmp("Points",ModeName)==0 )
    setDisplayMaskMode("Point");
  else if ( strcmp("Shaded+Wireframe",ModeName)==0 )
    setDisplayMaskMode("FlatWireframe");
  else if ( strcmp("Wireframe",ModeName)==0 )
    setDisplayMaskMode("Wireframe");

  ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderMeshNode::getDisplayModes(void) const
{
  std::vector<std::string> StrList;

  // add your own modes
  StrList.push_back("Shaded");
  StrList.push_back("Wireframe");
  StrList.push_back("Shaded+Wireframe");
  StrList.push_back("Points");

  return StrList;
}

bool ViewProviderMeshNode::setEdit(int ModNum)
{
  if ( m_bEdit )
      return true;
  m_bEdit = true;
  return true;
}

void ViewProviderMeshNode::unsetEdit(void)
{
  m_bEdit = false;
}

const char* ViewProviderMeshNode::getEditModeName(void)
{
  return "Polygon picking";
}

bool ViewProviderMeshNode::handleEvent(const SoEvent * const ev,Gui::View3DInventorViewer &Viewer)
{
  if ( m_bEdit )
  {
    unsetEdit();
    std::vector<SbVec2f> clPoly = Viewer.getPickedPolygon();
    if ( clPoly.size() < 3 )
      return true;
    if ( clPoly.front() != clPoly.back() )
      clPoly.push_back(clPoly.front());

    // get the normal of the front clipping plane
    SbVec3f b,n;
    Viewer.getNearPlane(b, n);
    Base::Vector3f cPoint(b[0],b[1],b[2]), cNormal(n[0],n[1],n[2]);
    SoCamera* pCam = Viewer.getCamera();  
    SbViewVolume  vol = pCam->getViewVolume (); 

    // create a tool shape from these points
    std::vector<MeshCore::MeshGeomFacet> aFaces;
    bool ok = ViewProviderMesh::createToolMesh( clPoly, vol, cNormal, aFaces );

    // Get the attached mesh property
    Mesh::PropertyMeshKernel& meshProp = ((Mesh::Feature*)pcObject)->Mesh;

    // Get the facet indices inside the tool mesh
    std::vector<unsigned long> indices;
    MeshCore::MeshKernel cToolMesh;
    cToolMesh = aFaces;
    MeshCore::MeshFacetGrid cGrid(meshProp.getValue().getKernel());
    MeshCore::MeshAlgorithm cAlg(meshProp.getValue().getKernel());
    cAlg.GetFacetsFromToolMesh(cToolMesh, cNormal, cGrid, indices);
    meshProp.deleteFacetIndices( indices );

      // update open edge display if needed
//      if ( pcOpenEdge ) 
//      {
//        showOpenEdges(false);
//        showOpenEdges(true);
//      }

    Viewer.render();
    if ( !ok ) // note: the mouse grabbing needs to be released
      //QMessageBox::warning(Viewer.getWidget(),"Invalid polygon","The picked polygon seems to have self-overlappings.\n\nThis could lead to strange rersults.");
      Base::Console().Message("The picked polygon seems to have self-overlappings. This could lead to strange results.");
  }

  return false;
}

void ViewProviderMeshNode::showOpenEdges(bool show)
{
#if 1
  if ( show ) {
    pcOpenEdge = new SoSeparator();
    pcOpenEdge->addChild(pcLineStyle);
    pcOpenEdge->addChild(pOpenColor);

    const Mesh::Feature* meshFeature = dynamic_cast<Mesh::Feature*>(pcObject);
    MeshGui::SoFCMeshOpenEdge* mesh = new MeshGui::SoFCMeshOpenEdge();
    mesh->setMesh(meshFeature->Mesh.getValuePtr());
    pcOpenEdge->addChild(mesh);
    
    // add to the highlight node
    pcHighlight->addChild(pcOpenEdge);
  } else if (pcOpenEdge) {
    // remove the node and destroy the data
    pcHighlight->removeChild(pcOpenEdge);
    pcOpenEdge = 0;
  }
#else
  if ( show ) {
    pcOpenEdge = new SoSeparator();
    pcOpenEdge->addChild(pcLineStyle);
    pcOpenEdge->addChild(pOpenColor);
    SoCoordinate3* points = new SoCoordinate3();
    pcOpenEdge->addChild(points);
    SoLineSet* lines = new SoLineSet();
    pcOpenEdge->addChild(lines);
    // add to the highlight node
    pcHighlight->addChild(pcOpenEdge);

    // Build up the array of border points
    int index=0;
    const MeshCore::MeshKernel& rMesh = dynamic_cast<Mesh::Feature*>(pcObject)->getMesh();
    const MeshCore::MeshFacetArray& rFaces = rMesh.GetFacets();
    const MeshCore::MeshPointArray& rPoint = rMesh.GetPoints();

    // Count number of open edges first
    int ctEdges=0;
    for ( MeshCore::MeshFacetArray::_TConstIterator jt = rFaces.begin(); jt != rFaces.end(); ++jt ) {
      for ( int i=0; i<3; i++ ) {
        if ( jt->_aulNeighbours[i] == ULONG_MAX ) {
          ctEdges++;
        }
      }
    }

    // disable internal notification for speedup
    points->enableNotify(false);
    lines->enableNotify(false);

    points->point.setNum(2*ctEdges);
    lines->numVertices.setNum(ctEdges);
    for ( MeshCore::MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it ) {
      for ( int i=0; i<3; i++ ) {
        if ( it->_aulNeighbours[i] == ULONG_MAX ) {
          const MeshCore::MeshPoint& cP0 = rPoint[it->_aulPoints[i]];
          const MeshCore::MeshPoint& cP1 = rPoint[it->_aulPoints[(i+1)%3]];
          points->point.set1Value(index++, cP0.x, cP0.y, cP0.z);
          points->point.set1Value(index++, cP1.x, cP1.y, cP1.z);
          lines->numVertices.set1Value(index/2-1,2);
        }
      }
    }

    // enable notification
    points->enableNotify(true);
    lines->enableNotify(true);
    points->touch();
    lines->touch();
  } else {
    // remove the node and destroy the data
    pcHighlight->removeChild(pcOpenEdge);
    pcOpenEdge = 0;
  }
#endif
}
