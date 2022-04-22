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
# include <TopoDS_Face.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepProj_Projection.hxx>
# include <TopoDS_Builder.hxx>
# include <TopoDS_Edge.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <ShapeFix_Wire.hxx>
# include <BRep_Tool.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GeomProjLib.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include "ShapeFix_Edge.hxx"
# include <BRepBuilderAPI_MakeFace.hxx>
# include <ShapeFix_Face.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <ShapeFix_Wireframe.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Ax1.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
#endif

#include <Inventor/SbVec3d.h>

#include "DlgProjectionOnSurface.h"
#include "ui_DlgProjectionOnSurface.h"

#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Application.h>
#include <Gui/SelectionObject.h>

#include "ViewProviderExt.h"





using namespace PartGui;

//////////////////////////////////////////////////////////////////////////
class DlgProjectionOnSurface::EdgeSelection : public Gui::SelectionFilterGate
{
public:
  bool canSelect;

  EdgeSelection()
    : Gui::SelectionFilterGate(nullPointer())
  {
    canSelect = false;
  }
  ~EdgeSelection() {}

  bool allow(App::Document* /*pDoc*/, App::DocumentObject* iPObj, const char* sSubName)
  {
    Part::Feature* aPart = dynamic_cast<Part::Feature*>(iPObj);
    if (!aPart)
        return false;
    if (!sSubName)
        return false;
    std::string subName(sSubName);
    if (subName.empty())
        return false;

    auto subShape = aPart->Shape.getShape().getSubShape(sSubName);
    if (subShape.IsNull())
        return false;
    auto type = subShape.ShapeType();
    if (type != TopAbs_EDGE)
        return false;
    return true;
  }
};
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class DlgProjectionOnSurface::FaceSelection : public Gui::SelectionFilterGate
{
public:
  bool canSelect;

  FaceSelection()
    : Gui::SelectionFilterGate(nullPointer())
  {
    canSelect = false;
  }
  ~FaceSelection() {}

  bool allow(App::Document* /*pDoc*/, App::DocumentObject* iPObj, const char* sSubName)
  {
    Part::Feature* aPart = dynamic_cast<Part::Feature*>(iPObj);
    if (!aPart)
        return false;
    if (!sSubName)
        return false;
    std::string subName(sSubName);
    if (subName.empty())
        return false;

    auto subShape = aPart->Shape.getShape().getSubShape(sSubName, true);
    if (subShape.IsNull())
        return false;
    auto type = subShape.ShapeType();
    if (type != TopAbs_FACE)
        return false;
    return true;
  }
};
//////////////////////////////////////////////////////////////////////////


DlgProjectionOnSurface::DlgProjectionOnSurface(QWidget *parent)
  : QWidget(parent)
  , ui(new Ui::DlgProjectionOnSurface)
  , m_projectionObjectName(tr("Projection Object"))
  , filterEdge(nullptr)
  , filterFace(nullptr)
{
    ui->setupUi(this);
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
    if (!m_partDocument)
    {
      throw Base::ValueError(QString(tr("Have no active document!!!")).toUtf8());
    }
    this->attachDocument(m_partDocument);
    m_partDocument->openTransaction("Project on surface");
    m_projectionObject = dynamic_cast<Part::Feature*>(m_partDocument->addObject("Part::Feature", "Projection Object"));
    if (!m_projectionObject)
    {
      throw Base::ValueError(QString(tr("Can not create a projection object!!!")).toUtf8());
    }
    m_projectionObject->Label.setValue(std::string(m_projectionObjectName.toUtf8()).c_str());
    on_radioButtonShowAll_clicked();
    m_lastDepthVal = ui->doubleSpinBoxSolidDepth->value();
}

DlgProjectionOnSurface::~DlgProjectionOnSurface()
{
  delete ui;
  for ( auto it : m_projectionSurfaceVec)
  {
    try {
      higlight_object(it.partFeature, it.partName, false, 0);
    }
    catch (Standard_NoSuchObject& e) {
      Base::Console().Warning("DlgProjectionOnSurface::~DlgProjectionOnSurface: %s", e.GetMessageString());
    }
    PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(it.partFeature));
    if (vp)
    {
      vp->Selectable.setValue(it.is_selectable);
      vp->Transparency.setValue(it.transparency);
    }
  }
  for (auto it : m_shapeVec)
  {
    try {
      higlight_object(it.partFeature, it.partName, false, 0);
    }
    catch (Standard_NoSuchObject& e) {
      Base::Console().Warning("DlgProjectionOnSurface::~DlgProjectionOnSurface: %s", e.GetMessageString());
    }
  }
  Gui::Selection().rmvSelectionGate();
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

