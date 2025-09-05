/***************************************************************************
 *   Copyright (c) 2019 Manuel Apeltauer, direkt cnc-systeme GmbH          *
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
#include <limits>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepProj_Projection.hxx>
#include <gp_Ax1.hxx>
#include <ShapeAnalysis.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeFix_Wireframe.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#endif

#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Application.h>
#include <Gui/Selection/SelectionObject.h>
#include <Inventor/SbVec3d.h>

#include "DlgProjectionOnSurface.h"
#include "ui_DlgProjectionOnSurface.h"
#include "ViewProviderExt.h"


using namespace PartGui;

namespace {
//////////////////////////////////////////////////////////////////////////
class EdgeSelection: public Gui::SelectionFilterGate
{
public:
    bool canSelect = false;

    EdgeSelection()
        : Gui::SelectionFilterGate(nullPointer())
    {}

    bool allow(App::Document* /*pDoc*/, App::DocumentObject* iPObj, const char* sSubName) override
    {
        auto aPart = dynamic_cast<Part::Feature*>(iPObj);
        if (!aPart) {
            return false;
        }
        if (!sSubName) {
            return false;
        }
        std::string subName(sSubName);
        if (subName.empty()) {
            return false;
        }

        auto subShape = aPart->Shape.getShape().getSubShape(sSubName);
        if (subShape.IsNull()) {
            return false;
        }
        auto type = subShape.ShapeType();
        return (type == TopAbs_EDGE);
    }
};
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class FaceSelection: public Gui::SelectionFilterGate
{
public:
    bool canSelect = false;

    FaceSelection()
        : Gui::SelectionFilterGate(nullPointer())
    {}

    bool allow(App::Document* /*pDoc*/, App::DocumentObject* iPObj, const char* sSubName) override
    {
        auto aPart = dynamic_cast<Part::Feature*>(iPObj);
        if (!aPart) {
            return false;
        }
        if (!sSubName) {
            return false;
        }
        std::string subName(sSubName);
        if (subName.empty()) {
            return false;
        }

        auto subShape = aPart->Shape.getShape().getSubShape(sSubName, true);
        if (subShape.IsNull()) {
            return false;
        }
        auto type = subShape.ShapeType();
        return (type == TopAbs_FACE);
    }
};
//////////////////////////////////////////////////////////////////////////
}

DlgProjectionOnSurface::DlgProjectionOnSurface(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DlgProjectionOnSurface)
    , m_projectionObjectName(tr("Projection object"))
    , filterEdge(nullptr)
    , filterFace(nullptr)
{
    ui->setupUi(this);
    setupConnections();

    ui->pushButtonAddEdge->setCheckable(true);
    ui->pushButtonAddFace->setCheckable(true);
    ui->pushButtonAddProjFace->setCheckable(true);
    ui->pushButtonAddWire->setCheckable(true);

    m_guiObjectVec.push_back(ui->pushButtonAddEdge);
    m_guiObjectVec.push_back(ui->pushButtonAddFace);
    m_guiObjectVec.push_back(ui->pushButtonAddProjFace);
    m_guiObjectVec.push_back(ui->pushButtonDirX);
    m_guiObjectVec.push_back(ui->pushButtonDirY);
    m_guiObjectVec.push_back(ui->pushButtonDirZ);
    m_guiObjectVec.push_back(ui->pushButtonGetCurrentCamDir);
    m_guiObjectVec.push_back(ui->radioButtonShowAll);
    m_guiObjectVec.push_back(ui->radioButtonFaces);
    m_guiObjectVec.push_back(ui->radioButtonEdges);
    m_guiObjectVec.push_back(ui->pushButtonAddWire);

    get_camera_direction();
    disable_ui_elements(m_guiObjectVec, ui->pushButtonAddProjFace);

    m_partDocument = App::GetApplication().getActiveDocument();
    if (!m_partDocument) {
        throw Base::ValueError(tr("No active document").toStdString());
    }
    this->attachDocument(m_partDocument);
    m_partDocument->openTransaction("Project on surface");
    m_projectionObject = m_partDocument->addObject<Part::Feature>("Projection Object");
    if (!m_projectionObject) {
        throw Base::ValueError(tr("Cannot create a projection object").toStdString());
    }
    m_projectionObject->Label.setValue(std::string(m_projectionObjectName.toUtf8()).c_str());
    onRadioButtonShowAllClicked();
    m_lastDepthVal = ui->doubleSpinBoxSolidDepth->value();
}

DlgProjectionOnSurface::~DlgProjectionOnSurface()
{
    delete ui;
    for (const auto& it : m_projectionSurfaceVec) {
        try {
            higlight_object(it.partFeature, it.partName, false, 0);
        }
        catch (Standard_NoSuchObject& e) {
            Base::Console().warning("DlgProjectionOnSurface::~DlgProjectionOnSurface: %s",
                                    e.GetMessageString());
        }
        auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
            Gui::Application::Instance->getViewProvider(it.partFeature));
        if (vp) {
            vp->Selectable.setValue(it.is_selectable);
            vp->Transparency.setValue(it.transparency);
        }
    }
    for (const auto& it : m_shapeVec) {
        try {
            higlight_object(it.partFeature, it.partName, false, 0);
        }
        catch (Standard_NoSuchObject& e) {
            Base::Console().warning("DlgProjectionOnSurface::~DlgProjectionOnSurface: %s",
                                    e.GetMessageString());
        }
    }
    Gui::Selection().rmvSelectionGate();
}

void PartGui::DlgProjectionOnSurface::setupConnections()
{
    // clang-format off
    connect(ui->pushButtonAddFace,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonAddFaceClicked);
    connect(ui->pushButtonAddEdge,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonAddEdgeClicked);
    connect(ui->pushButtonGetCurrentCamDir,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonGetCurrentCamDirClicked);
    connect(ui->pushButtonDirX,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonDirXClicked);
    connect(ui->pushButtonDirY,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonDirYClicked);
    connect(ui->pushButtonDirZ,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonDirZClicked);
    connect(ui->pushButtonAddProjFace,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonAddProjFaceClicked);
    connect(ui->radioButtonShowAll,
            &QRadioButton::clicked,
            this,
            &DlgProjectionOnSurface::onRadioButtonShowAllClicked);
    connect(ui->radioButtonFaces,
            &QRadioButton::clicked,
            this,
            &DlgProjectionOnSurface::onRadioButtonFacesClicked);
    connect(ui->radioButtonEdges,
            &QRadioButton::clicked,
            this,
            &DlgProjectionOnSurface::onRadioButtonEdgesClicked);
    connect(ui->doubleSpinBoxExtrudeHeight,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &DlgProjectionOnSurface::onDoubleSpinBoxExtrudeHeightValueChanged);
    connect(ui->pushButtonAddWire,
            &QPushButton::clicked,
            this,
            &DlgProjectionOnSurface::onPushButtonAddWireClicked);
    connect(ui->doubleSpinBoxSolidDepth,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &DlgProjectionOnSurface::onDoubleSpinBoxSolidDepthValueChanged);
    // clang-format on
}

