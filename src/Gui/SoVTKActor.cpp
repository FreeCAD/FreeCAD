/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger  <stefantroeger@gmx.net>             *
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

#ifdef FC_USE_VTK 

#include "SoVTKActor.h"
#include "InventorAll.h"
#include <Base/Console.h>
#include <Inventor/SbViewportRegion.h>

#include <vtkMapper.h>
#include <vtkGeometryFilter.h>
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>


using namespace Gui;


SO_NODE_SOURCE(SoVTKActor);

void SoVTKActor::initClass()
{
    SO_NODE_INIT_CLASS(SoVTKActor, SoSeparator, "VTKActor");
}

SoVTKActor::SoVTKActor() : SoSeparator(), m_mapper(NULL)
{
    SO_NODE_CONSTRUCTOR(SoVTKActor);

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
    
    addChild(m_shapeHints);
    addChild(m_materialBinding);
    addChild(m_material);
    addChild(m_normalBinding);
    addChild(m_normals);
    addChild(m_coordinates);
    addChild(m_markers);
    addChild(m_lines);
    addChild(m_faces);
    addChild(m_triangleStrips);
}

SoVTKActor::~SoVTKActor()
{

}

void SoVTKActor::doAction(SoAction* action)
{
    SoNode::doAction(action);
}


void SoVTKActor::setMapper(vtkMapper* m)
{
    m_mapper = m;
    
    
    vtkDataSet *ds;
    vtkPolyData *pd;
    vtkGeometryFilter *gf = NULL;
    vtkPointData *pntData;
    vtkPoints *points;
    vtkDataArray *normals = NULL;
    vtkDataArray *tcoords = NULL;
    int i;
    vtkProperty *prop;
    double *tempd;
    vtkCellArray *cells;
    vtkIdType npts = 0;
    vtkIdType *indx = 0;
    float tempf2;
    vtkPolyDataMapper *pm;
    vtkUnsignedCharArray *colors;
    double *p;
    unsigned char *c;
    vtkTransform *trans;

    Base::Console().Message("render SoVTKActor\n");
    
    // see if the actor has a mapper. it could be an assembly
    if (m_mapper == NULL)
        return;

    ds = m_mapper->GetInput();

    vtkAlgorithmOutput* pdProducer = 0;
    // we really want polydata
    if ( ds->GetDataObjectType() != VTK_POLY_DATA ) {
        gf = vtkGeometryFilter::New();
        gf->SetInputConnection(
        m_mapper->GetInputConnection(0, 0));
        gf->Update();
        pd = gf->GetOutput();
        pdProducer = gf->GetOutputPort();
    }
    else {
        m_mapper->GetInputAlgorithm()->Update();
        pd = static_cast<vtkPolyData *>(ds);
        pdProducer = m_mapper->GetInputConnection(0, 0);
    }

    pm = vtkPolyDataMapper::New();
    pm->SetInputConnection(pdProducer);
    pm->SetScalarRange(m_mapper->GetScalarRange());
    pm->SetScalarVisibility(m_mapper->GetScalarVisibility());
    pm->SetLookupTable(m_mapper->GetLookupTable());

    points = pd->GetPoints();
    pntData = pd->GetPointData();
    normals = pntData->GetNormals();
    tcoords = pntData->GetTCoords();
    colors  = pm->MapScalars(1.0);

    // write out point data if any
    WritePointData(points, normals, tcoords, colors);

    // write out polys if any
    if (pd->GetNumberOfPolys() > 0) {
        
        Base::Console().Message("render polys: %i\n", pd->GetNumberOfPolys());
        m_faces->coordIndex.startEditing();
        int soidx = 0;
        cells = pd->GetPolys();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
        
            for (i = 0; i < npts; i++) {
                m_faces->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_faces->coordIndex.set1Value(soidx, -1);
            ++soidx;
        }
        m_faces->coordIndex.finishEditing();
    }

  // write out tstrips if any
  if (pd->GetNumberOfStrips() > 0) {
      
      Base::Console().Message("render strips\n");
      int soidx = 0;
      cells = pd->GetStrips();
      m_triangleStrips->coordIndex.startEditing();
      for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
          
          for (i = 0; i < npts; i++) {
              m_triangleStrips->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
              ++soidx;
          }
          m_triangleStrips->coordIndex.set1Value(soidx, -1);
          ++soidx;   
      }
      m_triangleStrips->coordIndex.finishEditing();
  }

  // write out lines if any
  if (pd->GetNumberOfLines() > 0) {

        Base::Console().Message("render lines: %i\n", pd->GetNumberOfLines());
        int soidx = 0;
        cells = pd->GetLines();
        m_lines->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
            for (i = 0; i < npts; i++) {
                m_lines->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_lines->coordIndex.set1Value(soidx, -1);
            ++soidx;  
        }
        m_lines->coordIndex.finishEditing();
  }

  // write out verts if any
  // (more complex because there is no IndexedPointSet)
  if (pd->GetNumberOfVerts() > 0){
      
        Base::Console().Message("render verts\n");
        int soidx = 0;
        cells = pd->GetVerts();
        m_markers->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
            for (i = 0; i < npts; i++) {
                m_markers->coordIndex.set1Value(soidx, static_cast<int>(indx[i]));
                ++soidx;
            }
            m_markers->coordIndex.set1Value(soidx, -1);
            ++soidx;  
        }
        m_markers->coordIndex.finishEditing();
  }
  
  if (gf)
    gf->Delete();
  
  pm->Delete();
}