void PartGui::DlgProjectionOnSurface::apply(void)
{
  if (m_partDocument)
    m_partDocument->commitTransaction();
}

void PartGui::DlgProjectionOnSurface::reject(void)
{
  if (m_partDocument)
    m_partDocument->abortTransaction();
}

void PartGui::DlgProjectionOnSurface::on_pushButtonAddFace_clicked()
{
  if ( ui->pushButtonAddFace->isChecked() )
  {
    m_currentSelection = "add_face";
    disable_ui_elements(m_guiObjectVec, ui->pushButtonAddFace);
    if (!filterFace)
    {
      filterFace = new FaceSelection();
      Gui::Selection().addSelectionGate(filterFace);
    }
  }
  else
  {
    m_currentSelection = "";
    enable_ui_elements(m_guiObjectVec, nullptr);
    Gui::Selection().rmvSelectionGate();
    filterFace = nullptr;
  }
}

void PartGui::DlgProjectionOnSurface::on_pushButtonAddEdge_clicked()
{
  if (ui->pushButtonAddEdge->isChecked())
  {
    m_currentSelection = "add_edge";
    disable_ui_elements(m_guiObjectVec, ui->pushButtonAddEdge);
    if (!filterEdge)
    {
      filterEdge = new EdgeSelection();
      Gui::Selection().addSelectionGate(filterEdge);
    }
    ui->radioButtonEdges->setChecked(true);
    on_radioButtonEdges_clicked();
  }
  else
  {
    m_currentSelection = "";
    enable_ui_elements(m_guiObjectVec, nullptr);
    Gui::Selection().rmvSelectionGate();
    filterEdge = nullptr;
  }
}

void PartGui::DlgProjectionOnSurface::on_pushButtonGetCurrentCamDir_clicked()
{
  get_camera_direction();
}

void PartGui::DlgProjectionOnSurface::on_pushButtonDirX_clicked()
{
  set_xyz_dir_spinbox(ui->doubleSpinBoxDirX);
}

void PartGui::DlgProjectionOnSurface::on_pushButtonDirY_clicked()
{
  set_xyz_dir_spinbox(ui->doubleSpinBoxDirY);
}

void PartGui::DlgProjectionOnSurface::on_pushButtonDirZ_clicked()
{
  set_xyz_dir_spinbox(ui->doubleSpinBoxDirZ);
}

void PartGui::DlgProjectionOnSurface::onSelectionChanged(const Gui::SelectionChanges& msg)
{
  if (msg.Type == Gui::SelectionChanges::AddSelection)
  {
    if ( m_currentSelection == "add_face" || m_currentSelection == "add_edge" || m_currentSelection == "add_wire")
    {
      store_current_selected_parts(m_shapeVec, 0xff00ff00);
      create_projection_wire(m_shapeVec);
      create_projection_face_from_wire(m_shapeVec);
      create_face_extrude(m_shapeVec);
      show_projected_shapes(m_shapeVec);
    }
    else if (m_currentSelection == "add_projection_surface")
    {
      m_projectionSurfaceVec.clear();
      store_current_selected_parts(m_projectionSurfaceVec, 0xffff0000);
      if (m_projectionSurfaceVec.size())
      {
        PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(m_projectionSurfaceVec.back().partFeature));
        if (vp)
        {
          vp->Selectable.setValue(false);
          vp->Transparency.setValue(90);
        }
      }

      ui->pushButtonAddProjFace->setChecked(false);
      on_pushButtonAddProjFace_clicked();
    }
  }
}

void PartGui::DlgProjectionOnSurface::get_camera_direction(void)
{
  auto mainWindow = Gui::getMainWindow();

  auto mdiObject = dynamic_cast<Gui::View3DInventor*>(mainWindow->activeWindow());
  if (!mdiObject)
      return;
  auto camerRotation = mdiObject->getViewer()->getCameraOrientation();

  SbVec3f lookAt(0, 0, -1);
  camerRotation.multVec(lookAt, lookAt);

  float valX, valY, valZ;
  lookAt.getValue(valX, valY, valZ);

  ui->doubleSpinBoxDirX->setValue(valX);
  ui->doubleSpinBoxDirY->setValue(valY);
  ui->doubleSpinBoxDirZ->setValue(valZ);
}