void PartGui::DlgProjectionOnSurface::slotDeletedDocument(const App::Document& Doc)
{
    if (m_partDocument == &Doc) {
        m_partDocument = nullptr;
        m_projectionObject = nullptr;
    }
}

void PartGui::DlgProjectionOnSurface::slotDeletedObject(const App::DocumentObject& Obj)
{
    if (m_projectionObject == &Obj) {
        m_projectionObject = nullptr;
    }
}

void PartGui::DlgProjectionOnSurface::apply()
{
    if (m_partDocument) {
        m_partDocument->commitTransaction();
    }
}

void PartGui::DlgProjectionOnSurface::reject()
{
    if (m_partDocument) {
        m_partDocument->abortTransaction();
    }
}

void PartGui::DlgProjectionOnSurface::onPushButtonAddFaceClicked()
{
    if (ui->pushButtonAddFace->isChecked()) {
        m_currentSelection = "add_face";
        disable_ui_elements(m_guiObjectVec, ui->pushButtonAddFace);
        if (!filterFace) {
            filterFace = new FaceSelection();
            Gui::Selection().addSelectionGate(filterFace);
        }
    }
    else {
        m_currentSelection = "";
        enable_ui_elements(m_guiObjectVec, nullptr);
        Gui::Selection().rmvSelectionGate();
        filterFace = nullptr;
    }
}

void PartGui::DlgProjectionOnSurface::onPushButtonAddEdgeClicked()
{
    if (ui->pushButtonAddEdge->isChecked()) {
        m_currentSelection = "add_edge";
        disable_ui_elements(m_guiObjectVec, ui->pushButtonAddEdge);
        if (!filterEdge) {
            filterEdge = new EdgeSelection();
            Gui::Selection().addSelectionGate(filterEdge);
        }
        ui->radioButtonEdges->setChecked(true);
        onRadioButtonEdgesClicked();
    }
    else {
        m_currentSelection = "";
        enable_ui_elements(m_guiObjectVec, nullptr);
        Gui::Selection().rmvSelectionGate();
        filterEdge = nullptr;
    }
}

void PartGui::DlgProjectionOnSurface::onPushButtonGetCurrentCamDirClicked()
{
    get_camera_direction();
}

void PartGui::DlgProjectionOnSurface::onPushButtonDirXClicked()
{
    set_xyz_dir_spinbox(ui->doubleSpinBoxDirX);
}

void PartGui::DlgProjectionOnSurface::onPushButtonDirYClicked()
{
    set_xyz_dir_spinbox(ui->doubleSpinBoxDirY);
}

void PartGui::DlgProjectionOnSurface::onPushButtonDirZClicked()
{
    set_xyz_dir_spinbox(ui->doubleSpinBoxDirZ);
}

void PartGui::DlgProjectionOnSurface::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (m_currentSelection == "add_face" || m_currentSelection == "add_edge"
            || m_currentSelection == "add_wire") {
            store_current_selected_parts(m_shapeVec, 0xff00ff00);
            create_projection_wire(m_shapeVec);
            create_projection_face_from_wire(m_shapeVec);
            create_face_extrude(m_shapeVec);
            show_projected_shapes(m_shapeVec);
        }
        else if (m_currentSelection == "add_projection_surface") {
            m_projectionSurfaceVec.clear();
            store_current_selected_parts(m_projectionSurfaceVec, 0xffff0000);
            if (!m_projectionSurfaceVec.empty()) {
                auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                    Gui::Application::Instance->getViewProvider(
                        m_projectionSurfaceVec.back().partFeature));
                if (vp) {
                    vp->Selectable.setValue(false);
                    vp->Transparency.setValue(90);
                }
            }

            ui->pushButtonAddProjFace->setChecked(false);
            onPushButtonAddProjFaceClicked();
        }
    }
}

void PartGui::DlgProjectionOnSurface::get_camera_direction()
{
    auto mainWindow = Gui::getMainWindow();

    auto mdiObject = dynamic_cast<Gui::View3DInventor*>(mainWindow->activeWindow());
    if (!mdiObject) {
        return;
    }
    auto camerRotation = mdiObject->getViewer()->getCameraOrientation();

    SbVec3f lookAt(0, 0, -1);
    camerRotation.multVec(lookAt, lookAt);

    float valX {};
    float valY {};
    float valZ {};
    lookAt.getValue(valX, valY, valZ);

    ui->doubleSpinBoxDirX->setValue(valX);
    ui->doubleSpinBoxDirY->setValue(valY);
    ui->doubleSpinBoxDirZ->setValue(valZ);
}

void PartGui::DlgProjectionOnSurface::store_current_selected_parts(
    std::vector<SShapeStore>& iStoreVec,
    unsigned int iColor)
{
    if (!m_partDocument) {
        return;
    }
    std::vector<Gui::SelectionObject> selObj = Gui::Selection().getSelectionEx();
    if (!selObj.empty()) {
        for (auto it = selObj.begin(); it != selObj.end(); ++it) {
            auto aPart = it->getObject<Part::Feature>();
            if (!aPart) {
                continue;
            }

            if (aPart) {
                SShapeStore currentShapeStore;
                currentShapeStore.inputShape = aPart->Shape.getShape().getShape();
                currentShapeStore.partFeature = aPart;
                currentShapeStore.partName = aPart->getNameInDocument();

                auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
                    Gui::Application::Instance->getViewProvider(aPart));
                if (vp) {
                    currentShapeStore.is_selectable = vp->Selectable.getValue();
                    currentShapeStore.transparency = vp->Transparency.getValue();
                }
                if (!it->getSubNames().empty()) {
                    auto parentShape = currentShapeStore.inputShape;
                    for (const auto& itName : selObj.front().getSubNames()) {
                        auto currentShape = aPart->Shape.getShape().getSubShape(itName.c_str());

                        transform_shape_to_global_position(currentShape, aPart);

                        currentShapeStore.inputShape = currentShape;
                        currentShapeStore.partName = itName;
                        auto store = store_part_in_vector(currentShapeStore, iStoreVec);
                        higlight_object(aPart, itName, store, iColor);
                        store_wire_in_vector(currentShapeStore, parentShape, iStoreVec, iColor);
                    }
                }
                else {
                    transform_shape_to_global_position(currentShapeStore.inputShape,
                                                       currentShapeStore.partFeature);
                    auto store = store_part_in_vector(currentShapeStore, iStoreVec);
                    higlight_object(aPart, aPart->Shape.getName(), store, iColor);
                }
                Gui::Selection().clearSelection(m_partDocument->getName());
                Gui::Selection().rmvPreselect();
            }
        }
    }
}

