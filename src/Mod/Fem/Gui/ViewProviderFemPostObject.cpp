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
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedPointSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoIndexedTriangleStripSet.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoDrawStyle.h>

# include <vtkPointData.h>
# include <vtkCellArray.h>
# include <vtkCellData.h>
# include <vtkLookupTable.h>

# include <QMessageBox>
#endif

#include "ViewProviderFemPostObject.h"
#include "TaskPostBoxes.h"
#include <Mod/Fem/App/FemPostObject.h>
#include <Base/Console.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/SoFCColorBar.h>

using namespace FemGui;


PROPERTY_SOURCE(FemGui::ViewProviderFemPostObject, Gui::ViewProviderDocumentObject)

ViewProviderFemPostObject::ViewProviderFemPostObject() : m_blockPropertyChanges(false)
{
     //initialize the properties
    ADD_PROPERTY_TYPE(Field,((long)0), "Coloring", App::Prop_None, "Select the field used for calculating the color");
    ADD_PROPERTY_TYPE(VectorMode,((long)0), "Coloring", App::Prop_None, "Select what to show for a vector field");
    ADD_PROPERTY(Transparency, (0));

    sPixmap = "fem-femmesh-from-shape";

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
    m_drawStyle->lineWidth.setValue(2);
    m_drawStyle->pointSize.setValue(3);
    m_seperator = new SoSeparator();
    m_seperator->ref();

    // simple color bar
    m_colorRoot = new SoSeparator();
    m_colorRoot->ref();
    m_colorStyle = new SoDrawStyle();
    m_colorStyle->ref();
    m_colorRoot->addChild(m_colorStyle);
    m_colorBar = new Gui::SoFCColorBar;
    m_colorBar->Attach(this);
    m_colorBar->ref();

    //create the vtk algorithms we use for visualisation
    m_outline   = vtkSmartPointer<vtkOutlineCornerFilter>::New();
    m_points = vtkSmartPointer<vtkVertexGlyphFilter>::New();
    m_pointsSurface = vtkSmartPointer<vtkVertexGlyphFilter>::New();
    m_surface = vtkSmartPointer<vtkGeometryFilter>::New();
    m_wireframe = vtkSmartPointer<vtkExtractEdges>::New();
    m_wireframeSurface = vtkSmartPointer<vtkExtractEdges>::New();
    m_surfaceEdges = vtkSmartPointer<vtkAppendPolyData>::New();
    m_pointsSurface->AddInputConnection(m_surface->GetOutputPort());
    m_wireframeSurface->AddInputConnection(m_surface->GetOutputPort());
    m_surfaceEdges->AddInputConnection(m_surface->GetOutputPort());
    m_surfaceEdges->AddInputConnection(m_wireframeSurface->GetOutputPort());

    m_currentAlgorithm = m_outline;

    updateProperties();  // initialize the enums
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
    m_colorBar->Detach(this);
    m_colorBar->unref();
    m_colorStyle->unref();
    m_colorRoot->unref();
}

void ViewProviderFemPostObject::attach(App::DocumentObject *pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    // face nodes
    m_seperator->addChild(m_shapeHints);
    m_seperator->addChild(m_drawStyle);
    m_seperator->addChild(m_materialBinding);
    m_seperator->addChild(m_material);
    m_seperator->addChild(m_coordinates);
    m_seperator->addChild(m_markers);
    m_seperator->addChild(m_lines);
    m_seperator->addChild(m_faces);

    // Check for an already existing color bar
    Gui::SoFCColorBar* pcBar = ((Gui::SoFCColorBar*)findFrontRootOfType( Gui::SoFCColorBar::getClassTypeId() ));
    if ( pcBar ) {
        float fMin = m_colorBar->getMinValue();
        float fMax = m_colorBar->getMaxValue();

        // Attach to the foreign color bar and delete our own bar
        pcBar->Attach(this);
        pcBar->ref();
        pcBar->setRange(fMin, fMax, 3);
        pcBar->Notify(0);
        m_colorBar->Detach(this);
        m_colorBar->unref();
        m_colorBar = pcBar;
    }

    m_colorRoot->addChild(m_colorBar);

    //all
    addDisplayMaskMode(m_seperator, "Default");
    setDisplayMaskMode("Default");

    setupPipeline();
}

