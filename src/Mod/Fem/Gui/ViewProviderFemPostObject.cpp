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
#include <Inventor/nodes/SoIndexedPointSet.h>
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
#include "TaskPostBoxes.h"
#include <Mod/Fem/App/FemPostObject.h>
#include <Base/Console.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Gui/Document.h>

#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLookupTable.h>
#include <QMessageBox>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostObject, Gui::ViewProviderDocumentObject)

ViewProviderFemPostObject::ViewProviderFemPostObject() : m_blockPropertyChanges(false)
{
     //initialize the properties
    ADD_PROPERTY_TYPE(Field,((long)0), "Coloring", App::Prop_None, "Select the field used for calculating the color");    
    ADD_PROPERTY_TYPE(VectorMode,((long)0), "Coloring", App::Prop_None, "Select what to show for a vector field");
    ADD_PROPERTY(Transperency, (0));
       
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
    m_markers = new SoIndexedPointSet();
    m_markers->ref();
    m_lines = new SoIndexedLineSet();
    m_lines->ref();
    m_drawStyle = new SoDrawStyle();
    m_drawStyle->ref();
    m_seperator = new SoSeparator();
    m_seperator->ref();
    
    //create the vtk algorithms we use for visualisation
    m_outline   = vtkOutlineCornerFilter::New();
    m_points = vtkVertexGlyphFilter::New();
    m_surface = vtkGeometryFilter::New();
    m_wireframe = vtkExtractEdges::New();
    m_surfaceEdges = vtkAppendPolyData::New();
    m_surfaceEdges->AddInputConnection(m_surface->GetOutputPort());
    m_surfaceEdges->AddInputConnection(m_wireframe->GetOutputPort());
        
    m_lookup = vtkLookupTable::New();
    m_lookup->SetRampToLinear();
      
    m_currentAlgorithm = m_outline;
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
        m_currentAlgorithm = m_surface;
    else if (strcmp("Wireframe",ModeName)==0)
        m_currentAlgorithm = m_wireframe;
    else if (strcmp("Nodes",ModeName)==0)
        m_currentAlgorithm = m_points;

    update();
    
    ViewProviderDocumentObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderFemPostObject::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Outline");
    StrList.push_back("Nodes");
    StrList.push_back("Surface");
    StrList.push_back("Surface with Edges");
    StrList.push_back("Wireframe");
    return StrList;
}

void ViewProviderFemPostObject::update() {

    if(!setupPipeline())
        return;
    
    m_currentAlgorithm->Update();    
    updateProperties();
    update3D();
}

void ViewProviderFemPostObject::updateProperties() {
    
    m_blockPropertyChanges = true;
    vtkPolyData* poly = m_currentAlgorithm->GetOutput(); 
    
    //coloring
    std::string val;
    if(Field.getEnums() && Field.getValue() >= 0)
        val = Field.getValueAsString();
    
    std::vector<std::string> colorArrays;
    colorArrays.push_back("None");
    
    vtkPointData* point = poly->GetPointData();
    for(int i=0; i<point->GetNumberOfArrays(); ++i) 
        colorArrays.push_back(point->GetArrayName(i));

    vtkCellData* cell = poly->GetCellData();
    for(int i=0; i<cell->GetNumberOfArrays(); ++i)
        colorArrays.push_back(cell->GetArrayName(i));
    
    App::Enumeration empty;
    Field.setValue(empty);
    m_coloringEnum.setEnums(colorArrays);
    Field.setValue(m_coloringEnum);
    
    std::vector<std::string>::iterator it = std::find(colorArrays.begin(), colorArrays.end(), val);
    if(!val.empty() && it != colorArrays.end())
        Field.setValue(val.c_str());
    
    Field.purgeTouched();
    
    //Vector mode
    if(VectorMode.getEnums() && VectorMode.getValue() >= 0)
        val = VectorMode.getValueAsString();
    
    colorArrays.clear();
    if(Field.getValue() == 0)         
        colorArrays.push_back("Not a vector");
    else {
        int array = Field.getValue() - 1; //0 is none   
        vtkPolyData*  pd = m_currentAlgorithm->GetOutput();         
        vtkDataArray* data = pd->GetPointData()->GetArray(array);

        if(data->GetNumberOfComponents() == 1)
            colorArrays.push_back("Not a vector");
        else {
            colorArrays.push_back("Magnitude");
            if(data->GetNumberOfComponents() >= 2) {
                colorArrays.push_back("X");
                colorArrays.push_back("Y");
            }
            if(data->GetNumberOfComponents() >= 3)
                colorArrays.push_back("Z");
        }
    }

    VectorMode.setValue(empty);
    m_vectorEnum.setEnums(colorArrays);
    VectorMode.setValue(m_vectorEnum); 
    
    it = std::find(colorArrays.begin(), colorArrays.end(), val);
    if(!val.empty() && it != colorArrays.end())
        VectorMode.setValue(val.c_str());
    
    m_blockPropertyChanges = false;
}

