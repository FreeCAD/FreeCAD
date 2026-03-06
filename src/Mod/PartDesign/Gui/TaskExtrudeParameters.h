// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Gui/Inventor/Draggers/Gizmo.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderExtrude.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QToolButton;

class Ui_TaskPadPocketParameters;

namespace App
{
class Property;
class PropertyLinkSubList;
}  // namespace App
namespace Gui
{
class PrefQuantitySpinBox;
}

namespace Gui
{
class LinearGizmo;
class RotationalGizmo;
class GizmoContainer;
}  // namespace Gui

namespace PartDesign
{
class ProfileBased;
}

namespace PartDesignGui
{


class TaskExtrudeParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

    enum DirectionModes
    {
        Normal,
        Select,
        Custom,
        Reference
    };

public:
    enum class Type
    {
        Pad,
        Pocket
    };

    enum class SidesMode
    {
        OneSide,
        TwoSides,
        Symmetric,
    };

    enum class Side
    {
        First,
        Second,
    };

    enum class Mode
    {
        Dimension,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        ToShape,
    };

    enum SelectionMode
    {
        None,
        SelectFace,
        SelectShape,
        SelectShapeFaces,
        SelectReferenceAxis
    };

    TaskExtrudeParameters(
        ViewProviderExtrude* ExtrudeView,
        QWidget* parent,
        const std::string& pixmapname,
        const QString& parname
    );
    ~TaskExtrudeParameters() override = default;

    void saveHistory() override;

    void fillDirectionCombo();
    void addAxisToCombo(
        App::DocumentObject* linkObj,
        std::string linkSubname,
        QString itemText,
        bool hasSketch = true
    );
    void applyParameters();

    void setSelectionMode(SelectionMode mode, Side side = Side::First);

protected:
    // This struct holds all pointers for one side's UI and properties
    struct SideController
    {
        // UI Widgets
        QComboBox* changeMode = nullptr;
        QLabel* labelLength = nullptr;
        QLabel* labelOffset = nullptr;
        QLabel* labelTaperAngle = nullptr;
        Gui::PrefQuantitySpinBox* lengthEdit = nullptr;
        Gui::PrefQuantitySpinBox* offsetEdit = nullptr;
        Gui::PrefQuantitySpinBox* taperEdit = nullptr;
        QLineEdit* lineFaceName = nullptr;
        QToolButton* buttonFace = nullptr;
        QLineEdit* lineShapeName = nullptr;
        QToolButton* buttonShape = nullptr;
        QListWidget* listWidgetReferences = nullptr;
        QToolButton* buttonShapeFace = nullptr;
        QCheckBox* checkBoxAllFaces = nullptr;
        QWidget* upToShapeList = nullptr;
        QWidget* upToShapeFaces = nullptr;
        QAction* unselectShapeFaceAction = nullptr;

        // Feature Properties
        App::PropertyEnumeration* Type = nullptr;
        App::PropertyLength* Length = nullptr;
        App::PropertyLength* Offset = nullptr;
        App::PropertyAngle* TaperAngle = nullptr;
        App::PropertyLinkSub* UpToFace = nullptr;
        App::PropertyLinkSubList* UpToShape = nullptr;
    };

    SideController m_side1;
    SideController m_side2;

    SideController& getSideController(Side side)
    {
        return (side == Side::First) ? m_side1 : m_side2;
    }

protected Q_SLOTS:
    void onSidesModeChanged(int);
    virtual void onModeChanged(int index, Side side) = 0;

private Q_SLOTS:
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onReversedChanged(bool);

private:
    void onModeChanged_Side1(int index);
    void onModeChanged_Side2(int index);
    void onLengthChanged(double len, Side side);
    void onOffsetChanged(double len, Side side);
    void onTaperChanged(double angle, Side side);
    void onSelectFaceToggle(bool checked, Side side);

    void onFaceName(const QString& text, Side side);
    void onAllFacesToggled(bool checked, Side side);
    void onSelectShapeToggle(bool checked, Side side);
    void onSelectShapeFacesToggle(bool checked, Side side);
    void onUnselectShapeFacesTrigger(Side side);

protected:
    void updateWholeUI(Type type, Side side);
    void updateSideUI(
        const SideController& s,
        Type featureType,
        Mode sideMode,
        bool isParentVisible,
        bool setFocus
    );
    void setupDialog();
    void readValuesFromHistory();
    void changeEvent(QEvent* e) override;
    App::PropertyLinkSub* propReferenceAxis;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

    double getOffset() const;
    double getOffset2() const;
    bool getAlongSketchNormal() const;
    bool getCustom() const;
    std::string getReferenceAxis() const;
    double getXDirection() const;
    double getYDirection() const;
    double getZDirection() const;
    bool getReversed() const;
    int getMode() const;
    int getMode2() const;
    int getSidesMode() const;
    QString getFaceName(QLineEdit*) const;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void translateSidesList(int index);
    virtual void translateModeList(QComboBox* box, int index);
    virtual void updateUI(Side side);
    void updateDirectionEdits();
    void setDirectionMode(int index);
    void handleLineFaceNameClick(QLineEdit*);
    void handleLineFaceNameNo(QLineEdit*);

private:
    void setupSideDialog(SideController& side);

    void selectedReferenceAxis(const Gui::SelectionChanges& msg);
    void selectedFace(const Gui::SelectionChanges& msg, SideController& side);
    void selectedShape(const Gui::SelectionChanges& msg, SideController& side);
    void selectedShapeFace(const Gui::SelectionChanges& msg, SideController& side);

    void tryRecomputeFeature();
    void translateFaceName(QLineEdit*);
    void connectSlots();
    bool hasProfileFace(PartDesign::ProfileBased*) const;
    void clearFaceName(QLineEdit*);

    void updateShapeName(QLineEdit*, App::PropertyLinkSubList&);
    void updateShapeFaces(QListWidget* list, App::PropertyLinkSubList& prop);

    std::vector<std::string> getShapeFaces(App::PropertyLinkSubList& prop);

    void changeFaceName(QLineEdit* lineEdit, const QString& text);

    void createSideControllers();

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* lengthGizmo1 = nullptr;
    Gui::LinearGizmo* lengthGizmo2 = nullptr;
    Gui::RotationGizmo* taperAngleGizmo1 = nullptr;
    Gui::RotationGizmo* taperAngleGizmo2 = nullptr;
    void setupGizmos();
    void setGizmoPositions();

protected:
    QWidget* proxy;
    QAction* unselectShapeFaceAction;
    QAction* unselectShapeFaceAction2;

    std::unique_ptr<Ui_TaskPadPocketParameters> ui;
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;

    SelectionMode selectionMode = None;
    Side activeSelectionSide = Side::First;
};

class TaskDlgExtrudeParameters: public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgExtrudeParameters(PartDesignGui::ViewProviderExtrude* vp);
    ~TaskDlgExtrudeParameters() override = default;

    bool accept() override;
    bool reject() override;

protected:
    virtual TaskExtrudeParameters* getTaskParameters() = 0;
};

}  // namespace PartDesignGui