bool PartGui::DlgProjectionOnSurface::store_part_in_vector(SShapeStore& iCurrentShape,
                                                           std::vector<SShapeStore>& iStoreVec)
{
    if (iCurrentShape.inputShape.IsNull()) {
        return false;
    }
    auto currentType = iCurrentShape.inputShape.ShapeType();
    for (auto it = iStoreVec.begin(); it != iStoreVec.end(); ++it) {
        if (currentType == TopAbs_FACE) {
            if (it->aFace.IsSame(iCurrentShape.inputShape)) {
                iStoreVec.erase(it);
                return false;
            }
        }
        else if (currentType == TopAbs_EDGE) {
            if (it->aEdge.IsSame(iCurrentShape.inputShape)) {
                iStoreVec.erase(it);
                return false;
            }
        }
    }

    if (currentType == TopAbs_FACE) {
        iCurrentShape.aFace = TopoDS::Face(iCurrentShape.inputShape);
    }
    else if (currentType == TopAbs_EDGE) {
        iCurrentShape.aEdge = TopoDS::Edge(iCurrentShape.inputShape);
    }

    auto valX = ui->doubleSpinBoxDirX->value();
    auto valY = ui->doubleSpinBoxDirY->value();
    auto valZ = ui->doubleSpinBoxDirZ->value();

    iCurrentShape.aProjectionDir = gp_Dir(valX, valY, valZ);
    if (!m_projectionSurfaceVec.empty()) {
        iCurrentShape.surfaceToProject = m_projectionSurfaceVec.front().aFace;
    }
    iStoreVec.push_back(iCurrentShape);
    return true;
}

void PartGui::DlgProjectionOnSurface::create_projection_wire(
    std::vector<SShapeStore>& iCurrentShape)
{
    try {
        if (iCurrentShape.empty()) {
            return;
        }
        for (auto& itCurrentShape : iCurrentShape) {
            if (m_projectionSurfaceVec.empty()) {
                continue;
            };
            if (!itCurrentShape.aProjectedEdgeVec.empty()) {
                continue;
            };
            if (!itCurrentShape.aProjectedFace.IsNull()) {
                continue;
            };
            if (!itCurrentShape.aProjectedWireVec.empty()) {
                continue;
            };

            if (!itCurrentShape.aFace.IsNull()) {
                get_all_wire_from_face(itCurrentShape);
                for (const auto& itWire : itCurrentShape.aWireVec) {
                    BRepProj_Projection aProjection(itWire,
                                                    itCurrentShape.surfaceToProject,
                                                    itCurrentShape.aProjectionDir);
                    double minDistance = std::numeric_limits<double>::max();
                    TopoDS_Wire wireToTake;
                    for (; aProjection.More(); aProjection.Next()) {
                        auto it = aProjection.Current();
                        BRepExtrema_DistShapeShape distanceMeasure(it, itCurrentShape.aFace);
                        distanceMeasure.Perform();
                        auto currentDistance = distanceMeasure.Value();
                        if (currentDistance > minDistance) {
                            continue;
                        }
                        wireToTake = it;
                        minDistance = currentDistance;
                    }
                    auto aWire = sort_and_heal_wire(wireToTake, itCurrentShape.surfaceToProject);
                    itCurrentShape.aProjectedWireVec.push_back(aWire);
                }
            }
            else if (!itCurrentShape.aEdge.IsNull()) {
                BRepProj_Projection aProjection(itCurrentShape.aEdge,
                                                itCurrentShape.surfaceToProject,
                                                itCurrentShape.aProjectionDir);
                double minDistance = std::numeric_limits<double>::max();
                TopoDS_Wire wireToTake;
                for (; aProjection.More(); aProjection.Next()) {
                    auto it = aProjection.Current();
                    BRepExtrema_DistShapeShape distanceMeasure(it, itCurrentShape.aEdge);
                    distanceMeasure.Perform();
                    auto currentDistance = distanceMeasure.Value();
                    if (currentDistance > minDistance) {
                        continue;
                    }
                    wireToTake = it;
                    minDistance = currentDistance;
                }
                for (TopExp_Explorer aExplorer(wireToTake, TopAbs_EDGE); aExplorer.More();
                     aExplorer.Next()) {
                    itCurrentShape.aProjectedEdgeVec.push_back(TopoDS::Edge(aExplorer.Current()));
                }
            }
        }
    }
    catch (const Standard_Failure& error) {
        std::stringstream ssOcc;
        error.Print(ssOcc);
        throw Base::ValueError(ssOcc.str().c_str());
    }
}

TopoDS_Shape
PartGui::DlgProjectionOnSurface::create_compound(const std::vector<SShapeStore>& iShapeVec)
{
    if (iShapeVec.empty()) {
        return {};
    }

    TopoDS_Compound aCompound;
    TopoDS_Builder aBuilder;
    aBuilder.MakeCompound(aCompound);

    for (const auto& it : iShapeVec) {
        if (m_currentShowType == "edges") {
            for (const auto& it2 : it.aProjectedEdgeVec) {
                aBuilder.Add(aCompound, it2);
            }
            for (const auto& it2 : it.aProjectedWireVec) {
                aBuilder.Add(aCompound, it2);
            }
        }
        else if (m_currentShowType == "faces") {
            if (it.aProjectedFace.IsNull()) {
                for (const auto& it2 : it.aProjectedWireVec) {
                    if (!it2.IsNull()) {
                        aBuilder.Add(aCompound, it2);
                    }
                }
            }
            else {
                aBuilder.Add(aCompound, it.aProjectedFace);
            }
        }
        else if (m_currentShowType == "all") {
            if (!it.aProjectedSolid.IsNull()) {
                aBuilder.Add(aCompound, it.aProjectedSolid);
            }
            else if (!it.aProjectedFace.IsNull()) {
                aBuilder.Add(aCompound, it.aProjectedFace);
            }
            else if (!it.aProjectedWireVec.empty()) {
                for (const auto& itWire : it.aProjectedWireVec) {
                    if (itWire.IsNull()) {
                        continue;
                    }
                    aBuilder.Add(aCompound, itWire);
                }
            }
            else if (!it.aProjectedEdgeVec.empty()) {
                for (const auto& itEdge : it.aProjectedEdgeVec) {
                    if (itEdge.IsNull()) {
                        continue;
                    }
                    aBuilder.Add(aCompound, itEdge);
                }
            }
        }
    }
    return {std::move(aCompound)};
}

