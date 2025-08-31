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
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedPointSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransparencyType.h>
#include <functional>
#include <limits>


#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkImplicitFunction.h>
#include <vtkPointData.h>

#include <QApplication>
#include <QMessageBox>
#include <QTextStream>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/SoFCColorBar.h>
#include <Gui/SoFCColorBarNotifier.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Fem/App/FemPostFilter.h>

#include "TaskPostBoxes.h"
#ifdef FC_USE_VTK_PYTHON
#include "TaskPostExtraction.h"
#endif
#include "ViewProviderAnalysis.h"
#include "ViewProviderFemPostObject.h"

#include <Base/Tools.h>


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

App::PropertyFloatConstraint::Constraints ViewProviderFemPostObject::sizeRange = {1.0, 64.0, 1.0};

PROPERTY_SOURCE(FemGui::ViewProviderFemPostObject, Gui::ViewProviderDocumentObject)

ViewProviderFemPostObject::ViewProviderFemPostObject()
{
    // initialize the properties
    ADD_PROPERTY_TYPE(Field,
                      ((long)0),
                      "Coloring",
                      App::Prop_None,
                      "Select the field used for calculating the color");
    ADD_PROPERTY_TYPE(Component,
                      ((long)0),
                      "Coloring",
                      App::Prop_None,
                      "Select component to display");
    ADD_PROPERTY_TYPE(Transparency,
                      (0),
                      "Object Style",
                      App::Prop_None,
                      "Set object transparency.");
    ADD_PROPERTY_TYPE(EdgeColor,
                      (0.0f, 0.0f, 0.0f),
                      "Object Style",
                      App::Prop_None,
                      "Set wireframe line color.");
    ADD_PROPERTY_TYPE(PlainColorEdgeOnSurface,
                      (false),
                      "Object Style",
                      App::Prop_None,
                      "Use plain color for edges on surface.");
    ADD_PROPERTY_TYPE(LineWidth, (1), "Object Style", App::Prop_None, "Set wireframe line width.");
    ADD_PROPERTY_TYPE(PointSize, (3), "Object Style", App::Prop_None, "Set node point size.");


    LineWidth.setConstraints(&sizeRange);
    PointSize.setConstraints(&sizeRange);

    sPixmap = "FEM_PostPipelineFromResult";

    // create the subnodes which do the visualization work
    m_transpType = new SoTransparencyType();
    m_transpType->ref();
    m_transpType->value = SoTransparencyType::BLEND;
    m_depthBuffer = new SoDepthBuffer();
    m_depthBuffer->ref();
    m_shapeHints = new SoShapeHints();
    m_shapeHints->ref();
    m_shapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    m_shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    m_coordinates = new SoCoordinate3();
    m_coordinates->ref();
    m_materialBinding = new SoMaterialBinding();
    m_materialBinding->ref();
    m_switchMatEdges = new SoSwitch();
    m_switchMatEdges->ref();
    m_material = new SoMaterial();
    m_material->ref();
    m_matPlainEdges = new SoMaterial();
    m_matPlainEdges->ref();
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
    m_drawStyle->lineWidth.setValue(LineWidth.getValue());
    m_drawStyle->pointSize.setValue(PointSize.getValue());
    m_sepMarkerLine = new SoSeparator();
    m_sepMarkerLine->ref();
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
    Gui::SoFCColorBarNotifier::instance().attach(m_colorBar);
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
    try {
        FemPostObjectSelectionObserver::instance().unregisterFemPostObject(this);
        m_transpType->unref();
        m_depthBuffer->unref();
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
        m_sepMarkerLine->unref();
        m_separator->unref();
        m_material->unref();
        m_matPlainEdges->unref();
        m_switchMatEdges->unref();
        m_colorStyle->unref();
        m_colorRoot->unref();
        deleteColorBar();
    }
    catch (Base::Exception& e) {
        Base::Console().destructorError(
            "ViewProviderFemPostObject",
            "ViewProviderFemPostObject destructor threw an exception: %s\n",
            e.what());
    }
    catch (...) {
        Base::Console().destructorError(
            "ViewProviderFemPostObject",
            "ViewProviderFemPostObject destructor threw an unknown exception");
    }
}