void PartGui::DlgProjectionOnSurface::store_current_selected_parts(std::vector<SShapeStore>& iStoreVec, const unsigned int iColor)
{
  if (!m_partDocument)
      return;
  std::vector<Gui::SelectionObject> selObj = Gui::Selection().getSelectionEx();
  if (selObj.size())
  {
    for (auto it = selObj.begin(); it != selObj.end(); ++it)
    {
      auto aPart = dynamic_cast<Part::Feature*>(it->getObject());
      if (!aPart) continue;

      if (aPart)
      {
        SShapeStore currentShapeStore;
        currentShapeStore.inputShape = aPart->Shape.getShape().getShape();
        currentShapeStore.partFeature = aPart;
        currentShapeStore.partName = aPart->getNameInDocument();

        PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(aPart));
        if (vp)
        {
          currentShapeStore.is_selectable = vp->Selectable.getValue();
          currentShapeStore.transparency = vp->Transparency.getValue();
        }
        if (it->getSubNames().size() )
        {
          auto parentShape = currentShapeStore.inputShape;
          for (auto itName = selObj.front().getSubNames().begin(); itName != selObj.front().getSubNames().end(); ++itName)
          {
            auto currentShape =  aPart->Shape.getShape().getSubShape(itName->c_str());

            transform_shape_to_global_position(currentShape, aPart);

            currentShapeStore.inputShape = currentShape;
            currentShapeStore.partName = *itName;
            auto store = store_part_in_vector(currentShapeStore, iStoreVec);
            higlight_object(aPart, *itName, store, iColor);
            store_wire_in_vector(currentShapeStore, parentShape, iStoreVec, iColor);
          }
        }
        else
        {
          transform_shape_to_global_position(currentShapeStore.inputShape,currentShapeStore.partFeature);
          auto store = store_part_in_vector(currentShapeStore, iStoreVec);
          higlight_object(aPart, aPart->Shape.getName(), store, iColor);
        }
        Gui::Selection().clearSelection(m_partDocument->getName());
        Gui::Selection().rmvPreselect();
      }
    }
  }
}

bool PartGui::DlgProjectionOnSurface::store_part_in_vector(SShapeStore& iCurrentShape, std::vector<SShapeStore>& iStoreVec)
{
  if (iCurrentShape.inputShape.IsNull())
      return false;
  auto currentType = iCurrentShape.inputShape.ShapeType();
  for ( auto it = iStoreVec.begin(); it != iStoreVec.end(); ++it)
  {
    if ( currentType == TopAbs_FACE )
    {
      if (it->aFace.IsSame(iCurrentShape.inputShape))
      {
        iStoreVec.erase(it);
        return false;
      }
    }
    else if ( currentType == TopAbs_EDGE )
    {
      if (it->aEdge.IsSame(iCurrentShape.inputShape))
      {
        iStoreVec.erase(it);
        return false;
      }
    }
  }

  if (currentType == TopAbs_FACE)
  {
    iCurrentShape.aFace = TopoDS::Face(iCurrentShape.inputShape);
  }
  else if (currentType == TopAbs_EDGE)
  {
    iCurrentShape.aEdge = TopoDS::Edge(iCurrentShape.inputShape);
  }

  auto valX =  ui->doubleSpinBoxDirX->value();
  auto valY = ui->doubleSpinBoxDirY->value();
  auto valZ = ui->doubleSpinBoxDirZ->value();

  iCurrentShape.aProjectionDir = gp_Dir(valX, valY, valZ);
  if ( m_projectionSurfaceVec.size() )
  {
    iCurrentShape.surfaceToProject = m_projectionSurfaceVec.front().aFace;
  }
  iStoreVec.push_back(iCurrentShape);
  return true;
}

