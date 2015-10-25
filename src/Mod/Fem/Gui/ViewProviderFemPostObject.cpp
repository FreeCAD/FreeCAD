/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedMarkerSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoDrawStyle.h>
#endif

#include "ViewProviderFemPostObject.h"
#include <Mod/Fem/App/FemPostObject.h>
#include <Base/Console.h>

#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLookupTable.h>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostObject, Gui::ViewProviderDocumentObject)

ViewProviderFemPostObject::ViewProviderFemPostObject() : m_blockPropertyChanges(false)
{
     //initialize the properties
    ADD_PROPERTY(Coloring,((long)0));    
    ADD_PROPERTY(Transperency, (0));
    
    m_transperencyConstraint.StepSize = 1;
    m_transperencyConstraint.LowerBound = 0;
    m_transperencyConstraint.UpperBound = 100;
    Transperency.setConstraints(&m_transperencyConstraint);
    
    sPixmap = "fem-fem-mesh-from-shape";
    
    //create the subnodes which do the visualization work
    m_shapeHints = new SoShapeHints();
    m_shapeHints->ref();
    m_shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    m_coordinates = new SoCoordinate3();
    m_coordinates->ref();
    m_materialBinding = new SoMaterialBinding();
    m_materialBinding->ref();    
    m_material = new SoMaterial();
    m_material->ref();
    m_normalBinding = new SoNormalBinding();
    m_normalBinding->ref();
    m_normals = new SoNormal();
    m_normals->ref();
    m_faces = new SoIndexedFaceSet();
    m_faces->ref();
    m_triangleStrips = new SoIndexedTriangleStripSet();
    m_triangleStrips->ref();
    m_markers = new SoIndexedMarkerSet();
    m_markers->ref();
    m_lines = new SoIndexedLineSet();
    m_lines->ref();
    m_drawStyle = new SoDrawStyle();
    m_drawStyle->ref();
    m_seperator = new SoSeparator();
    m_seperator->ref();
}

ViewProviderFemPostObject::~ViewProviderFemPostObject()
{
    m_shapeHints->unref();
    m_coordinates->unref();
    m_materialBinding->unref();    
    m_drawStyle->unref();
    m_normalBinding->unref();
    m_normals->unref();
    m_faces->unref();
    m_triangleStrips->unref();
    m_markers->unref();
    m_lines->unref();
    m_seperator->unref();
    m_material->unref();
}

void ViewProviderFemPostObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);
    
    // flat
    SoGroup* pcFlatRoot = new SoGroup();
    // face nodes
    pcFlatRoot->addChild(m_coordinates);
    pcFlatRoot->addChild(m_shapeHints);
    pcFlatRoot->addChild(m_material);
    pcFlatRoot->addChild(m_materialBinding);
    pcFlatRoot->addChild(m_faces);

    // line
    SoGroup* pcWireRoot = new SoGroup();
    pcWireRoot->addChild(m_coordinates);
    pcWireRoot->addChild(m_drawStyle);
    pcWireRoot->addChild(m_lines);

    // Points
    SoGroup* pcPointsRoot = new SoSeparator();
    pcPointsRoot->addChild(m_coordinates);
    pcPointsRoot->addChild(m_markers);
 
    //all 
    m_seperator->addChild(pcFlatRoot);
    m_seperator->addChild(pcWireRoot);
    m_seperator->addChild(pcPointsRoot);
    addDisplayMaskMode(m_seperator, "Default");
    setDisplayMaskMode("Default");
    
    setupPipeline();   
}

void ViewProviderFemPostObject::setDisplayMode(const char* ModeName)
{
    if (strcmp("Outline",ModeName)==0)
        m_currentAlgorithm = m_outline;
    else if (strcmp("Surface with Edges",ModeName)==0)
        m_currentAlgorithm = m_surfaceEdges;
    else if (strcmp("Surface",ModeName)==0)
        m_currentAlgorithm = static_cast<Fem::FemPostObject*>(getObject())->getPolyAlgorithm();
    else if (strcmp("Wireframe",ModeName)==0)
        m_currentAlgorithm = m_wireframe;
    /*else if (strcmp("Nodes",ModeName)==0)
        setDisplayMaskMode("Nodes");*/

    update();
    
    ViewProviderDocumentObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderFemPostObject::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Outline");
   // StrList.push_back("Points");
    StrList.push_back("Surface");
    StrList.push_back("Surface with Edges");
    StrList.push_back("Wireframe");
    return StrList;
}