void ViewProviderFemPostObject::update3D() {
    
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
  if (pd->GetNumberOfVerts() > 0){
      
        Base::Console().Message("render verts: %i\n", pd->GetNumberOfVerts());
        int soidx = 0;
        cells = pd->GetVerts();
        m_markers->coordIndex.startEditing();
        m_markers->coordIndex.setNum(pd->GetNumberOfVerts());
        for (cells->InitTraversal(); cells->GetNextCell(npts,indx); ) {
            m_markers->coordIndex.set1Value(soidx, static_cast<int>(indx[0]));
            ++soidx;
        }
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
    m_coordinates->point.setNum(points->GetNumberOfPoints());
    for (i = 0; i < points->GetNumberOfPoints(); i++) {
        p = points->GetPoint(i);
        m_coordinates->point.set1Value(i, p[0], p[1], p[2]);
    }
    m_coordinates->point.finishEditing();

    // write out the point normal data
    if (normals) {
        
        Base::Console().Message("Write normals: %i\n", normals->GetNumberOfTuples());
        m_normals->vector.startEditing();
        m_normals->vector.setNum(normals->GetNumberOfTuples());
        for (i = 0; i < normals->GetNumberOfTuples(); i++) {
            p = normals->GetTuple(i);
            m_normals->vector.set1Value(i, SbVec3f(p[0], p[1], p[2]));
        }
        m_normals->vector.finishEditing();

        m_normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
        m_normalBinding->value.touch();
    }
}

void ViewProviderFemPostObject::WriteColorData() {

    if(!setupPipeline())
        return;
    
    if(Field.getEnumVector().empty() || Field.getValue() == 0) {
        
        m_material->diffuseColor.setValue(SbColor(0.8,0.8,0.8));
        m_material->transparency.setValue(0.);
        m_materialBinding->value = SoMaterialBinding::OVERALL;
        m_materialBinding->touch();  
        return;
    };
 
    
    int array = Field.getValue() - 1; //0 is none   
    vtkPolyData*  pd = m_currentAlgorithm->GetOutput();         
    vtkDataArray* data = pd->GetPointData()->GetArray(array);

    int component = VectorMode.getValue() - 1; //0 is either "Not a vector" or magnitude, for -1 is correct for magnitude. x y and z are one number too high
    if(strcmp(VectorMode.getValueAsString(), "Not a vector")==0)
        component = 0;
        
    //build the lookuptable
    double range[2];
    data->GetRange(range, component);
    m_lookup->SetTableRange(range[0], range[1]);
    m_lookup->SetScaleToLinear();
    m_lookup->Build();

    m_material->diffuseColor.startEditing();
     
    for (int i = 0; i < pd->GetNumberOfPoints(); i++) {
        
        double value = 0;
        if(component >= 0)
            value = data->GetComponent(i, component);
        else {
            for(int j=0; j<data->GetNumberOfComponents(); ++j)
                value += std::pow(data->GetComponent(i, j),2);
            
            value = std::sqrt(value);
        }
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
    
    if( strcmp(p->getName(), "Data") == 0 ) {
        update();
    }
}

bool ViewProviderFemPostObject::setupPipeline() {
    
    vtkDataObject* data = static_cast<Fem::FemPostObject*>(getObject())->Data.getValue();
    
    if(!data)
        return false;
    
        
    m_outline->SetInputData(data);
    m_surface->SetInputData(data);
    m_wireframe->SetInputData(data);
    m_points->SetInputData(data);
    
    return true;
}


void ViewProviderFemPostObject::onChanged(const App::Property* prop) {
    
    if(m_blockPropertyChanges)
        return;
    
    if(prop == &Field && setupPipeline()) {
        updateProperties();
        WriteColorData();
        WriteTransperency();
    } 
    else if(prop == &VectorMode && setupPipeline()) {
        WriteColorData();
        WriteTransperency();
    }
    else if(prop == &Transperency) {
        WriteTransperency();
    }    
    
    ViewProviderDocumentObject::onChanged(prop);
}

bool ViewProviderFemPostObject::doubleClicked(void) {
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}


bool ViewProviderFemPostObject::setEdit(int ModNum) {
    
     if (ModNum == ViewProvider::Default || ModNum == 1 ) {

        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgPost *postDlg = qobject_cast<TaskDlgPost*>(dlg);
        if (postDlg && postDlg->getView() != this)
            postDlg = 0; // another pad left open its task panel
        if (dlg && !postDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // start the edit dialog
        if (postDlg)
            Gui::Control().showDialog(postDlg);
        else {
            postDlg = new TaskDlgPost(this);
            setupTaskDialog(postDlg);
            Gui::Control().showDialog(postDlg);
        }

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemPostObject::setupTaskDialog(TaskDlgPost* dlg) {

    dlg->appendBox(new TaskPostDisplay(this));
}

void ViewProviderFemPostObject::unsetEdit(int ModNum) {
    
    if (ModNum == ViewProvider::Default) {
        // and update the pad
        //getSketchObject()->getDocument()->recompute();

        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