void PartGui::DlgProjectionOnSurface::create_projection_wire(std::vector<SShapeStore>& iCurrentShape)
{
  try
  {
    if (iCurrentShape.empty())
        return;
    for ( auto &itCurrentShape : iCurrentShape )
    {
      if (m_projectionSurfaceVec.empty()) continue;;
      if (itCurrentShape.aProjectedEdgeVec.size()) continue;;
      if (!itCurrentShape.aProjectedFace.IsNull()) continue;;
      if (itCurrentShape.aProjectedWireVec.size()) continue;;

      if (!itCurrentShape.aFace.IsNull())
      {
        get_all_wire_from_face(itCurrentShape);
        for (auto itWire : itCurrentShape.aWireVec)
        {
          BRepProj_Projection aProjection(itWire, itCurrentShape.surfaceToProject, itCurrentShape.aProjectionDir);
          double minDistance = std::numeric_limits<double>::max();
          TopoDS_Wire wireToTake;
          for ( ; aProjection.More(); aProjection.Next() )
          {
            auto it = aProjection.Current();
            BRepExtrema_DistShapeShape distanceMeasure(it, itCurrentShape.aFace);
            distanceMeasure.Perform();
            auto currentDistance = distanceMeasure.Value();
            if ( currentDistance > minDistance ) continue;
            wireToTake = it;
            minDistance = currentDistance;
          }
          auto aWire = sort_and_heal_wire(wireToTake, itCurrentShape.surfaceToProject);
          itCurrentShape.aProjectedWireVec.push_back(aWire);
        }
      }
      else if (!itCurrentShape.aEdge.IsNull())
      {
        BRepProj_Projection aProjection(itCurrentShape.aEdge, itCurrentShape.surfaceToProject, itCurrentShape.aProjectionDir);
        double minDistance = std::numeric_limits<double>::max();
        TopoDS_Wire wireToTake;
        for (; aProjection.More(); aProjection.Next())
        {
          auto it = aProjection.Current();
          BRepExtrema_DistShapeShape distanceMeasure(it, itCurrentShape.aEdge);
          distanceMeasure.Perform();
          auto currentDistance = distanceMeasure.Value();
          if (currentDistance > minDistance) continue;
          wireToTake = it;
          minDistance = currentDistance;
        }
        for (TopExp_Explorer aExplorer(wireToTake, TopAbs_EDGE); aExplorer.More(); aExplorer.Next())
        {
          itCurrentShape.aProjectedEdgeVec.push_back(TopoDS::Edge(aExplorer.Current()));
        }
      }

    }
  }
  catch (const Standard_Failure& error)
  {
    std::stringstream ssOcc;
    error.Print(ssOcc);
    throw Base::ValueError(ssOcc.str().c_str());
  }
}

TopoDS_Shape PartGui::DlgProjectionOnSurface::create_compound(const std::vector<SShapeStore>& iShapeVec)
{
  if (iShapeVec.empty())
      return TopoDS_Shape();

  TopoDS_Compound aCompound;
  TopoDS_Builder aBuilder;
  aBuilder.MakeCompound(aCompound);

  for (auto it : iShapeVec)
  {
    if ( m_currentShowType == "edges" )
    {
      for (auto it2 : it.aProjectedEdgeVec)
      {
        aBuilder.Add(aCompound, it2);
      }
      for (auto it2 : it.aProjectedWireVec)
      {
        aBuilder.Add(aCompound, it2);
      }
      continue;
    }
    else if ( m_currentShowType == "faces" )
    {
      if (it.aProjectedFace.IsNull())
      {
        for (auto it2 : it.aProjectedWireVec)
        {
          if (!it2.IsNull())
          {
            aBuilder.Add(aCompound, it2);
          }
        }
      }
      else aBuilder.Add(aCompound, it.aProjectedFace);
      continue;
    }
    else if ( m_currentShowType == "all" )
    {
      if (!it.aProjectedSolid.IsNull())
      {
        aBuilder.Add(aCompound, it.aProjectedSolid);
      }
      else if ( !it.aProjectedFace.IsNull() )
      {
        aBuilder.Add(aCompound, it.aProjectedFace);
      }
      else if (it.aProjectedWireVec.size())
      {
        for ( auto itWire : it.aProjectedWireVec )
        {
          if ( itWire.IsNull() ) continue;
          aBuilder.Add(aCompound, itWire);
        }
      }
      else if (it.aProjectedEdgeVec.size())
      {
        for (auto itEdge : it.aProjectedEdgeVec)
        {
          if (itEdge.IsNull()) continue;
          aBuilder.Add(aCompound, itEdge);
        }
      }
    }
  }
  return TopoDS_Shape(std::move(aCompound));
}

