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
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedPointSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <functional>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkImplicitFunction.h>
#include <vtkPointData.h>

#include <QApplication>
#include <QMessageBox>
#include <QTextStream>
#endif

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/SoFCColorBar.h>
#include <Gui/TaskView/TaskDialog.h>

#include <Mod/Fem/App/FemPostFilter.h>

#include "TaskPostBoxes.h"
#include "ViewProviderAnalysis.h"
#include "ViewProviderFemPostObject.h"


using namespace FemGui;
namespace sp = std::placeholders;

#ifdef VTK_CELL_ARRAY_V2
using vtkIdTypePtr = const vtkIdType*;
#else
using vtkIdTypePtr = vtkIdType*;
#endif

// ----------------------------------------------------------------------------

namespace
{
/*
 * The class FemPostObjectSelectionObserver notifies a ViewProviderFemPostObject
 *  only if its selection status has changed
 */
class FemPostObjectSelectionObserver
{
public:
    static FemPostObjectSelectionObserver& instance()
    {
        static FemPostObjectSelectionObserver inst;
        return inst;
    }
    void registerFemPostObject(ViewProviderFemPostObject* vp)
    {
        views.insert(vp);
    }
    void unregisterFemPostObject(ViewProviderFemPostObject* vp)
    {
        auto it = views.find(vp);
        if (it != views.end()) {
            views.erase(it);
        }
    }

    void selectionChanged(const Gui::SelectionChanges& msg)
    {
        Gui::SelectionObject obj(msg);
        auto findVP = std::find_if(views.begin(), views.end(), [&obj](const auto& vp) {
            return obj.getObject() == vp->getObject();
        });

        if (findVP != views.end()) {
            (*findVP)->onSelectionChanged(msg);
        }
    }

private:
    FemPostObjectSelectionObserver()
    {
        // NOLINTBEGIN
        this->connectSelection = Gui::Selection().signalSelectionChanged.connect(
            std::bind(&FemPostObjectSelectionObserver::selectionChanged, this, sp::_1));
        // NOLINTEND
    }

    ~FemPostObjectSelectionObserver() = default;

public:
    FemPostObjectSelectionObserver(const FemPostObjectSelectionObserver&) = delete;
    FemPostObjectSelectionObserver& operator=(const FemPostObjectSelectionObserver&) = delete;

private:
    std::set<ViewProviderFemPostObject*> views;
    using Connection = boost::signals2::scoped_connection;
    Connection connectSelection;
};

}  // namespace

// ----------------------------------------------------------------------------

PROPERTY_SOURCE(FemGui::ViewProviderFemPostObject, Gui::ViewProviderDocumentObject)

ViewProviderFemPostObject::ViewProviderFemPostObject()
{
    // initialize the properties
    ADD_PROPERTY_TYPE(Field,
                      ((long)0),
                      "Coloring",
                      App::Prop_None,
                      "Select the field used for calculating the color");
    ADD_PROPERTY_TYPE(VectorMode,
                      ((long)0),
                      "Coloring",
                      App::Prop_None,
                      "Select what to show for a vector field");
    ADD_PROPERTY(Transparency, (0));

    sPixmap = "fem-femmesh-from-shape";

    // create the subnodes which do the visualization work
    m_shapeHints = new SoShapeHints();
    m_shapeHints->ref();
    m_shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    m_shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
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
    m_separator = new SoSeparator();
    m_separator->ref();

    // simple color bar
    m_colorRoot = new SoSeparator();
    m_colorRoot->ref();
    m_colorStyle = new SoDrawStyle();
    m_colorStyle->ref();
    m_colorRoot->addChild(m_colorStyle);
    m_colorBar = new Gui::SoFCColorBar;
    m_colorBar->Attach(this);
    m_colorBar->ref();

    // create the vtk algorithms we use for visualisation
    m_outline = vtkSmartPointer<vtkOutlineCornerFilter>::New();
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

    FemPostObjectSelectionObserver::instance().registerFemPostObject(this);
}

ViewProviderFemPostObject::~ViewProviderFemPostObject()
{
    FemPostObjectSelectionObserver::instance().unregisterFemPostObject(this);
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
    m_separator->unref();
    m_material->unref();
    m_colorBar->Detach(this);
    m_colorBar->unref();
    m_colorStyle->unref();
    m_colorRoot->unref();
}