void PartGui::DlgProjectionOnSurface::show_projected_shapes(
    const std::vector<SShapeStore>& iShapeStoreVec)
{
    if (!m_projectionObject) {
        return;
    }
    auto aCompound = create_compound(iShapeStoreVec);
    if (aCompound.IsNull()) {
        if (!m_partDocument) {
            return;
        }
        m_projectionObject->Shape.setValue(TopoDS_Shape());
        return;
    }
    auto currentPlacement = m_projectionObject->Placement.getValue();
    m_projectionObject->Shape.setValue(aCompound);
    m_projectionObject->Placement.setValue(currentPlacement);

    // set color
    auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
        Gui::Application::Instance->getViewProvider(m_projectionObject));
    if (vp) {
        const unsigned int color = 0x8ae23400;
        vp->LineColor.setValue(color);
        vp->ShapeAppearance.setDiffuseColor(Base::Color(color));
        vp->PointColor.setValue(color);
        vp->Transparency.setValue(0);
    }
}

void PartGui::DlgProjectionOnSurface::disable_ui_elements(const std::vector<QWidget*>& iObjectVec,
                                                          QWidget* iExceptThis)
{
    for (auto it : iObjectVec) {
        if (!it) {
            continue;
        }
        if (it == iExceptThis) {
            continue;
        }
        it->setDisabled(true);
    }
}

void PartGui::DlgProjectionOnSurface::enable_ui_elements(const std::vector<QWidget*>& iObjectVec,
                                                         QWidget* iExceptThis)
{
    for (auto it : iObjectVec) {
        if (!it) {
            continue;
        }
        if (it == iExceptThis) {
            continue;
        }
        it->setEnabled(true);
    }
}

void PartGui::DlgProjectionOnSurface::higlight_object(Part::Feature* iCurrentObject,
                                                      const std::string& iShapeName,
                                                      bool iHighlight,
                                                      unsigned int iColor)
{
    if (!iCurrentObject) {
        return;
    }
    auto partenShape = iCurrentObject->Shape.getShape().getShape();
    auto subShape = iCurrentObject->Shape.getShape().getSubShape(iShapeName.c_str(), true);

    TopoDS_Shape currentShape = subShape;
    if (subShape.IsNull()) {
        currentShape = partenShape;
    }

    auto currentShapeType = currentShape.ShapeType();
    TopTools_IndexedMapOfShape anIndices;
    TopExp::MapShapes(partenShape, currentShapeType, anIndices);
    if (anIndices.IsEmpty()) {
        return;
    }
    if (!anIndices.Contains(currentShape)) {
        return;
    }
    auto index = anIndices.FindIndex(currentShape);

    // set color
    auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
        Gui::Application::Instance->getViewProvider(iCurrentObject));
    if (vp) {
        std::vector<Base::Color> colors;
        Base::Color defaultColor;
        if (currentShapeType == TopAbs_FACE) {
            colors = vp->ShapeAppearance.getDiffuseColors();
            defaultColor = colors.front();
        }
        else if (currentShapeType == TopAbs_EDGE) {
            colors = vp->LineColorArray.getValues();
            defaultColor = vp->LineColor.getValue();
        }

        if (static_cast<Standard_Integer>(colors.size()) != anIndices.Extent()) {
            colors.resize(anIndices.Extent(), defaultColor);
        }

        if (iHighlight) {
            Base::Color aColor;
            aColor.setPackedValue(iColor);
            colors.at(index - 1) = aColor;
        }
        else {
            colors.at(index - 1) = defaultColor;
        }
        if (currentShapeType == TopAbs_FACE) {
            vp->ShapeAppearance.setDiffuseColors(colors);
        }
        else if (currentShapeType == TopAbs_EDGE) {
            vp->LineColorArray.setValues(colors);
        }
    }
}

void PartGui::DlgProjectionOnSurface::get_all_wire_from_face(SShapeStore& ioCurrentSahpe)
{
    auto outerWire = ShapeAnalysis::OuterWire(ioCurrentSahpe.aFace);
    ioCurrentSahpe.aWireVec.push_back(outerWire);
    for (TopExp_Explorer aExplorer(ioCurrentSahpe.aFace, TopAbs_WIRE); aExplorer.More();
         aExplorer.Next()) {
        auto currentWire = TopoDS::Wire(aExplorer.Current());
        if (currentWire.IsSame(outerWire)) {
            continue;
        }
        ioCurrentSahpe.aWireVec.push_back(currentWire);
    }
}