void PartGui::DlgProjectionOnSurface::show_projected_shapes(const std::vector<SShapeStore>& iShapeStoreVec)
{
  if (!m_projectionObject)
      return;
  auto aCompound = create_compound(iShapeStoreVec);
  if ( aCompound.IsNull() )
  {
    if (!m_partDocument)
        return;
    m_projectionObject->Shape.setValue(TopoDS_Shape());
    return;
  }
  auto currentPlacement = m_projectionObject->Placement.getValue();
  m_projectionObject->Shape.setValue(aCompound);
  m_projectionObject->Placement.setValue(currentPlacement);

  //set color
  PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(m_projectionObject));
  if (vp)
  {
    vp->LineColor.setValue(0x8ae23400);
    vp->ShapeColor.setValue(0x8ae23400);
    vp->PointColor.setValue(0x8ae23400);
  }
}

void PartGui::DlgProjectionOnSurface::disable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis)
{
  for ( auto it : iObjectVec )
  {
    if ( !it ) continue;
    if ( it == iExceptThis ) continue;
    it->setDisabled(true);
  }
}

void PartGui::DlgProjectionOnSurface::enable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis)
{
  for (auto it : iObjectVec)
  {
    if (!it) continue;
    if (it == iExceptThis) continue;
    it->setEnabled(true);
  }
}

void PartGui::DlgProjectionOnSurface::higlight_object(Part::Feature* iCurrentObject, const std::string& iShapeName, bool iHighlight, const unsigned int iColor)
{
  if (!iCurrentObject)
      return;
  auto partenShape = iCurrentObject->Shape.getShape().getShape();
  auto subShape = iCurrentObject->Shape.getShape().getSubShape(iShapeName.c_str(), true);

  TopoDS_Shape currentShape = subShape;
  if (subShape.IsNull()) currentShape = partenShape;

  auto currentShapeType = currentShape.ShapeType();
  TopTools_IndexedMapOfShape anIndices;
  TopExp::MapShapes(partenShape, currentShapeType, anIndices);
  if (anIndices.IsEmpty())
      return;
  if (!anIndices.Contains(currentShape))
      return;
  auto index = anIndices.FindIndex(currentShape);

  //set color
  PartGui::ViewProviderPartExt* vp = dynamic_cast<PartGui::ViewProviderPartExt*>(Gui::Application::Instance->getViewProvider(iCurrentObject));
  if (vp)
  {
    std::vector<App::Color> colors;
    App::Color defaultColor;
    if (currentShapeType == TopAbs_FACE)
    {
      colors = vp->DiffuseColor.getValues();
      defaultColor = vp->ShapeColor.getValue();
    }
    else if ( currentShapeType == TopAbs_EDGE )
    {
      colors = vp->LineColorArray.getValues();
      defaultColor = vp->LineColor.getValue();
    }

    if ( static_cast<Standard_Integer>(colors.size()) != anIndices.Extent() )
    {
      colors.resize(anIndices.Extent(), defaultColor);
    }

    if ( iHighlight )
    {
      App::Color aColor;
      aColor.setPackedValue(iColor);
      colors.at(index - 1) = aColor;
    }
    else
    {
      colors.at(index - 1) = defaultColor;
    }
    if (currentShapeType == TopAbs_FACE)
    {
      vp->DiffuseColor.setValues(colors);
    }
    else if (currentShapeType == TopAbs_EDGE)
    {
      vp->LineColorArray.setValues(colors);
    }
  }
}

void PartGui::DlgProjectionOnSurface::get_all_wire_from_face(SShapeStore& ioCurrentSahpe)
{
  auto outerWire = ShapeAnalysis::OuterWire(ioCurrentSahpe.aFace);
  ioCurrentSahpe.aWireVec.push_back(outerWire);
  for (TopExp_Explorer aExplorer(ioCurrentSahpe.aFace, TopAbs_WIRE); aExplorer.More(); aExplorer.Next())
  {
    auto currentWire = TopoDS::Wire(aExplorer.Current());
    if (currentWire.IsSame(outerWire)) continue;
    ioCurrentSahpe.aWireVec.push_back(currentWire);
  }
}