void ViewProviderFemPostObject::update() {

    if(!setupPipeline())
        return;
    
    m_currentAlgorithm->Update();
    vtkPolyData* poly = m_currentAlgorithm->GetOutput();  
    
    //update the coloring property
    m_blockPropertyChanges = true;
    
    std::string val;
    if(Coloring.getEnums() && Coloring.getValue() >= 0)
        val = Coloring.getValueAsString();
    
    std::vector<std::string> colorArrays;
    colorArrays.push_back("None");
    
    vtkPointData* point = poly->GetPointData();
    for(int i=0; i<point->GetNumberOfArrays(); ++i) 
        colorArrays.push_back(point->GetArrayName(i));

    vtkCellData* cell = poly->GetCellData();
    for(int i=0; i<cell->GetNumberOfArrays(); ++i)
        colorArrays.push_back(cell->GetArrayName(i));
    
    App::Enumeration empty;
    Coloring.setValue(empty);
    m_coloringEnum.setEnums(colorArrays);
    Coloring.setValue(m_coloringEnum);
    
    std::vector<std::string>::iterator it = std::find(colorArrays.begin(), colorArrays.end(), val);
    if(!val.empty() && it != colorArrays.end())
        Coloring.setValue(val.c_str());
    
    Coloring.purgeTouched();
    
    m_blockPropertyChanges = false;
    
    //update the visualization
    update3D();
}


void ViewProviderFemPostObject::update3D() {
    
    if(!setupPipeline())
        return;
    
    vtkPolyData* pd = m_currentAlgorithm->GetOutput();  
    
    vtkPointData *pntData;
    vtkPoints *points;
    vtkDataArray *normals = NULL;
    vtkDataArray *tcoords = NULL;
    vtkCellArray *cells;
    vtkIdType npts = 0;
    vtkIdType *indx = 0;
 
    points = pd->GetPoints();
    pntData = pd->GetPointData();
    normals = pntData->GetNormals();
    tcoords = pntData->GetTCoords();

    // write out point data if any
    WritePointData(points, normals, tcoords);
    WriteColorData();
    WriteTransperency();

    // write out polys if any
    if (pd->GetNumberOfPolys() > 0) {
        
        Base::Console().Message("render polys: %i\n", pd->GetNumberOfPolys());
        m_faces->coordIndex.startEditing();
        int soidx = 0;
        cells = pd->GetPolys();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
        
            for (int i = 0; i < npts; i++) {
                m_faces->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_faces->coordIndex.set1Value(soidx, -1);
            ++soidx;
        }
        m_faces->coordIndex.setNum(soidx);
        m_faces->coordIndex.finishEditing();
    }
    else        
        m_faces->coordIndex.setNum(0);
    

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0) {
      
      Base::Console().Message("render strips\n");
      int soidx = 0;
      cells = pd->GetStrips();
      m_triangleStrips->coordIndex.startEditing();
      for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
          
          for (int i = 0; i < npts; i++) {
              m_triangleStrips->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
              ++soidx;
          }
          m_triangleStrips->coordIndex.set1Value(soidx, -1);
          ++soidx;   
      }
      m_triangleStrips->coordIndex.setNum(soidx);
      m_triangleStrips->coordIndex.finishEditing();
  }
  else 
      m_triangleStrips->coordIndex.setNum(0);

  // write out lines if any
  if (pd->GetNumberOfLines() > 0) {

        Base::Console().Message("render lines: %i\n", pd->GetNumberOfLines());
        int soidx = 0;
        cells = pd->GetLines();
        m_lines->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
            for (int i = 0; i < npts; i++) {
                m_lines->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_lines->coordIndex.set1Value(soidx, -1);
            ++soidx;  
        }
        m_lines->coordIndex.setNum(soidx);
        m_lines->coordIndex.finishEditing();
  }
  else 
      m_lines->coordIndex.setNum(0);

  // write out verts if any
  // (more complex because there is no IndexedPointSet)
  if (pd->GetNumberOfVerts() > 0){
      
        Base::Console().Message("render verts\n");
        int soidx = 0;
        cells = pd->GetVerts();
        m_markers->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
            for (int i = 0; i < npts; i++) {
                m_markers->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_markers->coordIndex.set1Value(soidx, -1);
            ++soidx;  
        }
        m_markers->coordIndex.setNum(soidx);
        m_markers->coordIndex.finishEditing();
  }
  else 
      m_markers->coordIndex.setNum(0);
}

