// SPDX-License-Identifier: LGPL-2.1-or-later

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
#pragma once

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
#include <Mod/Part/App/FeatureProjectOnSurface.h>


namespace PartGui
{

class Ui_DlgProjectionOnSurface;

namespace Ui
{
class DlgProjectionOnSurface;
}

class DlgProjectionOnSurface: public QWidget, public Gui::SelectionObserver, public App::DocumentObserver
{
    Q_OBJECT

public:
    explicit DlgProjectionOnSurface(QWidget* parent = nullptr);
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
    struct SShapeStore
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
        double extrudeValue = 0.0;
    };

    // from Gui::SelectionObserver
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;


    void get_camera_direction();
    void store_current_selected_parts(std::vector<SShapeStore>& iStoreVec, unsigned int iColor);
    bool store_part_in_vector(SShapeStore& iCurrentShape, std::vector<SShapeStore>& iStoreVec);
    void create_projection_wire(std::vector<SShapeStore>& iCurrentShape);
    TopoDS_Shape create_compound(const std::vector<SShapeStore>& iShapeVec);
    void show_projected_shapes(const std::vector<SShapeStore>& iShapeStoreVec);
    void disable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis);
    void enable_ui_elements(const std::vector<QWidget*>& iObjectVec, QWidget* iExceptThis);
    void higlight_object(
        Part::Feature* iCurrentObject,
        const std::string& iShapeName,
        bool iHighlight,
        unsigned int iColor
    );
    void get_all_wire_from_face(SShapeStore& ioCurrentSahpe);
    void create_projection_face_from_wire(std::vector<SShapeStore>& iCurrentShape);
    TopoDS_Wire sort_and_heal_wire(const TopoDS_Shape& iShape, const TopoDS_Face& iFaceToProject);
    TopoDS_Wire sort_and_heal_wire(
        const std::vector<TopoDS_Edge>& iEdgeVec,
        const TopoDS_Face& iFaceToProject
    );
    void create_face_extrude(std::vector<SShapeStore>& iCurrentShape);
    void store_wire_in_vector(
        const SShapeStore& iCurrentShape,
        const TopoDS_Shape& iParentShape,
        std::vector<SShapeStore>& iStoreVec,
        unsigned int iColor
    );
    void set_xyz_dir_spinbox(QDoubleSpinBox* icurrentSpinBox);
    void transform_shape_to_global_position(TopoDS_Shape& ioShape, Part::Feature* iPart);

private:
    /** Checks if the given document is about to be closed */
    void slotDeletedDocument(const App::Document& Doc) override;
    /** Checks if the given object is about to be removed. */
    void slotDeletedObject(const App::DocumentObject& Obj) override;

private:
    Ui::DlgProjectionOnSurface* ui;
    std::vector<SShapeStore> m_shapeVec;
    std::vector<SShapeStore> m_projectionSurfaceVec;

    std::string m_currentSelection;
    std::string m_currentShowType;

    std::vector<QWidget*> m_guiObjectVec;

    const QString m_projectionObjectName;
    Part::Feature* m_projectionObject = nullptr;
    App::Document* m_partDocument = nullptr;
    double m_lastDepthVal;

    Gui::SelectionFilterGate* filterEdge;
    Gui::SelectionFilterGate* filterFace;
};

class TaskProjectionOnSurface: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskProjectionOnSurface();

public:
    bool accept() override;
    bool reject() override;
    void clicked(int id) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    DlgProjectionOnSurface* widget = nullptr;
    Gui::TaskView::TaskBox* taskbox = nullptr;
};

// ------------------------------------------------------------------------------------------------

class DlgProjectOnSurface: public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit DlgProjectOnSurface(Part::ProjectOnSurface* feature, QWidget* parent = nullptr);
    ~DlgProjectOnSurface() override;

    void accept();
    void reject();

    // from Gui::SelectionObserver
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    enum SelectionMode
    {
        None,
        SupportFace,
        AddFace,
        AddWire,
        AddEdge
    };
    void setupConnections();
    void onAddFaceClicked();
    void onAddEdgeClicked();
    void onGetCurrentCamDirClicked();
    void onDirXClicked();
    void onDirYClicked();
    void onDirZClicked();
    void onAddProjFaceClicked();
    void onShowAllClicked();
    void onFacesClicked();
    void onEdgesClicked();
    void onExtrudeHeightValueChanged(double arg1);
    void onAddWireClicked();
    void onSolidDepthValueChanged(double arg1);
    void setDirection();
    void addWire(const Gui::SelectionChanges& msg);
    void addSelection(const Gui::SelectionChanges& msg);
    void addSelection(const Gui::SelectionChanges& msg, const std::string& subName);
    void setSupportFace(const Gui::SelectionChanges& msg);
    void fetchDirection();
    void fetchMode();

private:
    std::unique_ptr<Ui::DlgProjectionOnSurface> ui;
    Gui::SelectionFilterGate* filterEdge;
    Gui::SelectionFilterGate* filterFace;
    App::WeakPtrT<Part::ProjectOnSurface> feature;
    SelectionMode selectionMode = SelectionMode::None;
};

class TaskProjectOnSurface: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskProjectOnSurface(App::Document*);
    explicit TaskProjectOnSurface(Part::ProjectOnSurface*);

public:
    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    void resetEdit();

private:
    DlgProjectOnSurface* widget = nullptr;
    Gui::TaskView::TaskBox* taskbox = nullptr;
};


}  // namespace PartGui