void PartGui::DlgProjectionOnSurface::create_projection_face_from_wire(
    std::vector<SShapeStore>& iCurrentShape)
{
    try {
        if (iCurrentShape.empty()) {
            return;
        }

        for (auto& itCurrentShape : iCurrentShape) {
            if (itCurrentShape.aFace.IsNull()) {
                continue;
            };
            if (itCurrentShape.aProjectedWireVec.empty()) {
                continue;
            };
            if (!itCurrentShape.aProjectedFace.IsNull()) {
                continue;
            };

            auto surface = BRep_Tool::Surface(itCurrentShape.surfaceToProject);

            // create a wire of all edges in parametric space on the surface of the face to
            // projected
            //  --> otherwise BRepBuilderAPI_MakeFace can not make a face from the wire!
            for (const auto& itWireVec : itCurrentShape.aProjectedWireVec) {
                std::vector<TopoDS_Shape> edgeVec;
                for (TopExp_Explorer aExplorer(itWireVec, TopAbs_EDGE); aExplorer.More();
                     aExplorer.Next()) {
                    auto currentEdge = TopoDS::Edge(aExplorer.Current());
                    edgeVec.push_back(currentEdge);
                }
                if (edgeVec.empty()) {
                    continue;
                }

                std::vector<TopoDS_Edge> edgeInParametricSpaceVec;
                for (auto itEdge : edgeVec) {
                    Standard_Real first {};
                    Standard_Real last {};
                    auto currentCurve = BRep_Tool::CurveOnSurface(TopoDS::Edge(itEdge),
                                                                  itCurrentShape.surfaceToProject,
                                                                  first,
                                                                  last);
                    if (!currentCurve) {
                        continue;
                    }
                    auto edgeInParametricSpace =
                        BRepBuilderAPI_MakeEdge(currentCurve, surface, first, last).Edge();
                    edgeInParametricSpaceVec.push_back(edgeInParametricSpace);
                }
                auto aWire =
                    sort_and_heal_wire(edgeInParametricSpaceVec, itCurrentShape.surfaceToProject);
                itCurrentShape.aProjectedWireInParametricSpaceVec.push_back(aWire);
            }

            // try to create a face from the wires
            // the first wire is the otherwise
            // the following wires are the inside wires
            BRepBuilderAPI_MakeFace faceMaker;
            bool first = true;
            for (auto itWireVec : itCurrentShape.aProjectedWireInParametricSpaceVec) {
                if (first) {
                    first = false;
                    // change the wire direction, otherwise no face is created
                    auto currentWire = TopoDS::Wire(itWireVec.Reversed());
                    if (itCurrentShape.surfaceToProject.Orientation() == TopAbs_REVERSED) {
                        currentWire = itWireVec;
                    }
                    faceMaker = BRepBuilderAPI_MakeFace(surface, currentWire);
                    ShapeFix_Face fix(faceMaker.Face());
                    fix.Perform();
                    auto aFace = fix.Face();
                    BRepCheck_Analyzer aChecker(aFace);
                    if (!aChecker.IsValid()) {
                        faceMaker =
                            BRepBuilderAPI_MakeFace(surface, TopoDS::Wire(currentWire.Reversed()));
                    }
                }
                else {
                    // make a copy of the current face maker
                    // if the face fails just try again with the copy
                    TopoDS_Face tempCopy = BRepBuilderAPI_MakeFace(faceMaker.Face()).Face();
                    faceMaker.Add(TopoDS::Wire(itWireVec.Reversed()));
                    ShapeFix_Face fix(faceMaker.Face());
                    fix.Perform();
                    auto aFace = fix.Face();
                    BRepCheck_Analyzer aChecker(aFace);
                    if (!aChecker.IsValid()) {
                        faceMaker = BRepBuilderAPI_MakeFace(tempCopy);
                        faceMaker.Add(TopoDS::Wire(itWireVec));
                    }
                }
            }
            // auto doneFlag = faceMaker.IsDone();
            // auto error = faceMaker.Error();
            itCurrentShape.aProjectedFace = faceMaker.Face();
        }
    }
    catch (const Standard_Failure& error) {
        std::stringstream ssOcc;
        error.Print(ssOcc);
        throw Base::ValueError(ssOcc.str().c_str());
    }
}

TopoDS_Wire PartGui::DlgProjectionOnSurface::sort_and_heal_wire(const TopoDS_Shape& iShape,
                                                                const TopoDS_Face& iFaceToProject)
{
    std::vector<TopoDS_Edge> aEdgeVec;
    for (TopExp_Explorer aExplorer(iShape, TopAbs_EDGE); aExplorer.More(); aExplorer.Next()) {
        auto anEdge = TopoDS::Edge(aExplorer.Current());
        aEdgeVec.push_back(anEdge);
    }
    return sort_and_heal_wire(aEdgeVec, iFaceToProject);
}

TopoDS_Wire
PartGui::DlgProjectionOnSurface::sort_and_heal_wire(const std::vector<TopoDS_Edge>& iEdgeVec,
                                                    const TopoDS_Face& iFaceToProject)
{
    // try to sort and heal all wires
    // if the wires are not clean making a face will fail!
    ShapeAnalysis_FreeBounds shapeAnalyzer;
    Handle(TopTools_HSequenceOfShape) shapeList = new TopTools_HSequenceOfShape;
    Handle(TopTools_HSequenceOfShape) aWireHandle;
    Handle(TopTools_HSequenceOfShape) aWireWireHandle;

    for (const auto& it : iEdgeVec) {
        shapeList->Append(it);
    }

    const double tolerance = 0.0001;
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(shapeList, tolerance, false, aWireHandle);
    ShapeAnalysis_FreeBounds::ConnectWiresToWires(aWireHandle, tolerance, false, aWireWireHandle);
    if (!aWireWireHandle) {
        return {};
    }
    for (auto it = 1; it <= aWireWireHandle->Length(); ++it) {
        auto aShape = TopoDS::Wire(aWireWireHandle->Value(it));
        ShapeFix_Wire aWireRepair(aShape, iFaceToProject, tolerance);
        aWireRepair.FixAddCurve3dMode() = 1;
        aWireRepair.FixAddPCurveMode() = 1;
        aWireRepair.Perform();
        // return aWireRepair.Wire();
        ShapeFix_Wireframe aWireFramFix(aWireRepair.Wire());
        aWireFramFix.FixWireGaps();
        aWireFramFix.FixSmallEdges();
        return TopoDS::Wire(aWireFramFix.Shape());
    }
    return {};
}

void PartGui::DlgProjectionOnSurface::create_face_extrude(std::vector<SShapeStore>& iCurrentShape)
{
    try {
        if (iCurrentShape.empty()) {
            return;
        }

        auto height = ui->doubleSpinBoxExtrudeHeight->value();

        for (auto& itCurrentShape : iCurrentShape) {
            if (itCurrentShape.aProjectedFace.IsNull()) {
                continue;
            }
            if (itCurrentShape.extrudeValue == height) {
                continue;
            }

            itCurrentShape.extrudeValue = height;
            if (height == 0) {
                itCurrentShape.aProjectedSolid.Nullify();
            }
            else {
                gp_Vec directionToExtrude(itCurrentShape.aProjectionDir.XYZ());
                directionToExtrude.Reverse();
                directionToExtrude.Multiply(height);
                BRepPrimAPI_MakePrism extrude(itCurrentShape.aProjectedFace, directionToExtrude);
                itCurrentShape.aProjectedSolid = extrude.Shape();
            }
        }
    }
    catch (const Standard_Failure& error) {
        std::stringstream ssOcc;
        error.Print(ssOcc);
        throw Base::ValueError(ssOcc.str().c_str());
    }
}