void PartGui::DlgProjectionOnSurface::create_projection_face_from_wire(std::vector<SShapeStore>& iCurrentShape)
{
  try
  {
    if (iCurrentShape.empty())
        return;

    for ( auto &itCurrentShape : iCurrentShape )
    {
      if (itCurrentShape.aFace.IsNull()) continue;;
      if (itCurrentShape.aProjectedWireVec.empty()) continue;;
      if (!itCurrentShape.aProjectedFace.IsNull()) continue;;

      auto surface = BRep_Tool::Surface(itCurrentShape.surfaceToProject);

      //create a wire of all edges in parametric space on the surface of the face to projected
      // --> otherwise BRepBuilderAPI_MakeFace can not make a face from the wire!
      for (auto itWireVec : itCurrentShape.aProjectedWireVec)
      {
        std::vector<TopoDS_Shape> edgeVec;
        for (TopExp_Explorer aExplorer(itWireVec, TopAbs_EDGE); aExplorer.More(); aExplorer.Next())
        {
          auto currentEdge = TopoDS::Edge(aExplorer.Current());
          edgeVec.push_back(currentEdge);
        }
        if (edgeVec.empty()) continue;

        std::vector<TopoDS_Edge> edgeInParametricSpaceVec;
        for (auto itEdge : edgeVec)
        {
          Standard_Real first, last;
          auto currentCurve = BRep_Tool::CurveOnSurface(TopoDS::Edge(itEdge), itCurrentShape.surfaceToProject, first, last);
          if (!currentCurve) continue;
          auto edgeInParametricSpace = BRepBuilderAPI_MakeEdge(currentCurve, surface, first,last).Edge();
          edgeInParametricSpaceVec.push_back(edgeInParametricSpace);
        }
        auto aWire = sort_and_heal_wire(edgeInParametricSpaceVec, itCurrentShape.surfaceToProject);
        itCurrentShape.aProjectedWireInParametricSpaceVec.push_back(aWire);
      }

      // try to create a face from the wires
      // the first wire is the otherwise
      // the following wires are the inside wires
      BRepBuilderAPI_MakeFace faceMaker;
      bool first = true;
      for (auto itWireVec : itCurrentShape.aProjectedWireInParametricSpaceVec)
      {
        if (first)
        {
          first = false;
          // change the wire direction, otherwise no face is created
          auto currentWire = TopoDS::Wire(itWireVec.Reversed());
          if (itCurrentShape.surfaceToProject.Orientation() == TopAbs_REVERSED) currentWire = itWireVec;
          faceMaker = BRepBuilderAPI_MakeFace(surface, currentWire);
          ShapeFix_Face fix(faceMaker.Face());
          fix.Perform();
          auto aFace = fix.Face();
          BRepCheck_Analyzer aChecker(aFace);
          if (!aChecker.IsValid())
          {
            faceMaker = BRepBuilderAPI_MakeFace(surface, TopoDS::Wire(currentWire.Reversed()));
          }
        }
        else
        {
          // make a copy of the current face maker
          // if the face fails just try again with the copy
          TopoDS_Face tempCopy = BRepBuilderAPI_MakeFace(faceMaker.Face()).Face();
          faceMaker.Add(TopoDS::Wire(itWireVec.Reversed()));
          ShapeFix_Face fix(faceMaker.Face());
          fix.Perform();
          auto aFace = fix.Face();
          BRepCheck_Analyzer aChecker(aFace);
          if (!aChecker.IsValid())
          {
            faceMaker = BRepBuilderAPI_MakeFace(tempCopy);
            faceMaker.Add(TopoDS::Wire(itWireVec));
          }
        }
      }
      //auto doneFlag = faceMaker.IsDone();
      //auto error = faceMaker.Error();
      itCurrentShape.aProjectedFace = faceMaker.Face();
    }
  }
  catch (const Standard_Failure& error)
  {
    std::stringstream ssOcc;
    error.Print(ssOcc);
    throw Base::ValueError(ssOcc.str().c_str());
  }
}

TopoDS_Wire PartGui::DlgProjectionOnSurface::sort_and_heal_wire(const TopoDS_Shape& iShape, const TopoDS_Face& iFaceToProject)
{
  std::vector<TopoDS_Edge> aEdgeVec;
  for (TopExp_Explorer aExplorer(iShape, TopAbs_EDGE); aExplorer.More(); aExplorer.Next())
  {
    auto anEdge = TopoDS::Edge(aExplorer.Current());
    aEdgeVec.push_back(anEdge);
  }
  return sort_and_heal_wire(aEdgeVec, iFaceToProject);
}