SoSeparator* ViewProviderFemPostObject::getFrontRoot(void) const {

    return m_colorRoot;
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
    else if (strcmp("Wireframe (surface only)",ModeName)==0)
        m_currentAlgorithm = m_wireframeSurface;
    else if (strcmp("Nodes",ModeName)==0)
        m_currentAlgorithm = m_points;
    else if (strcmp("Nodes (surface only)",ModeName)==0)
        m_currentAlgorithm = m_pointsSurface;

    update();

    ViewProviderDocumentObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderFemPostObject::getDisplayModes(void) const
{
    std::vector<std::string> StrList;
    StrList.push_back("Outline");
    StrList.push_back("Nodes");
    //StrList.push_back("Nodes (surface only)"); somehow this filter does not work
    StrList.push_back("Surface");
    StrList.push_back("Surface with Edges");
    StrList.push_back("Wireframe");
    StrList.push_back("Wireframe (surface only)");
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
    for(int i=0; i<point->GetNumberOfArrays(); ++i) {
        std::string FieldName = point->GetArrayName(i);
        if (FieldName != "Texture Coordinates")
            colorArrays.push_back(FieldName);
    }

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
    bool ResetColorBarRange = true;
    WriteColorData(ResetColorBarRange);
    WriteTransparency();

    // write out polys if any
    if (pd->GetNumberOfPolys() > 0) {

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

    Q_UNUSED(tcoords)
    double *p;
    int i;

    if(!points)
        return;

    m_coordinates->point.startEditing();
    m_coordinates->point.setNum(points->GetNumberOfPoints());
    for (i = 0; i < points->GetNumberOfPoints(); i++) {
        p = points->GetPoint(i);
        m_coordinates->point.set1Value(i, p[0], p[1], p[2]);
    }
    m_coordinates->point.finishEditing();

    // write out the point normal data
    if (normals) {

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

void ViewProviderFemPostObject::WriteColorData(bool ResetColorBarRange) {

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
    if (ResetColorBarRange == true) {
        double range[2];
        data->GetRange(range, component);
        m_colorBar->setRange(range[0], range[1]);
    }

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
        App::Color c = m_colorBar->getColor(value);
        m_material->diffuseColor.set1Value(i, c.r, c.g, c.b);
    }
    m_material->diffuseColor.finishEditing();
    m_materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    m_materialBinding->touch();
}

void ViewProviderFemPostObject::WriteTransparency() {

    float trans = float(Transparency.getValue()) / 100.;
    m_material->transparency.setValue(trans);
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

    bool ResetColorBarRange = true;
    if(prop == &Field && setupPipeline()) {
        updateProperties();
        WriteColorData(ResetColorBarRange);
        WriteTransparency();
    }
    else if(prop == &VectorMode && setupPipeline()) {
        WriteColorData(ResetColorBarRange);
        WriteTransparency();
    }
    else if(prop == &Transparency) {
        WriteTransparency();
    }

    ViewProviderDocumentObject::onChanged(prop);
}

bool ViewProviderFemPostObject::doubleClicked(void) {
    // work around for a problem in VTK implementation: https://forum.freecadweb.org/viewtopic.php?t=10587&start=130#p125688
    // check if backlight is enabled
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool isBackLightEnabled = hGrp->GetBool("EnableBacklight", false);
    if (isBackLightEnabled == false)
        Base::Console().Error("Backlight is not enabled. Due to a VTK implementation problem you really should consider to enable backlight in FreeCAD display preferences if you work with VTK post processing.\n");
    // set edit
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

void ViewProviderFemPostObject::hide(void) {
    Gui::ViewProviderDocumentObject::hide();
    m_colorStyle->style = SoDrawStyle::INVISIBLE;
}

void ViewProviderFemPostObject::show(void) {
    Gui::ViewProviderDocumentObject::show();
    m_colorStyle->style = SoDrawStyle::FILLED;
}


void ViewProviderFemPostObject::OnChange(Base::Subject< int >& /*rCaller*/, int /*rcReason*/) {
    bool ResetColorBarRange = false;
    WriteColorData(ResetColorBarRange);
}