void PartGui::DlgProjectionOnSurface::store_wire_in_vector(const SShapeStore& iCurrentShape,
                                                           const TopoDS_Shape& iParentShape,
                                                           std::vector<SShapeStore>& iStoreVec,
                                                           unsigned int iColor)
{
    if (m_currentSelection != "add_wire") {
        return;
    }
    if (iParentShape.IsNull()) {
        return;
    }
    if (iCurrentShape.inputShape.IsNull()) {
        return;
    }
    auto currentType = iCurrentShape.inputShape.ShapeType();
    if (currentType != TopAbs_EDGE) {
        return;
    }

    std::vector<TopoDS_Wire> aWireVec;
    for (TopExp_Explorer aExplorer(iParentShape, TopAbs_WIRE); aExplorer.More(); aExplorer.Next()) {
        aWireVec.push_back(TopoDS::Wire(aExplorer.Current()));
    }

    std::vector<TopoDS_Edge> edgeVec;
    for (const auto& it : aWireVec) {
        bool edgeExists = false;
        for (TopExp_Explorer aExplorer(it, TopAbs_EDGE); aExplorer.More(); aExplorer.Next()) {
            auto currentEdge = TopoDS::Edge(aExplorer.Current());
            edgeVec.push_back(currentEdge);
            if (currentEdge.IsSame(iCurrentShape.inputShape)) {
                edgeExists = true;
            }
        }
        if (edgeExists) {
            break;
        }
        edgeVec.clear();
    }

    if (edgeVec.empty()) {
        return;
    }
    TopTools_IndexedMapOfShape indexMap;
    TopExp::MapShapes(iParentShape, TopAbs_EDGE, indexMap);
    if (indexMap.IsEmpty()) {
        return;
    }

    for (const auto& it : edgeVec) {
        if (it.IsSame(iCurrentShape.inputShape)) {
            continue;
        }
        if (!indexMap.Contains(it)) {
            return;
        }
        auto index = indexMap.FindIndex(it);
        auto newEdgeObject = iCurrentShape;
        newEdgeObject.inputShape = it;
        newEdgeObject.partName = "Edge" + std::to_string(index);

        auto store = store_part_in_vector(newEdgeObject, iStoreVec);
        higlight_object(newEdgeObject.partFeature, newEdgeObject.partName, store, iColor);
    }
}

void PartGui::DlgProjectionOnSurface::set_xyz_dir_spinbox(QDoubleSpinBox* icurrentSpinBox)
{
    auto currentVal = icurrentSpinBox->value();
    auto newVal = 0.0;
    if (currentVal != 1.0 && currentVal != -1.0) {
        newVal = -1;
    }
    else if (currentVal == 1.0) {
        newVal = -1;
    }
    else if (currentVal == -1.0) {
        newVal = 1;
    }
    ui->doubleSpinBoxDirX->setValue(0);
    ui->doubleSpinBoxDirY->setValue(0);
    ui->doubleSpinBoxDirZ->setValue(0);
    icurrentSpinBox->setValue(newVal);
}

void PartGui::DlgProjectionOnSurface::transform_shape_to_global_position(TopoDS_Shape& ioShape,
                                                                         Part::Feature* iPart)
{
    auto currentPos = iPart->Placement.getValue().getPosition();
    auto currentRotation = iPart->Placement.getValue().getRotation();
    auto globalPlacement = iPart->globalPlacement();
    auto globalPosition = globalPlacement.getPosition();
    auto globalRotation = globalPlacement.getRotation();

    if (currentRotation != globalRotation) {
        auto newRotation = globalRotation;
        newRotation *= currentRotation.invert();

        gp_Trsf aAngleTransform;
        Base::Vector3d rotationAxes;
        double rotationAngle {};
        newRotation.getRawValue(rotationAxes, rotationAngle);
        aAngleTransform.SetRotation(gp_Ax1(gp_Pnt(currentPos.x, currentPos.y, currentPos.z),
                                           gp_Dir(rotationAxes.x, rotationAxes.y, rotationAxes.z)),
                                    rotationAngle);
        ioShape = BRepBuilderAPI_Transform(ioShape, aAngleTransform, true).Shape();
    }

    if (currentPos != globalPosition) {
        gp_Trsf aPosTransform;
        aPosTransform.SetTranslation(gp_Pnt(currentPos.x, currentPos.y, currentPos.z),
                                     gp_Pnt(globalPosition.x, globalPosition.y, globalPosition.z));
        ioShape = BRepBuilderAPI_Transform(ioShape, aPosTransform, true).Shape();
    }
}