void ViewProviderFemPostObject::WritePointData(vtkPoints* points, vtkDataArray* normals, vtkDataArray* tcoords) {

    
    double *p;
    int i;

    if(!points)
        return;

    Base::Console().Message("render points: %i", points->GetNumberOfPoints());
    Base::Console().Message("\n");

    m_coordinates->point.startEditing();
    for (i = 0; i < points->GetNumberOfPoints(); i++) {
        p = points->GetPoint(i);
        m_coordinates->point.set1Value(i, p[0], p[1], p[2]);
    }
    m_coordinates->point.setNum(points->GetNumberOfPoints());
    m_coordinates->point.finishEditing();

    // write out the point normal data
    if (normals) {
        
        Base::Console().Message("Write normals: %i\n", normals->GetNumberOfTuples());
        m_normals->vector.startEditing();
        for (i = 0; i < normals->GetNumberOfTuples(); i++) {
            p = normals->GetTuple(i);
            m_normals->vector.set1Value(i, SbVec3f(p[0], p[1], p[2]));
        }
        m_normals->vector.setNum(normals->GetNumberOfTuples());
        m_normals->vector.finishEditing();

        m_normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
        m_normalBinding->value.touch();
    }
}

void ViewProviderFemPostObject::WriteColorData() {

    if(!m_currentAlgorithm)
        return;
    
    if(Coloring.getEnumVector().empty() || Coloring.getValue() == 0) {
        
        m_material->diffuseColor.setValue(SbColor(0.8,0.8,0.8));
        m_material->transparency.setValue(0.);
        m_materialBinding->value = SoMaterialBinding::OVERALL;
        m_materialBinding->touch();  
        return;
    };
 
    
    int array = Coloring.getValue() - 1; //0 is none   
    vtkPolyData*  pd = m_currentAlgorithm->GetOutput();         
    vtkDataArray* data = pd->GetPointData()->GetArray(array);

    //build the lookuptable
    double range[2];
    data->GetRange(range, 0);
    m_lookup->SetTableRange(range[0], range[1]);
    m_lookup->SetScaleToLinear();
    m_lookup->Build();

    m_material->diffuseColor.startEditing();
     
    for (int i = 0; i < pd->GetNumberOfPoints(); i++) {
        
        double value = data->GetComponent(i, 0);
        double c[3];
        m_lookup->GetColor(value, c);
        m_material->diffuseColor.set1Value(i, c[0], c[1], c[2]);
    }
    m_material->diffuseColor.finishEditing();
    m_materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    m_materialBinding->touch();    
}

void ViewProviderFemPostObject::WriteTransperency() {
    
    float trans = float(Transperency.getValue()) / 100.;
    
    m_material->transparency.startEditing();
    for(int i=0; i<m_material->diffuseColor.getNum(); ++i)
        m_material->transparency.set1Value(i, trans);
    
    m_material->transparency.finishEditing();
}



void ViewProviderFemPostObject::updateData(const App::Property* p) {
    
    if( strcmp(p->getName(), "ModificationTime") == 0 && setupPipeline() ) {
        update();
    }
}

bool ViewProviderFemPostObject::setupPipeline() {
    
    if(!static_cast<Fem::FemPostObject*>(getObject())->getPolyAlgorithm())
        return false;
    
    if(!m_currentAlgorithm) {
        
        vtkSmartPointer<vtkPolyDataAlgorithm> algorithm = static_cast<Fem::FemPostObject*>(getObject())->getPolyAlgorithm();
        
        m_outline   = vtkOutlineCornerFilter::New();
        m_outline->SetInputConnection(algorithm->GetOutputPort());
        
        m_surface = vtkGeometryFilter::New();
        m_surface->SetInputConnection(algorithm->GetOutputPort());
        
        m_wireframe = vtkExtractEdges::New();
        m_wireframe->SetInputConnection(algorithm->GetOutputPort());
        
        m_surfaceEdges = vtkAppendPolyData::New();
        m_surfaceEdges->AddInputConnection(m_surface->GetOutputPort());
        m_surfaceEdges->AddInputConnection(m_wireframe->GetOutputPort());
        
        m_lookup = vtkLookupTable::New();
        m_lookup->SetRampToLinear();
        
        m_currentAlgorithm = m_outline;
    }
    
    return true;
}


void ViewProviderFemPostObject::onChanged(const App::Property* prop) {
    
    if(m_blockPropertyChanges)
        return;
    
    Base::Console().Message("On Changed: %s\n", prop->getName());    
    if(prop == &Coloring && setupPipeline()) {
        WriteColorData();
    } 
    else if(prop == &Transperency) {
        WriteTransperency();
    }    
    
    ViewProviderDocumentObject::onChanged(prop);
}