TopoDS_Wire PartGui::DlgProjectionOnSurface::sort_and_heal_wire(const std::vector<TopoDS_Edge>& iEdgeVec, const TopoDS_Face& iFaceToProject)
{
  // try to sort and heal all wires
// if the wires are not clean making a face will fail!
  ShapeAnalysis_FreeBounds shapeAnalyzer;
  Handle(TopTools_HSequenceOfShape) shapeList = new TopTools_HSequenceOfShape;
  Handle(TopTools_HSequenceOfShape) aWireHandle;
  Handle(TopTools_HSequenceOfShape) aWireWireHandle;

  for (auto it : iEdgeVec)
  {
    shapeList->Append(it);
  }

  shapeAnalyzer.ConnectEdgesToWires(shapeList, 0.0001, false, aWireHandle);
  shapeAnalyzer.ConnectWiresToWires(aWireHandle, 0.0001, false, aWireWireHandle);
  if (!aWireWireHandle)
      return TopoDS_Wire();
  for (auto it = 1; it <= aWireWireHandle->Length(); ++it)
  {
    auto aShape = TopoDS::Wire(aWireWireHandle->Value(it));
    ShapeFix_Wire aWireRepair(aShape, iFaceToProject, 0.0001);
    aWireRepair.FixAddCurve3dMode() = 1;
    aWireRepair.FixAddPCurveMode() = 1;
    aWireRepair.Perform();
    //return aWireRepair.Wire();
    ShapeFix_Wireframe aWireFramFix(aWireRepair.Wire());
    auto retVal = aWireFramFix.FixWireGaps();
    retVal = aWireFramFix.FixSmallEdges();
    Q_UNUSED(retVal);
    return TopoDS::Wire(aWireFramFix.Shape());
  }
  return TopoDS_Wire();
}

void PartGui::DlgProjectionOnSurface::create_face_extrude(std::vector<SShapeStore>& iCurrentShape)
{
  try
  {
    if (iCurrentShape.empty())
        return;

    for ( auto &itCurrentShape : iCurrentShape )
    {
      if (itCurrentShape.aProjectedFace.IsNull()) continue;;
      auto height = ui->doubleSpinBoxExtrudeHeight->value();
      if (itCurrentShape.exrudeValue == height) continue;;

      gp_Vec directionToExtrude(itCurrentShape.aProjectionDir.XYZ());
      directionToExtrude.Reverse();
      if (height == 0)
          return;
      directionToExtrude.Multiply(height);
      BRepPrimAPI_MakePrism extrude(itCurrentShape.aProjectedFace, directionToExtrude);
      itCurrentShape.aProjectedSolid = extrude.Shape();
      itCurrentShape.exrudeValue = height;
    }
  }
  catch (const Standard_Failure& error)
  {
    std::stringstream ssOcc;
    error.Print(ssOcc);
    throw Base::ValueError(ssOcc.str().c_str());
  }
}