vtkMapper* SoVTKActor::getMapper()
{
    return m_mapper;
}

/*
void SoVTKActor::GLRender(SoGLRenderAction *action)
{
}*/

void SoVTKActor::WritePointData(vtkPoints *points, vtkDataArray *normals,
                                   vtkDataArray *tcoords,
                                   vtkUnsignedCharArray *colors)
{
  double *p;
  int i;
  unsigned char *c;

  if(!points)
      return;
  
  Base::Console().Message("render points: %i", points->GetNumberOfPoints());
  if(colors)
      Base::Console().Message(", colors: %i", colors->GetNumberOfTuples());
  Base::Console().Message("\n");
  
  m_coordinates->point.startEditing();
  for (i = 0; i < points->GetNumberOfPoints(); i++) {
      p = points->GetPoint(i);
      m_coordinates->point.set1Value(i, p[0], p[1], p[2]);
  }
  m_coordinates->point.finishEditing();

  // write out the point nrmal data
  if (normals) {
     
    Base::Console().Message("Write normals: %i\n", normals->GetNumberOfTuples());
    m_normals->vector.startEditing();
    for (i = 0; i < normals->GetNumberOfTuples(); i++) {
        p = normals->GetTuple(i);
        m_normals->vector.set1Value(i, SbVec3f(p[0], p[1], p[2]));
    }
    m_normals->vector.finishEditing();
    
    m_normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
    m_normalBinding->value.touch();
  }
/*
  // write out the point data
  if (tcoords)
    {
        fprintf(fp,"%sTextureCoordinateBinding  {\n",indent);
        VTK_INDENT_MORE;
        fprintf(fp,"%svalue PER_VERTEX_INDEXED\n",indent);
        VTK_INDENT_LESS;
        fprintf(fp,"%s}\n",indent);
        fprintf(fp,"%sTextureCoordinate2 {\n", indent);
    VTK_INDENT_MORE;
    fprintf(fp,"%spoint [\n", indent);
    VTK_INDENT_MORE;
    for (i = 0; i < tcoords->GetNumberOfTuples(); i++)
      {
      p = tcoords->GetTuple(i);
      fprintf (fp,"%s%g %g,\n", indent, p[0], p[1]);
      }
    fprintf(fp,"%s]\n", indent);
        VTK_INDENT_LESS;
    fprintf(fp,"%s}\n", indent);
        VTK_INDENT_LESS;
  }*/

    // write out the point data
    if (colors) {
        m_material->diffuseColor.startEditing();
        m_material->transparency.startEditing();
        for (i = 0; i < colors->GetNumberOfTuples(); i++) {
            
            c = colors->GetPointer(4*i); 
            m_material->diffuseColor.set1Value(i,static_cast<unsigned long>(c[3]),
                                                 static_cast<unsigned long>(c[2]),
                                                 static_cast<unsigned long>(c[1]));
            m_material->transparency.set1Value(i, static_cast<unsigned long>(c[0]));
        }
        m_material->diffuseColor.finishEditing();
        m_material->transparency.finishEditing();
        m_materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
        m_materialBinding->touch();
    }
}

#endif //FC_USE_VTK