void ViewProviderFemPostObject::deleteColorBar()
{
    Gui::SoFCColorBarNotifier::instance().detach(m_colorBar);
    m_colorBar->Detach(this);
    m_colorBar->unref();
}

void ViewProviderFemPostObject::attach(App::DocumentObject* pcObj)
{
    ViewProviderDocumentObject::attach(pcObj);

    m_switchMatEdges->addChild(m_material);
    m_switchMatEdges->addChild(m_matPlainEdges);
    // marker and line nodes
    m_sepMarkerLine->addChild(m_transpType);
    m_sepMarkerLine->addChild(m_depthBuffer);
    m_sepMarkerLine->addChild(m_drawStyle);
    m_sepMarkerLine->addChild(m_materialBinding);
    m_sepMarkerLine->addChild(m_switchMatEdges);
    m_sepMarkerLine->addChild(m_coordinates);
    m_sepMarkerLine->addChild(m_markers);
    m_sepMarkerLine->addChild(m_lines);

    // face nodes
    SoPolygonOffset* offset = new SoPolygonOffset();
    m_separator->addChild(m_shapeHints);
    m_separator->addChild(m_materialBinding);
    m_separator->addChild(m_material);
    m_separator->addChild(m_coordinates);
    m_separator->addChild(m_sepMarkerLine);
    m_separator->addChild(offset);
    m_separator->addChild(m_faces);

    // Check for an already existing color bar
    Gui::SoFCColorBar* pcBar =
        static_cast<Gui::SoFCColorBar*>(findFrontRootOfType(Gui::SoFCColorBar::getClassTypeId()));
    if (pcBar) {
        // Attach to the foreign color bar and delete our own bar
        pcBar->Attach(this);
        pcBar->ref();
        pcBar->Notify(0);
        deleteColorBar();
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
    StrList.emplace_back("Surface");
    StrList.emplace_back("Surface with Edges");
    StrList.emplace_back("Wireframe");
    StrList.emplace_back("Wireframe (surface only)");
    StrList.emplace_back("Nodes (surface only)");
    return StrList;
}

void ViewProviderFemPostObject::updateVtk()
{

    if (!setupPipeline()) {
        return;
    }

    m_currentAlgorithm->Update();
    if (!isRestoring()) {
        updateProperties();
    }
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

    auto it = std::ranges::find(colorArrays, val);
    if (!val.empty() && it != colorArrays.end()) {
        Field.setValue(val.c_str());
    }

    Field.purgeTouched();

    // Vector mode
    if (Component.hasEnums() && Component.getValue() >= 0) {
        val = Component.getValueAsString();
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
            // scalar
            colorArrays.emplace_back("Not a vector");
        }
        else {
            colorArrays.emplace_back("Magnitude");
            if (data->GetNumberOfComponents() == 2) {
                // 2D vector
                colorArrays.emplace_back("X");
                colorArrays.emplace_back("Y");
            }
            else if (data->GetNumberOfComponents() == 3) {
                // 3D vector
                colorArrays.emplace_back("X");
                colorArrays.emplace_back("Y");
                colorArrays.emplace_back("Z");
            }
            else if (data->GetNumberOfComponents() == 6) {
                // symmetric tensor
                colorArrays.emplace_back("XX");
                colorArrays.emplace_back("YY");
                colorArrays.emplace_back("ZZ");
                colorArrays.emplace_back("XY");
                colorArrays.emplace_back("YZ");
                colorArrays.emplace_back("ZX");
            }
        }
    }

    Component.setValue(empty);
    m_vectorEnum.setEnums(colorArrays);
    Component.setValue(m_vectorEnum);

    it = std::ranges::find(colorArrays, val);
    if (!val.empty() && it != colorArrays.end()) {
        Component.setValue(val.c_str());
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

void ViewProviderFemPostObject::setRangeOfColorBar(float min, float max)
{
    try {
        // setRange expects max value greater than min value.
        // A typical case is max equal to min, so machine epsilon
        // is used to overwrite and differentiate both values
        if (min >= max) {
            static constexpr float eps = std::numeric_limits<float>::epsilon();
            if (max > 0) {
                min = max * (1 - eps);
                max = max * (1 + eps);
            }
            else if (max < 0) {
                min = max * (1 + eps);
                max = max * (1 - eps);
            }
            else {
                static constexpr float minF = std::numeric_limits<float>::min();
                min = -1 * minF;
                max = minF;
            }
        }
        m_colorBar->setRange(min, max);
    }
    catch (const Base::ValueError& e) {
        e.reportException();
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
        float trans = Base::fromPercent(Transparency.getValue());
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

    int component = Component.getValue() - 1;  // 0 is either "Not a vector" or magnitude,
                                               // for -1 is correct for magnitude.
                                               // x y and z are one number too high
    if (strcmp(Component.getValueAsString(), "Not a vector") == 0) {
        component = 0;
    }

    // build the lookuptable
    if (ResetColorBarRange) {
        double range[2];
        data->GetRange(range, component);
        setRangeOfColorBar(static_cast<float>(range[0]), static_cast<float>(range[1]));
    }

    vtkIdType numPts = pd->GetNumberOfPoints();
    m_material->diffuseColor.setNum(numPts);
    m_matPlainEdges->diffuseColor.setNum(numPts);
    SbColor* diffcol = m_material->diffuseColor.startEditing();
    SbColor* edgeDiffcol = m_matPlainEdges->diffuseColor.startEditing();

    float overallTransp = Base::fromPercent(Transparency.getValue());
    m_material->transparency.setNum(numPts);
    m_matPlainEdges->transparency.setNum(numPts);
    float* transp = m_material->transparency.startEditing();
    float* edgeTransp = m_matPlainEdges->transparency.startEditing();
    Base::Color c;
    Base::Color cEdge = EdgeColor.getValue();
    for (int i = 0; i < numPts; i++) {

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

        c = m_colorBar->getColor(value);
        diffcol[i].setValue(c.r, c.g, c.b);
        transp[i] = std::max(c.transparency(), overallTransp);
        edgeDiffcol[i].setValue(cEdge.r, cEdge.g, cEdge.b);
        edgeTransp[i] = std::max(cEdge.transparency(), overallTransp);
    }

    m_material->diffuseColor.finishEditing();
    m_material->transparency.finishEditing();
    m_matPlainEdges->diffuseColor.finishEditing();
    m_matPlainEdges->transparency.finishEditing();
    m_materialBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;

    // In order to apply the transparency changes the shape nodes must be touched
    m_faces->touch();
    m_triangleStrips->touch();
}

void ViewProviderFemPostObject::WriteTransparency()
{
    float trans = Base::fromPercent(Transparency.getValue());
    float* value = m_material->transparency.startEditing();
    float* edgeValue = m_matPlainEdges->transparency.startEditing();
    // m_material and m_matPlainEdges field containers have same size
    for (int i = 0; i < m_material->transparency.getNum(); ++i) {
        value[i] = trans;
        edgeValue[i] = trans;
    }
    m_material->transparency.finishEditing();
    m_matPlainEdges->transparency.finishEditing();

    if (Transparency.getValue() > 99) {
        m_depthBuffer->test.setValue(false);
    }
    else {
        m_depthBuffer->test.setValue(true);
    }
    // In order to apply the transparency changes the shape nodes must be touched
    m_faces->touch();
    m_triangleStrips->touch();
}

void ViewProviderFemPostObject::updateData(const App::Property* p)
{
    Fem::FemPostObject* postObject = getObject<Fem::FemPostObject>();
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

    m_blockPropertyChanges = false;

    // restore initial vsibility
    if (!visibility) {
        this->Visibility.setValue(visibility);
    }
}

bool ViewProviderFemPostObject::setupPipeline()
{
    if (m_blockPropertyChanges) {
        return false;
    }

    auto postObject = getObject<Fem::FemPostObject>();

    // check all fields if there is a real/imaginary one and if so
    // add a field with an absolute value
    vtkDataSet* dset = postObject->getDataSet();
    if (!dset) {
        return false;
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
    if (getObject<Fem::FemPostObject>()->getTypeId()
        == Base::Type::fromName("Fem::FemPostDataAtPointFilter")) {
        ResetColorBarRange = false;
    }
    else {
        ResetColorBarRange = true;
    }

    if (prop == &Field && setupPipeline()) {
        if (!isRestoring()) {
            updateProperties();
        }
        WriteColorData(ResetColorBarRange);
    }
    else if (prop == &Component && setupPipeline()) {
        WriteColorData(ResetColorBarRange);
    }
    else if (prop == &Transparency) {
        WriteTransparency();
    }
    else if (prop == &LineWidth) {
        m_drawStyle->lineWidth.setValue(LineWidth.getValue());
    }
    else if (prop == &PointSize) {
        m_drawStyle->pointSize.setValue(PointSize.getValue());
    }
    else if (prop == &EdgeColor && setupPipeline()) {
        Base::Color c = EdgeColor.getValue();
        SbColor* edgeColor = m_matPlainEdges->diffuseColor.startEditing();
        for (int i = 0; i < m_matPlainEdges->diffuseColor.getNum(); ++i) {
            edgeColor[i].setValue(c.r, c.g, c.b);
        }
        m_matPlainEdges->diffuseColor.finishEditing();
    }
    else if (prop == &PlainColorEdgeOnSurface || prop == &DisplayMode) {
        bool plainColor = PlainColorEdgeOnSurface.getValue()
            && (strcmp("Surface with Edges", DisplayMode.getValueAsString()) == 0);
        int child = plainColor ? 1 : 0;
        m_switchMatEdges->whichChild.setValue(child);
    }

    ViewProviderDocumentObject::onChanged(prop);
}

bool ViewProviderFemPostObject::doubleClicked()
{
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
            QMessageBox msgBox(Gui::getMainWindow());
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
            postDlg->processCollapsedWidgets();
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
    assert(dlg->getView() == this);
    auto dispPanel = new TaskPostDisplay(this);
    dlg->addTaskBox(dispPanel->windowIcon().pixmap(32), dispPanel);

#ifdef FC_USE_VTK_PYTHON
    auto extrPanel = new TaskPostExtraction(this);
    dlg->addTaskBox(extrPanel->windowIcon().pixmap(32), extrPanel);
#endif
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
        if (it->isDerivedFrom<Fem::FemPostObject>()) {
            if (!firstVisiblePostObject && it->Visibility.getValue()
                && !it->isDerivedFrom<Fem::FemPostDataAtPointFilter>()) {
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
    if (!ViewProviderFemAnalysis::checkSelectedChildren(objs, this->getDocument(), "pipeline")) {
        return false;
    };

    // delete all subelements
    for (auto obj : objs) {
        getObject()->getDocument()->removeObject(obj->getNameInDocument());
    }
    return true;
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

void ViewProviderFemPostObject::handleChangedPropertyName(Base::XMLReader& reader,
                                                          const char* typeName,
                                                          const char* propName)
{
    if (strcmp(propName, "Field") == 0 && strcmp(typeName, "App::PropertyEnumeration") == 0) {
        App::PropertyEnumeration field;
        field.Restore(reader);
        Component.setValue(field.getValue());
    }
    else {
        Gui::ViewProviderDocumentObject::handleChangedPropertyName(reader, typeName, propName);
    }
}