void PartGui::DlgProjectionOnSurface::store_wire_in_vector(const SShapeStore& iCurrentShape, const TopoDS_Shape& iParentShape, std::vector<SShapeStore>& iStoreVec, const unsigned int iColor)
{
  if (m_currentSelection != "add_wire")
      return;
  if (iParentShape.IsNull())
      return;
  if (iCurrentShape.inputShape.IsNull())
      return;
  auto currentType = iCurrentShape.inputShape.ShapeType();
  if (currentType != TopAbs_EDGE)
      return;

  std::vector<TopoDS_Wire> aWireVec;
  for (TopExp_Explorer aExplorer(iParentShape, TopAbs_WIRE); aExplorer.More(); aExplorer.Next())
  {
    aWireVec.push_back(TopoDS::Wire(aExplorer.Current()));
  }

  std::vector<TopoDS_Edge> edgeVec;
  for ( auto it : aWireVec )
  {
    bool edgeExists = false;
    for (TopExp_Explorer aExplorer(it, TopAbs_EDGE); aExplorer.More(); aExplorer.Next())
    {
      auto currentEdge = TopoDS::Edge(aExplorer.Current());
      edgeVec.push_back(currentEdge);
      if (currentEdge.IsSame(iCurrentShape.inputShape)) edgeExists = true;
    }
    if (edgeExists) break;
    edgeVec.clear();
  }

  if (edgeVec.empty())
      return;
  TopTools_IndexedMapOfShape indexMap;
  TopExp::MapShapes(iParentShape, TopAbs_EDGE, indexMap);
  if (indexMap.IsEmpty())
      return;

  for ( auto it : edgeVec )
  {
    if ( it.IsSame(iCurrentShape.inputShape)) continue;
    if (!indexMap.Contains(it))
        return;
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
  if (currentVal != 1.0 && currentVal != -1.0)
  {
    newVal = -1;
  }
  else if (currentVal == 1.0)
  {
    newVal = -1;
  }
  else if (currentVal == -1.0)
  {
    newVal = 1;
  }
  ui->doubleSpinBoxDirX->setValue(0);
  ui->doubleSpinBoxDirY->setValue(0);
  ui->doubleSpinBoxDirZ->setValue(0);
  icurrentSpinBox->setValue(newVal);
}

void PartGui::DlgProjectionOnSurface::transform_shape_to_global_position(TopoDS_Shape& ioShape, Part::Feature* iPart)
{
  auto currentPos = iPart->Placement.getValue().getPosition();
  auto currentRotation = iPart->Placement.getValue().getRotation();
  auto globalPlacement = iPart->globalPlacement();
  auto globalPosition = globalPlacement.getPosition();
  auto globalRotation = globalPlacement.getRotation();

  if (currentRotation != globalRotation)
  {
    auto newRotation = globalRotation;
    newRotation *= currentRotation.invert();

    gp_Trsf aAngleTransform;
    Base::Vector3d rotationAxes;
    double rotationAngle;
    newRotation.getRawValue(rotationAxes, rotationAngle);
    aAngleTransform.SetRotation(gp_Ax1(gp_Pnt(currentPos.x, currentPos.y, currentPos.z), gp_Dir(rotationAxes.x, rotationAxes.y, rotationAxes.z)), rotationAngle);
    ioShape = BRepBuilderAPI_Transform(ioShape, aAngleTransform, true).Shape();
  }

  if (currentPos != globalPosition)
  {
    gp_Trsf aPosTransform;
    aPosTransform.SetTranslation(gp_Pnt(currentPos.x, currentPos.y, currentPos.z), gp_Pnt(globalPosition.x, globalPosition.y, globalPosition.z));
    ioShape = BRepBuilderAPI_Transform(ioShape, aPosTransform, true).Shape();
  }
}

void PartGui::DlgProjectionOnSurface::on_pushButtonAddProjFace_clicked()
{
  if (ui->pushButtonAddProjFace->isChecked())
  {
    m_currentSelection = "add_projection_surface";
    disable_ui_elements(m_guiObjectVec, ui->pushButtonAddProjFace);
    if (!filterFace)
    {
      filterFace = new FaceSelection();
      Gui::Selection().addSelectionGate(filterFace);
    }
  }
  else
  {
    m_currentSelection = "";
    enable_ui_elements(m_guiObjectVec, nullptr);
    Gui::Selection().rmvSelectionGate();
    filterFace = nullptr;
  }
}
void PartGui::DlgProjectionOnSurface::on_radioButtonShowAll_clicked()
{
  m_currentShowType = "all";
  show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::on_radioButtonFaces_clicked()
{
  m_currentShowType = "faces";
  show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::on_radioButtonEdges_clicked()
{
  m_currentShowType = "edges";
  show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::on_doubleSpinBoxExtrudeHeight_valueChanged(double arg1)
{
  Q_UNUSED(arg1);
  create_face_extrude(m_shapeVec);
  show_projected_shapes(m_shapeVec);
}

void PartGui::DlgProjectionOnSurface::on_pushButtonAddWire_clicked()
{
  if (ui->pushButtonAddWire->isChecked())
  {
    m_currentSelection = "add_wire";
    disable_ui_elements(m_guiObjectVec, ui->pushButtonAddWire);
    if (!filterEdge)
    {
      filterEdge = new EdgeSelection();
      Gui::Selection().addSelectionGate(filterEdge);
    }
    ui->radioButtonEdges->setChecked(true);
    on_radioButtonEdges_clicked();
  }
  else
  {
    m_currentSelection = "";
    enable_ui_elements(m_guiObjectVec, nullptr);
    Gui::Selection().rmvSelectionGate();
    filterEdge = nullptr;
  }
}

void PartGui::DlgProjectionOnSurface::on_doubleSpinBoxSolidDepth_valueChanged(double arg1)
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
{
  widget = new DlgProjectionOnSurface();
  taskbox = new Gui::TaskView::TaskBox(
    Gui::BitmapFactory().pixmap("Part_ProjectionOnSurface"),
    widget->windowTitle(), true, nullptr);
  taskbox->groupLayout()->addWidget(widget);
  Content.push_back(taskbox);
}

TaskProjectionOnSurface::~TaskProjectionOnSurface()
{
  // automatically deleted in the sub-class
}

bool TaskProjectionOnSurface::accept()
{
  widget->apply();
  return true;
  //return (widget->result() == QDialog::Accepted);
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

    };
  }
}

#include "moc_DlgProjectionOnSurface.cpp"