void PartGui::DlgProjectionOnSurface::onPushButtonAddProjFaceClicked()
{
    if (ui->pushButtonAddProjFace->isChecked()) {
        m_currentSelection = "add_projection_surface";
        disable_ui_elements(m_guiObjectVec, ui->pushButtonAddProjFace);
        if (!filterFace) {
            filterFace = new FaceSelection();
            Gui::Selection().addSelectionGate(filterFace);
        }
    }
    else {
        m_currentSelection = "";
        enable_ui_elements(m_guiObjectVec, nullptr);
        Gui::Selection().rmvSelectionGate();
        filterFace = nullptr;
    }
}
void PartGui::DlgProjectionOnSurface::onRadioButtonShowAllClicked()
{
    m_currentShowType = "all";
    show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::onRadioButtonFacesClicked()
{
    m_currentShowType = "faces";
    show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::onRadioButtonEdgesClicked()
{
    m_currentShowType = "edges";
    show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::onDoubleSpinBoxExtrudeHeightValueChanged(double arg1)
{
    Q_UNUSED(arg1);
    create_face_extrude(m_shapeVec);
    show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::onPushButtonAddWireClicked()
{
    if (ui->pushButtonAddWire->isChecked()) {
        m_currentSelection = "add_wire";
        disable_ui_elements(m_guiObjectVec, ui->pushButtonAddWire);
        if (!filterEdge) {
            filterEdge = new EdgeSelection();
            Gui::Selection().addSelectionGate(filterEdge);
        }
        ui->radioButtonEdges->setChecked(true);
        onRadioButtonEdgesClicked();
    }
    else {
        m_currentSelection = "";
        enable_ui_elements(m_guiObjectVec, nullptr);
        Gui::Selection().rmvSelectionGate();
        filterEdge = nullptr;
    }
}

void PartGui::DlgProjectionOnSurface::onDoubleSpinBoxSolidDepthValueChanged(double arg1)
{
    auto valX = ui->doubleSpinBoxDirX->value();
    auto valY = ui->doubleSpinBoxDirY->value();
    auto valZ = ui->doubleSpinBoxDirZ->value();

    auto valueToMove = arg1 - m_lastDepthVal;
    Base::Vector3d vectorToMove(valX, valY, valZ);
    vectorToMove *= valueToMove;

    auto placment = m_projectionObject->Placement.getValue();
    placment.move(vectorToMove);
    m_projectionObject->Placement.setValue(placment);

    m_lastDepthVal = ui->doubleSpinBoxSolidDepth->value();
}

// ---------------------------------------

TaskProjectionOnSurface::TaskProjectionOnSurface()
    : widget(new DlgProjectionOnSurface())
    , taskbox(new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_ProjectionOnSurface"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr))
{
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

bool TaskProjectionOnSurface::accept()
{
    widget->apply();
    return true;
}

bool TaskProjectionOnSurface::reject()
{
    widget->reject();
    return true;
}

void TaskProjectionOnSurface::clicked(int id)
{
    if (id == QDialogButtonBox::Apply) {
        try {
            widget->apply();
        }
        catch (Base::AbortException&) {
        }
    }
}

// ------------------------------------------------------------------------------------------------

DlgProjectOnSurface::DlgProjectOnSurface(Part::ProjectOnSurface* feature, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DlgProjectionOnSurface)
    , filterEdge(nullptr)
    , filterFace(nullptr)
    , feature(feature)
{
    ui->setupUi(this);
    ui->doubleSpinBoxExtrudeHeight->setValue(feature->Height.getValue());
    ui->doubleSpinBoxSolidDepth->setValue(feature->Offset.getValue());
    fetchDirection();
    fetchMode();
    setupConnections();

    ui->pushButtonAddEdge->setCheckable(true);
    ui->pushButtonAddFace->setCheckable(true);
    ui->pushButtonAddProjFace->setCheckable(true);
    ui->pushButtonAddWire->setCheckable(true);
}

DlgProjectOnSurface::~DlgProjectOnSurface()
{
    if (filterFace || filterEdge) {
        Gui::Selection().rmvSelectionGate();
    }
}

void DlgProjectOnSurface::setupConnections()
{
    // clang-format off
    connect(ui->pushButtonAddFace, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onAddFaceClicked);
    connect(ui->pushButtonAddEdge, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onAddEdgeClicked);
    connect(ui->pushButtonGetCurrentCamDir, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onGetCurrentCamDirClicked);
    connect(ui->pushButtonDirX, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onDirXClicked);
    connect(ui->pushButtonDirY, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onDirYClicked);
    connect(ui->pushButtonDirZ, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onDirZClicked);
    connect(ui->pushButtonAddProjFace, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onAddProjFaceClicked);
    connect(ui->radioButtonShowAll, &QRadioButton::clicked,
            this, &DlgProjectOnSurface::onShowAllClicked);
    connect(ui->radioButtonFaces, &QRadioButton::clicked,
            this, &DlgProjectOnSurface::onFacesClicked);
    connect(ui->radioButtonEdges, &QRadioButton::clicked,
            this, &DlgProjectOnSurface::onEdgesClicked);
    connect(ui->doubleSpinBoxExtrudeHeight, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &DlgProjectOnSurface::onExtrudeHeightValueChanged);
    connect(ui->pushButtonAddWire, &QPushButton::clicked,
            this, &DlgProjectOnSurface::onAddWireClicked);
    connect(ui->doubleSpinBoxSolidDepth, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &DlgProjectOnSurface::onSolidDepthValueChanged);
    // clang-format off
}

void DlgProjectOnSurface::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // clang-format off
    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (selectionMode == SelectionMode::AddFace ||
            selectionMode == SelectionMode::AddEdge) {
            addSelection(msg);
        }
        else if (selectionMode == SelectionMode::AddWire) {
            addWire(msg);
        }
        else if (selectionMode == SelectionMode::SupportFace) {
            ui->pushButtonAddProjFace->setChecked(false);
            setSupportFace(msg);
            onAddProjFaceClicked();
        }
    }
    // clang-format on
}

void DlgProjectOnSurface::accept()
{
    if (!feature.expired()) {
        auto document = feature->getDocument();
        document->commitTransaction();
        document->recompute();
    }
}

void DlgProjectOnSurface::reject()
{
    if (!feature.expired()) {
        auto document = feature->getDocument();
        document->abortTransaction();
    }
}

void DlgProjectOnSurface::onAddProjFaceClicked()
{
    if (ui->pushButtonAddProjFace->isChecked()) {
        selectionMode = SelectionMode::SupportFace;
        if (!filterFace) {
            filterFace = new FaceSelection();
            Gui::Selection().addSelectionGate(filterFace);
        }
    }
    else {
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        filterFace = nullptr;
    }
}

void DlgProjectOnSurface::onAddFaceClicked()
{
    if (ui->pushButtonAddFace->isChecked()) {
        selectionMode = SelectionMode::AddFace;
        if (!filterFace) {
            filterFace = new FaceSelection();
            Gui::Selection().addSelectionGate(filterFace);
        }
    }
    else {
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        filterFace = nullptr;
    }
}

void DlgProjectOnSurface::onAddWireClicked()
{
    if (ui->pushButtonAddWire->isChecked()) {
        selectionMode = SelectionMode::AddWire;
        if (!filterEdge) {
            filterEdge = new EdgeSelection();
            Gui::Selection().addSelectionGate(filterEdge);
        }
        ui->radioButtonEdges->setChecked(true);
        onEdgesClicked();
    }
    else {
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        filterEdge = nullptr;
    }
}

void DlgProjectOnSurface::onAddEdgeClicked()
{
    if (ui->pushButtonAddEdge->isChecked()) {
        selectionMode = SelectionMode::AddEdge;
        if (!filterEdge) {
            filterEdge = new EdgeSelection();
            Gui::Selection().addSelectionGate(filterEdge);
        }
        ui->radioButtonEdges->setChecked(true);
        onEdgesClicked();
    }
    else {
        selectionMode = SelectionMode::None;
        Gui::Selection().rmvSelectionGate();
        filterEdge = nullptr;
    }
}

void DlgProjectOnSurface::onGetCurrentCamDirClicked()
{
    auto mainWindow = Gui::getMainWindow();

    auto mdiObject = dynamic_cast<Gui::View3DInventor*>(mainWindow->activeWindow());
    if (!mdiObject) {
        return;
    }
    auto camerRotation = mdiObject->getViewer()->getCameraOrientation();

    SbVec3f lookAt(0, 0, -1);
    camerRotation.multVec(lookAt, lookAt);

    float valX {};
    float valY {};
    float valZ {};
    lookAt.getValue(valX, valY, valZ);

    ui->doubleSpinBoxDirX->setValue(valX);
    ui->doubleSpinBoxDirY->setValue(valY);
    ui->doubleSpinBoxDirZ->setValue(valZ);
    setDirection();
}

void DlgProjectOnSurface::onDirXClicked()
{
    auto currentVal = ui->doubleSpinBoxDirX->value();
    ui->doubleSpinBoxDirX->setValue(currentVal > 0 ? -1 : 1);
    ui->doubleSpinBoxDirY->setValue(0);
    ui->doubleSpinBoxDirZ->setValue(0);
    setDirection();
}

void DlgProjectOnSurface::onDirYClicked()
{
    auto currentVal = ui->doubleSpinBoxDirY->value();
    ui->doubleSpinBoxDirX->setValue(0);
    ui->doubleSpinBoxDirY->setValue(currentVal > 0 ? -1 : 1);
    ui->doubleSpinBoxDirZ->setValue(0);
    setDirection();
}

void DlgProjectOnSurface::onDirZClicked()
{
    auto currentVal = ui->doubleSpinBoxDirZ->value();
    ui->doubleSpinBoxDirX->setValue(0);
    ui->doubleSpinBoxDirY->setValue(0);
    ui->doubleSpinBoxDirZ->setValue(currentVal > 0 ? -1 : 1);
    setDirection();
}

void DlgProjectOnSurface::setDirection()
{
    if (!feature.expired()) {
        auto xVal = ui->doubleSpinBoxDirX->value();
        auto yVal = ui->doubleSpinBoxDirY->value();
        auto zVal = ui->doubleSpinBoxDirZ->value();
        feature->Direction.setValue(Base::Vector3d(xVal, yVal, zVal));
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::addWire(const Gui::SelectionChanges& msg)
{
    auto isEdgePartOf = [](const TopoDS_Shape& wire, const TopoDS_Shape& edge) {
        for (TopExp_Explorer xp(wire, TopAbs_EDGE); xp.More(); xp.Next()) {
            if (edge.IsSame(xp.Current())) {
                return true;
            }
        }

        return false;
    };
    if (selectionMode != SelectionMode::AddWire) {
        return;
    }

    Gui::SelectionObject selObj(msg);
    if (!selObj.hasSubNames()) {
        return;
    }

    Part::TopoShape part = Part::Feature::getTopoShape(selObj.getObject(), Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (part.isNull()) {
        return;
    }

    std::string subName = selObj.getSubNames().front();
    TopoDS_Shape edge = part.getSubShape(subName.c_str(), true);
    if (edge.IsNull() || edge.ShapeType() != TopAbs_EDGE) {
        return;
    }

    int index = 1;
    const TopoDS_Shape& shape = part.getShape();
    for (TopExp_Explorer xp(shape, TopAbs_WIRE); xp.More(); xp.Next()) {
        if (isEdgePartOf(xp.Current(), edge)) {
            std::string name{"Wire"};
            name += std::to_string(index);
            addSelection(msg, name);
            break;
        }
        index++;
    }
}

void DlgProjectOnSurface::addSelection(const Gui::SelectionChanges& msg,
                                       const std::string& subName)
{
    if (!feature.expired()) {
        Gui::SelectionObject selObj(msg);
        feature->Projection.addValue(selObj.getObject(), {subName});
    }
}

void DlgProjectOnSurface::addSelection(const Gui::SelectionChanges& msg)
{
    if (!feature.expired()) {
        Gui::SelectionObject selObj(msg);
        feature->Projection.addValue(selObj.getObject(), selObj.getSubNames());
    }
}

void DlgProjectOnSurface::setSupportFace(const Gui::SelectionChanges& msg)
{
    Gui::SelectionObject selObj(msg);
    if (!feature.expired()) {
        feature->SupportFace.setValue(selObj.getObject(), selObj.getSubNames());
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::fetchDirection()
{
    if (!feature.expired()) {
        auto dir = feature->Direction.getValue();
        ui->doubleSpinBoxDirX->setValue(dir.x);
        ui->doubleSpinBoxDirY->setValue(dir.y);
        ui->doubleSpinBoxDirZ->setValue(dir.z);
    }
}

void DlgProjectOnSurface::fetchMode()
{
    if (!feature.expired()) {
        if (strcmp(feature->Mode.getValueAsString(), Part::ProjectOnSurface::AllMode) == 0) {
            ui->radioButtonShowAll->setChecked(true);
        }
        else if (strcmp(feature->Mode.getValueAsString(), Part::ProjectOnSurface::FacesMode) == 0) {
            ui->radioButtonFaces->setChecked(true);
        }
        else if (strcmp(feature->Mode.getValueAsString(), Part::ProjectOnSurface::EdgesMode) == 0) {
            ui->radioButtonEdges->setChecked(true);
        }
    }
}

void DlgProjectOnSurface::onShowAllClicked()
{
    if (!feature.expired()) {
        feature->Mode.setValue(Part::ProjectOnSurface::AllMode);
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::onFacesClicked()
{
    if (!feature.expired()) {
        feature->Mode.setValue(Part::ProjectOnSurface::FacesMode);
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::onEdgesClicked()
{
    if (!feature.expired()) {
        feature->Mode.setValue(Part::ProjectOnSurface::EdgesMode);
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::onExtrudeHeightValueChanged(double value)
{
    if (!feature.expired()) {
        feature->Height.setValue(value);
        feature->recomputeFeature();
    }
}

void DlgProjectOnSurface::onSolidDepthValueChanged(double value)
{
    if (!feature.expired()) {
        feature->Offset.setValue(value);
        feature->recomputeFeature();
    }
}

// ---------------------------------------

TaskProjectOnSurface::TaskProjectOnSurface(App::Document* doc)
{
    setDocumentName(doc->getName());
    doc->openTransaction(QT_TRANSLATE_NOOP("Command", "Project on surface"));
    auto feature = doc->addObject<Part::ProjectOnSurface>("Projection");
    widget = new DlgProjectOnSurface(feature);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_ProjectionOnSurface"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskProjectOnSurface::TaskProjectOnSurface(Part::ProjectOnSurface* feature)
    : widget(new DlgProjectOnSurface(feature))
    , taskbox(new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_ProjectionOnSurface"),
                                         widget->windowTitle(),
                                         true,
                                         nullptr))
{
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

void TaskProjectOnSurface::resetEdit()
{
    std::string document = getDocumentName();
    Gui::doCommandT(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document);
}

bool TaskProjectOnSurface::accept()
{
    widget->accept();
    resetEdit();
    return true;
}

bool TaskProjectOnSurface::reject()
{
    widget->reject();
    resetEdit();
    return true;
}

#include "moc_DlgProjectionOnSurface.cpp"
