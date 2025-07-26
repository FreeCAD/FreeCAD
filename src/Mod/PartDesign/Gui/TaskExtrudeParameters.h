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

#ifndef GUI_TASKVIEW_TaskExtrudeParameters_H
#define GUI_TASKVIEW_TaskExtrudeParameters_H

#include "TaskSketchBasedParameters.h"
#include "ViewProviderExtrude.h"

class QComboBox;
class QCheckBox;
class QLineEdit;
class QListWidget;
class QToolButton;

class Ui_TaskPadPocketParameters;

namespace App {
class Property;
class PropertyLinkSubList;
}

namespace PartDesign {
class ProfileBased;
}

namespace PartDesignGui {


class TaskExtrudeParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

    enum DirectionModes {
        Normal,
        Select,
        Custom,
        Reference
    };

public:
    enum class Type {
        Pad,
        Pocket
    };

    enum class SidesMode {
        OneSide,
        TwoSides,
        Symmetric,
    };

    enum class Sides {
        First,
        Second,
    };

    enum class Mode {
        Dimension,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        ToShape,
    };

    enum SelectionMode { 
        None,
        SelectFace,
        SelectShape,
        SelectShapeFaces,
        SelectFace2,
        SelectShape2,
        SelectShapeFaces2,
        SelectReferenceAxis
    };

    TaskExtrudeParameters(ViewProviderExtrude *ExtrudeView, QWidget *parent,
                          const std::string& pixmapname, const QString& parname);
    ~TaskExtrudeParameters() override;

    void saveHistory() override;

    void fillDirectionCombo();
    void addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname, QString itemText,
        bool hasSketch = true);
    void applyParameters();

    void setSelectionMode(SelectionMode mode);

protected Q_SLOTS:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onOffset2Changed(double);
    void onTaperChanged(double);
    void onTaper2Changed(double);
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onDirectionToggled(bool);
    void onAllFacesToggled(bool);
    void onAllFaces2Toggled(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onReversedChanged(bool);
    void onFaceName(const QString& text);
    void onFaceName2(const QString& text);
    void onSelectFaceToggle(const bool checked = true);
    void onSelectShapeToggle(const bool checked = true);
    void onSelectShapeFacesToggle(const bool checked);
    void onUnselectShapeFacesTrigger();
    void onSelectFace2Toggle(const bool checked = true);
    void onSelectShape2Toggle(const bool checked = true);
    void onSelectShapeFaces2Toggle(const bool checked);
    void onUnselectShapeFaces2Trigger();

    void onSidesModeChanged(int);
    virtual void onModeChanged(int);
    virtual void onMode2Changed(int);

protected:
    void setCheckboxes(Type type, Sides side);
    void setupDialog();
    void readValuesFromHistory();
    void changeEvent(QEvent *e) override;
    App::PropertyLinkSub* propReferenceAxis;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

    double getOffset() const;
    double getOffset2() const;
    bool   getAlongSketchNormal() const;
    bool   getCustom() const;
    std::string getReferenceAxis() const;
    double getXDirection() const;
    double getYDirection() const;
    double getZDirection() const;
    bool   getReversed() const;
    int    getMode() const;
    int    getMode2() const;
    int    getSidesMode() const;
    QString getFaceName(QLineEdit*) const;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void translateSidesList(int index);
    virtual void translateModeList(QComboBox* box, int index);
    virtual void updateUI(Sides side);
    void updateDirectionEdits();
    void setDirectionMode(int index);
    void handleLineFaceNameClick(QLineEdit*);
    void handleLineFaceNameNo(QLineEdit*);

private:
    void selectedReferenceAxis(const Gui::SelectionChanges& msg);
    void selectedFace(const Gui::SelectionChanges& msg,
                      QLineEdit* lineEdit,
                      QToolButton* btn,
                      App::PropertyLinkSub& prop);
    void selectedShape(const Gui::SelectionChanges& msg,
                       QLineEdit* lineEdit,
                       QListWidget* list,
                       QCheckBox* box,
                       App::PropertyLinkSubList& prop);
    void selectedShapeFace(const Gui::SelectionChanges& msg,
                           QListWidget* list,
                           App::PropertyLinkSubList& prop);

    void tryRecomputeFeature();
    void translateFaceName(QLineEdit*);
    void connectSlots();
    bool hasProfileFace(PartDesign::ProfileBased*) const;
    void clearFaceName(QLineEdit*);

    void updateShapeName(QLineEdit*, App::PropertyLinkSubList&);
    void updateShapeFaces(QListWidget* list, App::PropertyLinkSubList& prop);

    std::vector<std::string> getShapeFaces(App::PropertyLinkSubList& prop);

    void changeFaceName(QLineEdit* lineEdit, const QString& text);
    void selectShapeToggle(const bool checked, QLineEdit*, App::PropertyLinkSubList&, SelectionMode);
    void selectFaceToggle(const bool checked, QLineEdit*, SelectionMode);
    void unselectShapeFacesTrigger(QListWidget*, App::PropertyLinkSubList&);
    void selectShapeFacesToggle(bool checked, SelectionMode, QToolButton*);

protected:
    QWidget* proxy;
    QAction* unselectShapeFaceAction;
    QAction* unselectShapeFaceAction2;

    std::unique_ptr<Ui_TaskPadPocketParameters> ui;
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;

    SelectionMode selectionMode = None;
};

class TaskDlgExtrudeParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgExtrudeParameters(PartDesignGui::ViewProviderExtrude *vp);
    ~TaskDlgExtrudeParameters() override = default;

    bool accept() override;
    bool reject() override;

protected:
    virtual TaskExtrudeParameters* getTaskParameters() = 0;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskExtrudeParameters_H
