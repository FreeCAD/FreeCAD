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
#ifndef PARTGUI_DLGPROJECTIONONSURFACE_H
#define PARTGUI_DLGPROJECTIONONSURFACE_H

#include "gp_Dir.hxx"
#include "TopoDS_Edge.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Shape.hxx"
#include "TopoDS_Wire.hxx"

#include <QDoubleSpinBox>
#include <QWidget>
#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/PartFeature.h>


namespace PartGui {

  class Ui_DlgProjectionOnSurface;

  namespace Ui {
    class DlgProjectionOnSurface;
  }

class DlgProjectionOnSurface : public QWidget,
                               public Gui::SelectionObserver,
                               public App::DocumentObserver
{
    Q_OBJECT

public:
    explicit DlgProjectionOnSurface(QWidget *parent = nullptr);
    ~DlgProjectionOnSurface() override;

    void apply();
    void reject();

private:
    void setupConnections();
    void onPushButtonAddFaceClicked();
    void onPushButtonAddEdgeClicked();
    void onPushButtonGetCurrentCamDirClicked();
    void onPushButtonDirXClicked();
    void onPushButtonDirYClicked();
    void onPushButtonDirZClicked();
    void onPushButtonAddProjFaceClicked();
    void onRadioButtonShowAllClicked();
    void onRadioButtonFacesClicked();
    void onRadioButtonEdgesClicked();
    void onDoubleSpinBoxExtrudeHeightValueChanged(double arg1);
    void onPushButtonAddWireClicked();
    void onDoubleSpinBoxSolidDepthValueChanged(double arg1);

private:

  struct  SShapeStore
  {
    TopoDS_Shape inputShape;
    TopoDS_Face surfaceToProject;
    gp_Dir aProjectionDir;
    TopoDS_Face aFace;
    TopoDS_Edge aEdge;
    std::vector<TopoDS_Wire> aWireVec;
    std::vector<TopoDS_Wire> aProjectedWireVec;
    std::vector<TopoDS_Edge> aProjectedEdgeVec;
    std::vector<TopoDS_Wire> aProjectedWireInParametricSpaceVec;
    TopoDS_Face aProjectedFace;
    TopoDS_Shape aProjectedSolid;
    Part::Feature* partFeature = nullptr;
    std::string partName;
    bool is_selectable = false;
    long transparency = 0;
    float exrudeValue = 0.0f;
  };

  //from Gui::SelectionObserver
  void onSelectionChanged(const Gui::SelectionChanges& msg) override;


  void get_camera_direction();
  void store_current_selected_parts(std::vector<SShapeStore>& iStoreVec, const unsigned int iColor);
  bool store_part_in_vector(SShapeStore& iCurrentShape, std::vector<SShapeStore>& iStoreVec);
  void create_projection_wire(std::vector<SShapeStore>& iCurrentShape);
  TopoDS_Shape create_compound(const std::vector<SShapeStore>& iShapeVec);
  void show_projected_shapes(const std::vector<SShapeStore>& iShapeStoreVec);
  void disable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis);
  void enable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis);
  void higlight_object(Part::Feature* iCurrentObject, const std::string& iShapeName, bool iHighlight, const unsigned int iColor);
  void get_all_wire_from_face(SShapeStore& ioCurrentSahpe);
  void create_projection_face_from_wire(std::vector<SShapeStore>& iCurrentShape);
  TopoDS_Wire sort_and_heal_wire(const TopoDS_Shape& iShape, const TopoDS_Face& iFaceToProject);
  TopoDS_Wire sort_and_heal_wire(const std::vector<TopoDS_Edge>& iEdgeVec, const TopoDS_Face& iFaceToProject);
  void create_face_extrude(std::vector<SShapeStore>& iCurrentShape);
  void store_wire_in_vector(const SShapeStore& iCurrentShape, const TopoDS_Shape& iParentShape, std::vector<SShapeStore>& iStoreVec, const unsigned int iColor);
  void set_xyz_dir_spinbox(QDoubleSpinBox* icurrentSpinBox);
  void transform_shape_to_global_position(TopoDS_Shape& ioShape, Part::Feature* iPart);

private:
  /** Checks if the given document is about to be closed */
  void slotDeletedDocument(const App::Document& Doc) override;
  /** Checks if the given object is about to be removed. */
  void slotDeletedObject(const App::DocumentObject& Obj) override;

private:
    Ui::DlgProjectionOnSurface *ui;
    std::vector<SShapeStore> m_shapeVec;
    std::vector<SShapeStore> m_projectionSurfaceVec;

    std::string m_currentSelection;
    std::string m_currentShowType;

    std::vector<QWidget*> m_guiObjectVec;

    const QString m_projectionObjectName;
    Part::Feature* m_projectionObject;
    App::Document* m_partDocument;
    float m_lastDepthVal;

    class EdgeSelection;
    EdgeSelection* filterEdge;

    class FaceSelection;
    FaceSelection* filterFace;
};

class TaskProjectionOnSurface : public Gui::TaskView::TaskDialog
{
  Q_OBJECT

public:
  TaskProjectionOnSurface();

public:
  bool accept() override;
  bool reject() override;
  void clicked(int) override;

  QDialogButtonBox::StandardButtons getStandardButtons() const override
  {
    return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
  }

private:
  DlgProjectionOnSurface* widget;
  Gui::TaskView::TaskBox* taskbox;
};


} // namespace PartGui
#endif // PARTGUI_DLGPROJECTIONONSURFACE_H