void ViewProviderFemPostObject::attach(App::DocumentObject* pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    // face nodes
    m_separator->addChild(m_shapeHints);
    m_separator->addChild(m_drawStyle);
    m_separator->addChild(m_materialBinding);
    m_separator->addChild(m_material);
    m_separator->addChild(m_coordinates);
    m_separator->addChild(m_markers);
    m_separator->addChild(m_lines);
    m_separator->addChild(m_faces);

    // Check for an already existing color bar
    Gui::SoFCColorBar* pcBar =
        ((Gui::SoFCColorBar*)findFrontRootOfType(Gui::SoFCColorBar::getClassTypeId()));
    if (pcBar) {
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

    // all
    addDisplayMaskMode(m_separator, "Default");
    setDisplayMaskMode("Default");

    (void)setupPipeline();
}

SoSeparator* ViewProviderFemPostObject::getFrontRoot() const
{
    return m_colorRoot;
}

void ViewProviderFemPostObject::setDisplayMode(const char* ModeName)
{
    if (strcmp("Outline", ModeName) == 0) {
        m_currentAlgorithm = m_outline;
    }
    else if (strcmp("Surface with Edges", ModeName) == 0) {
        m_currentAlgorithm = m_surfaceEdges;
    }
    else if (strcmp("Surface", ModeName) == 0) {
        m_currentAlgorithm = m_surface;
    }
    else if (strcmp("Wireframe", ModeName) == 0) {
        m_currentAlgorithm = m_wireframe;
    }
    else if (strcmp("Wireframe (surface only)", ModeName) == 0) {
        m_currentAlgorithm = m_wireframeSurface;
    }
    else if (strcmp("Nodes", ModeName) == 0) {
        m_currentAlgorithm = m_points;
    }
    else if (strcmp("Nodes (surface only)", ModeName) == 0) {
        m_currentAlgorithm = m_pointsSurface;
    }

    updateVtk();

    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderFemPostObject::getDisplayModes() const
{
    std::vector<std::string> StrList;
    StrList.emplace_back("Outline");
    StrList.emplace_back("Nodes");
    // StrList.emplace_back("Nodes (surface only)"); somehow this filter does not work
    StrList.emplace_back("Surface");
    StrList.emplace_back("Surface with Edges");
    StrList.emplace_back("Wireframe");
    StrList.emplace_back("Wireframe (surface only)");
    return StrList;
}

void ViewProviderFemPostObject::updateVtk()
{

    if (!setupPipeline()) {
        return;
    }

    m_currentAlgorithm->Update();
    updateProperties();
    update3D();
}

void ViewProviderFemPostObject::updateProperties()
{

    m_blockPropertyChanges = true;
    vtkPolyData* poly = m_currentAlgorithm->GetOutput();

    // coloring
    std::string val;
    if (Field.hasEnums() && Field.getValue() >= 0) {
        val = Field.getValueAsString();
    }

    std::vector<std::string> colorArrays;
    colorArrays.emplace_back("None");

    vtkPointData* point = poly->GetPointData();
    for (int i = 0; i < point->GetNumberOfArrays(); ++i) {
        std::string FieldName = point->GetArrayName(i);
        if (FieldName != "Texture Coordinates") {
            colorArrays.push_back(FieldName);
        }
    }

    vtkCellData* cell = poly->GetCellData();
    for (int i = 0; i < cell->GetNumberOfArrays(); ++i) {
        colorArrays.emplace_back(cell->GetArrayName(i));
    }

    App::Enumeration empty;
    Field.setValue(empty);
    m_coloringEnum.setEnums(colorArrays);
    Field.setValue(m_coloringEnum);

    std::vector<std::string>::iterator it = std::find(colorArrays.begin(), colorArrays.end(), val);
    if (!val.empty() && it != colorArrays.end()) {
        Field.setValue(val.c_str());
    }

    Field.purgeTouched();

    // Vector mode
    if (VectorMode.hasEnums() && VectorMode.getValue() >= 0) {
        val = VectorMode.getValueAsString();
    }

    colorArrays.clear();
    if (Field.getValue() == 0) {
        colorArrays.emplace_back("Not a vector");
    }
    else {
        int array = Field.getValue() - 1;  // 0 is none
        vtkDataArray* data = point->GetArray(array);
        if (!data) {
            return;
        }

        if (data->GetNumberOfComponents() == 1) {
            colorArrays.emplace_back("Not a vector");
        }
        else {
            colorArrays.emplace_back("Magnitude");
            if (data->GetNumberOfComponents() >= 2) {
                colorArrays.emplace_back("X");
                colorArrays.emplace_back("Y");
            }
            if (data->GetNumberOfComponents() >= 3) {
                colorArrays.emplace_back("Z");
            }
        }
    }

    VectorMode.setValue(empty);
    m_vectorEnum.setEnums(colorArrays);
    VectorMode.setValue(m_vectorEnum);

    it = std::find(colorArrays.begin(), colorArrays.end(), val);
    if (!val.empty() && it != colorArrays.end()) {
        VectorMode.setValue(val.c_str());
    }

    m_blockPropertyChanges = false;
}

void ViewProviderFemPostObject::update3D()
{

    vtkPolyData* pd = m_currentAlgorithm->GetOutput();

    vtkPointData* pntData;
    vtkPoints* points;
    vtkDataArray* normals = nullptr;
    vtkDataArray* tcoords = nullptr;
    vtkCellArray* cells;
    vtkIdType npts = 0;
    vtkIdTypePtr indx = nullptr;

    points = pd->GetPoints();
    pntData = pd->GetPointData();
    normals = pntData->GetNormals();
    tcoords = pntData->GetTCoords();

    // write out point data if any
    WritePointData(points, normals, tcoords);
    WriteTransparency();
    bool ResetColorBarRange = false;
    WriteColorData(ResetColorBarRange);

    // write out polys if any
    if (pd->GetNumberOfPolys() > 0) {

        m_faces->coordIndex.startEditing();
        int soidx = 0;
        cells = pd->GetPolys();
        for (cells->InitTraversal(); cells->GetNextCell(npts, indx);) {

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
    else {
        m_faces->coordIndex.setNum(0);
    }

    // write out tstrips if any
    if (pd->GetNumberOfStrips() > 0) {

        int soidx = 0;
        cells = pd->GetStrips();
        m_triangleStrips->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts, indx);) {

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
    else {
        m_triangleStrips->coordIndex.setNum(0);
    }

    // write out lines if any
    if (pd->GetNumberOfLines() > 0) {

        int soidx = 0;
        cells = pd->GetLines();
        m_lines->coordIndex.startEditing();
        for (cells->InitTraversal(); cells->GetNextCell(npts, indx);) {
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
    else {
        m_lines->coordIndex.setNum(0);
    }

    // write out verts if any
    if (pd->GetNumberOfVerts() > 0) {

        int soidx = 0;
        cells = pd->GetVerts();
        m_markers->coordIndex.startEditing();
        m_markers->coordIndex.setNum(pd->GetNumberOfVerts());
        for (cells->InitTraversal(); cells->GetNextCell(npts, indx);) {
            m_markers->coordIndex.set1Value(soidx, static_cast<int>(indx[0]));
            ++soidx;
        }
        m_markers->coordIndex.finishEditing();
    }
    else {
        m_markers->coordIndex.setNum(0);
    }
}

void ViewProviderFemPostObject::WritePointData(vtkPoints* points,
                                               vtkDataArray* normals,
                                               vtkDataArray* tcoords)
{
    Q_UNUSED(tcoords);

    if (!points) {
        return;
    }

    m_coordinates->point.setNum(points->GetNumberOfPoints());
    SbVec3f* pnts = m_coordinates->point.startEditing();
    for (int i = 0; i < points->GetNumberOfPoints(); i++) {
        double* p = points->GetPoint(i);
        pnts[i].setValue(p[0], p[1], p[2]);
    }
    m_coordinates->point.finishEditing();

    // write out the point normal data
    if (normals) {
        m_normals->vector.setNum(normals->GetNumberOfTuples());
        SbVec3f* dirs = m_normals->vector.startEditing();
        for (int i = 0; i < normals->GetNumberOfTuples(); i++) {
            double* p = normals->GetTuple(i);
            dirs[i].setValue(p[0], p[1], p[2]);
        }
        m_normals->vector.finishEditing();

        m_normalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
        m_normalBinding->value.touch();
    }
}

void ViewProviderFemPostObject::setRangeOfColorBar(double min, double max)
{
    try {
        if (min >= max) {
            min = max - 10 * std::numeric_limits<double>::epsilon();
            max = max + 10 * std::numeric_limits<double>::epsilon();
        }
        m_colorBar->setRange(min, max);
    }
    catch (const Base::ValueError& e) {
        e.ReportException();
    }
}

void ViewProviderFemPostObject::updateMaterial()
{
    WriteColorData(true);
}

void ViewProviderFemPostObject::WriteColorData(bool ResetColorBarRange)
{
    if (!setupPipeline()) {
        return;
    }

    if (Field.getEnumVector().empty() || Field.getValue() == 0) {
        m_material->diffuseColor.setValue(SbColor(0.8, 0.8, 0.8));
        float trans = float(Transparency.getValue()) / 100.0;
        m_material->transparency.setValue(trans);
        m_materialBinding->value = SoMaterialBinding::OVERALL;
        m_materialBinding->touch();
        // since there is no field, set the range to the default
        // range as if a new object is created
        setRangeOfColorBar(-0.5, 0.5);
        return;
    };

    int array = Field.getValue() - 1;  // 0 is none
    vtkPolyData* pd = m_currentAlgorithm->GetOutput();
    vtkDataArray* data = pd->GetPointData()->GetArray(array);
    if (!data) {
        return;
    }

    int component = VectorMode.getValue() - 1;  // 0 is either "Not a vector" or magnitude,
                                                // for -1 is correct for magnitude.
                                                // x y and z are one number too high
    if (strcmp(VectorMode.getValueAsString(), "Not a vector") == 0) {
        component = 0;
    }

    // build the lookuptable
    if (ResetColorBarRange) {
        double range[2];
        data->GetRange(range, component);
        setRangeOfColorBar(range[0], range[1]);
    }

    m_material->diffuseColor.setNum(pd->GetNumberOfPoints());
    SbColor* diffcol = m_material->diffuseColor.startEditing();

    float overallTransp = Transparency.getValue() / 100.0f;
    m_material->transparency.setNum(pd->GetNumberOfPoints());
    float* transp = m_material->transparency.startEditing();

    for (int i = 0; i < pd->GetNumberOfPoints(); i++) {

        double value = 0;
        if (component >= 0) {
            value = data->GetComponent(i, component);
        }
        else {
            for (int j = 0; j < data->GetNumberOfComponents(); ++j) {
                value += std::pow(data->GetComponent(i, j), 2);
            }

            value = std::sqrt(value);
        }

        App::Color c = m_colorBar->getColor(value);
        diffcol[i].setValue(c.r, c.g, c.b);
        transp[i] = std::max(c.a, overallTransp);
    }

    m_material->diffuseColor.finishEditing();
    m_material->transparency.finishEditing();
    m_materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;

    // In order to apply the transparency changes the shape nodes must be touched
    m_faces->touch();
    m_triangleStrips->touch();
}

void ViewProviderFemPostObject::WriteTransparency()
{
    float trans = float(Transparency.getValue()) / 100.0;
    m_material->transparency.setValue(trans);

    // In order to apply the transparency changes the shape nodes must be touched
    m_faces->touch();
    m_triangleStrips->touch();
}

void ViewProviderFemPostObject::updateData(const App::Property* p)
{
    Fem::FemPostObject* postObject = static_cast<Fem::FemPostObject*>(getObject());
    if (p == &postObject->Data) {
        updateVtk();
    }
}

void ViewProviderFemPostObject::filterArtifacts(vtkDataSet* dset)
{
    // The problem is that in the surface view the boundary regions of the volumes
    // calculated by the different CPU cores is always visible, independent of the
    // transparency setting. Elmer is not to blame because this is a property of the
    // partial VTK file reader. So this can happen with various inputs
    // since FreeCAD can also be used to view VTK files without the need to perform
    // an analysis. Therefore it is impossible to know in advance when a filter
    // is necessary or not.
    // Only for pure CCX analyses we know that no filtering is necessary. However,
    // the effort to catch this case is not worth it since the filtering is
    // only as time-consuming as enabling the surface filter. In fact, it is like
    // performing the surface filter twice.

    // We need to set the filter clipping plane below the z-minimum of the data.
    // We can either do this by checking the VTK data or by getting the info from
    // the 3D view. We use here the latter because this is much faster.

    // since we will set the filter according to the visible bounding box
    // assure the object is visible
    bool visibility = this->Visibility.getValue();
    if (!visibility) {
        this->Visibility.setValue(true);
    }
    m_blockPropertyChanges = true;

    Gui::Document* doc = this->getDocument();
    Gui::View3DInventor* view =
        qobject_cast<Gui::View3DInventor*>(doc->getViewOfViewProvider(this));

    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        SbBox3f boundingBox;
        boundingBox = viewer->getBoundingBox();
        if (boundingBox.hasVolume()) {
            // setup
            vtkSmartPointer<vtkImplicitFunction> m_implicit;
            auto m_plane = vtkSmartPointer<vtkPlane>::New();
            m_implicit = m_plane;
            m_plane->SetNormal(0., 0., 1.);
            auto extractor = vtkSmartPointer<vtkTableBasedClipDataSet>::New();
            float dx, dy, dz;
            boundingBox.getSize(dx, dy, dz);
            // Set plane below the minimum to assure there are
            // no boundary cells (touching the function) and for Warp filters
            // the user might change the warp factor a lot. Thus set
            // 10 times dz to be safe even for unrealistic warp deformations
            m_plane->SetOrigin(0., 0., -10 * dz);
            extractor->SetClipFunction(m_implicit);
            extractor->SetInputData(dset);
            extractor->Update();
            auto extractorResult = extractor->GetOutputDataObject(0);
            if (extractorResult) {
                m_surface->SetInputData(extractorResult);
            }
            else {
                m_surface->SetInputData(dset);
            }
        }
        else {
            // for the case that there are only 2D objects
            m_surface->SetInputData(dset);
        }
    }
    // restore initial vsibility
    if (!visibility) {
        this->Visibility.setValue(visibility);
    }
    m_blockPropertyChanges = false;
}

bool ViewProviderFemPostObject::setupPipeline()
{
    if (m_blockPropertyChanges) {
        return false;
    }

    auto postObject = static_cast<Fem::FemPostObject*>(getObject());

    vtkDataObject* data = postObject->Data.getValue();
    if (!data) {
        return false;
    }

    // check all fields if there is a real/imaginary one and if so
    // add a field with an absolute value
    vtkSmartPointer<vtkDataObject> SPdata = data;
    vtkDataSet* dset = vtkDataSet::SafeDownCast(SPdata);
    if (!dset) {
        return false;
    }
    std::string FieldName;
    auto numFields = dset->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numFields; ++i) {
        FieldName = std::string(dset->GetPointData()->GetArrayName(i));
        addAbsoluteField(dset, FieldName);
    }

    m_outline->SetInputData(dset);
    m_points->SetInputData(dset);
    m_wireframe->SetInputData(dset);

    // Filtering artifacts is necessary for partial VTU files (*.pvtu) independent of the
    // current Elmer CPU core settings because the user might load an external file.
    // It is only necessary for the surface filter.
    // The problem is that when opening an existing FreeCAD file, we get no information how the
    // Data of the postObject was once created. The vtkDataObject type does not provide this info.
    // Therefore the only way is the hack to filter only if the used Elmer CPU cores are > 1.
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Elmer");
    bool FilterMultiCPUResults = hGrp->GetBool("FilterMultiCPUResults", true);
    int UseNumberOfCores = hGrp->GetInt("UseNumberOfCores", 1);
    // filtering is only necessary for pipelines and warp filters
    if (FilterMultiCPUResults && (UseNumberOfCores > 1)
        && ((postObject->getTypeId() == Base::Type::fromName("Fem::FemPostPipeline"))
            || (postObject->getTypeId() == Base::Type::fromName("Fem::FemPostWarpVectorFilter")))) {
        filterArtifacts(dset);
    }
    else {
        m_surface->SetInputData(dset);
    }

    return true;
}

void ViewProviderFemPostObject::onChanged(const App::Property* prop)
{
    if (m_blockPropertyChanges) {
        return;
    }

    bool ResetColorBarRange;

    // the point filter delivers a single value thus recoloring the bar is senseless
    if (static_cast<Fem::FemPostObject*>(getObject())->getTypeId()
        == Base::Type::fromName("Fem::FemPostDataAtPointFilter")) {
        ResetColorBarRange = false;
    }
    else {
        ResetColorBarRange = true;
    }

    if (prop == &Field && setupPipeline()) {
        updateProperties();
        WriteColorData(ResetColorBarRange);
        WriteTransparency();
    }
    else if (prop == &VectorMode && setupPipeline()) {
        WriteColorData(ResetColorBarRange);
        WriteTransparency();
    }
    else if (prop == &Transparency) {
        WriteTransparency();
    }

    ViewProviderDocumentObject::onChanged(prop);
}

bool ViewProviderFemPostObject::doubleClicked()
{
    // work around for a problem in VTK implementation:
    // https://forum.freecad.org/viewtopic.php?t=10587&start=130#p125688
    // check if backlight is enabled
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    bool isBackLightEnabled = hGrp->GetBool("EnableBacklight", false);
    if (!isBackLightEnabled) {
        Base::Console().Error("Backlight is not enabled. Due to a VTK implementation problem you "
                              "really should consider to enable backlight in FreeCAD display "
                              "preferences if you work with VTK post processing.\n");
    }
    // set edit
    Gui::Application::Instance->activeDocument()->setEdit(this, (int)ViewProvider::Default);
    return true;
}

bool ViewProviderFemPostObject::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default || ModNum == 1) {

        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        TaskDlgPost* postDlg = qobject_cast<TaskDlgPost*>(dlg);
        if (postDlg && postDlg->getView() != this) {
            postDlg = nullptr;  // another pad left open its task panel
        }
        if (dlg && !postDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().reject();
            }
            else {
                return false;
            }
        }

        // start the edit dialog
        if (postDlg) {
            Gui::Control().showDialog(postDlg);
        }
        else {
            postDlg = new TaskDlgPost(this);
            setupTaskDialog(postDlg);
            postDlg->connectSlots();
            Gui::Control().showDialog(postDlg);
        }

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemPostObject::setupTaskDialog(TaskDlgPost* dlg)
{
    dlg->appendBox(new TaskPostDisplay(this));
}

void ViewProviderFemPostObject::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // and update the pad
        // getSketchObject()->getDocument()->recompute();

        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

void ViewProviderFemPostObject::hide()
{
    Gui::ViewProviderDocumentObject::hide();
    m_colorStyle->style = SoDrawStyle::INVISIBLE;
    // The object is now hidden but the color bar is wrong
    // if there are other FemPostObjects visible.
    // We must therefore search for the first visible FemPostObject
    // according to their order in the Tree View (excluding the point
    // object FemPostDataAtPointFilter) and refresh its color bar.

    // get all objects in the document
    auto docGui = Gui::Application::Instance->activeDocument();
    if (!docGui) {
        return;
    }
    auto doc = docGui->getDocument();
    std::vector<App::DocumentObject*> ObjectsList = doc->getObjects();
    App::DocumentObject* firstVisiblePostObject = nullptr;
    // step through the objects
    for (auto it : ObjectsList) {
        if (it->getTypeId().isDerivedFrom(Fem::FemPostObject::getClassTypeId())) {
            if (!firstVisiblePostObject && it->Visibility.getValue()
                && !it->isDerivedFrom(Fem::FemPostDataAtPointFilter::getClassTypeId())) {
                firstVisiblePostObject = it;
                break;
            }
        }
    }
    // refresh found object
    if (firstVisiblePostObject) {
        auto viewProvider = docGui->getViewProvider(firstVisiblePostObject);
        auto FEMviewProvider = static_cast<FemGui::ViewProviderFemPostObject*>(viewProvider);
        if (FEMviewProvider) {
            FEMviewProvider->WriteColorData(true);
        }
    }
}

void ViewProviderFemPostObject::show()
{
    Gui::ViewProviderDocumentObject::show();
    m_colorStyle->style = SoDrawStyle::FILLED;
    // we must update the color bar except for data point filters
    // (for ViewProviderFemPostDataAtPoint show() is overridden to prevent the update)
    WriteColorData(true);
}

void ViewProviderFemPostObject::OnChange(Base::Subject<int>& /*rCaller*/, int /*rcReason*/)
{
    bool ResetColorBarRange = false;
    WriteColorData(ResetColorBarRange);
}

bool ViewProviderFemPostObject::onDelete(const std::vector<std::string>&)
{
    // warn the user if the object has unselected children
    auto objs = claimChildren();
    return ViewProviderFemAnalysis::checkSelectedChildren(objs, this->getDocument(), "pipeline");
}

bool ViewProviderFemPostObject::canDelete(App::DocumentObject* obj) const
{
    // deletions of objects from a FemPostObject don't necessarily destroy anything
    // thus we can pass this action
    // we can warn the user if necessary in the object's ViewProvider in the onDelete() function
    Q_UNUSED(obj)
    return true;
}

void ViewProviderFemPostObject::onSelectionChanged(const Gui::SelectionChanges& sel)
{
    // If a FemPostObject is selected in the document tree we must refresh its
    // color bar.
    // But don't do this if the object is invisible because other objects with a
    // color bar might be visible and the color bar is then wrong.
    if (sel.Type == Gui::SelectionChanges::AddSelection) {
        if (this->getObject()->Visibility.getValue()) {
            updateMaterial();
        }
    }
}

// if there is a real and an imaginary field, an absolute field is added
void ViewProviderFemPostObject::addAbsoluteField(vtkDataSet* dset, std::string FieldName)
{
    // real field names have the suffix " re", given by Elmer
    // if the field does not have this suffix, we can return
    auto suffix = FieldName.substr(FieldName.size() - 3, FieldName.size() - 1);
    if (strcmp(suffix.c_str(), " re") != 0) {
        return;
    }

    // absolute fields might have already been created, then do nothing
    auto strAbsoluteFieldName = FieldName.substr(0, FieldName.size() - 2) + "abs";
    vtkDataArray* testArray = dset->GetPointData()->GetArray(strAbsoluteFieldName.c_str());
    if (testArray) {
        return;
    }

    // safety check
    vtkDataArray* realDdata = dset->GetPointData()->GetArray(FieldName.c_str());
    if (!realDdata) {
        return;
    }

    // now check if the imaginary counterpart exists
    auto strImaginaryFieldName = FieldName.substr(0, FieldName.size() - 2) + "im";
    vtkDataArray* imagDdata = dset->GetPointData()->GetArray(strImaginaryFieldName.c_str());
    if (!imagDdata) {
        return;
    }

    // create a new array and copy over the real data
    // since one cannot directly access the values of a vtkDataSet
    // we need to copy them over in a loop
    vtkSmartPointer<vtkDoubleArray> absoluteData = vtkSmartPointer<vtkDoubleArray>::New();
    absoluteData->SetNumberOfComponents(realDdata->GetNumberOfComponents());
    auto numTuples = realDdata->GetNumberOfTuples();
    absoluteData->SetNumberOfTuples(numTuples);
    double tuple[] = {0, 0, 0};
    for (vtkIdType i = 0; i < numTuples; ++i) {
        absoluteData->SetTuple(i, tuple);
    }
    // name the array
    auto strAbsFieldName = FieldName.substr(0, FieldName.size() - 2) + "abs";
    absoluteData->SetName(strAbsFieldName.c_str());

    // add array to data set
    dset->GetPointData()->AddArray(absoluteData);

    // step through all mesh points and calculate them
    double realValue = 0;
    double imaginaryValue = 0;
    double absoluteValue = 0;
    for (int i = 0; i < dset->GetNumberOfPoints(); ++i) {
        if (absoluteData->GetNumberOfComponents() == 1) {
            realValue = realDdata->GetComponent(i, 0);
            imaginaryValue = imagDdata->GetComponent(i, 0);
            absoluteValue = sqrt(pow(realValue, 2) + pow(imaginaryValue, 2));
            absoluteData->SetComponent(i, 0, absoluteValue);
        }
        // if field is a vector
        else {
            for (int j = 0; j < absoluteData->GetNumberOfComponents(); ++j) {
                realValue = realDdata->GetComponent(i, j);
                imaginaryValue = imagDdata->GetComponent(i, j);
                absoluteValue = sqrt(pow(realValue, 2) + pow(imaginaryValue, 2));
                absoluteData->SetComponent(i, j, absoluteValue);
            }
        }
    }
}
