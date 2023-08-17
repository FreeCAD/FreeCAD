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
#include "ViewProviderSketchBased.h"


class Ui_TaskPadPocketParameters;

namespace App {
class Property;
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
    TaskExtrudeParameters(ViewProviderSketchBased *SketchBasedView, QWidget *parent,
                          const std::string& pixmapname, const QString& parname);
    ~TaskExtrudeParameters() override;

    void saveHistory() override;

    void fillDirectionCombo();
    void addAxisToCombo(App::DocumentObject* linkObj, std::string linkSubname, QString itemText,
        bool hasSketch = true);
    void applyParameters(QString facename);

    enum class Type {
        Pad,
        Pocket
    };
    enum class Modes {
        Dimension,
        ThroughAll,
        ToLast = ThroughAll,
        ToFirst,
        ToFace,
        TwoDimensions
    };

protected Q_SLOTS:
    void onLengthChanged(double);
    void onLength2Changed(double);
    void onOffsetChanged(double);
    void onTaperChanged(double);
    void onTaper2Changed(double);
    void onDirectionCBChanged(int);
    void onAlongSketchNormalChanged(bool);
    void onDirectionToggled(bool);
    void onXDirectionEditChanged(double);
    void onYDirectionEditChanged(double);
    void onZDirectionEditChanged(double);
    void onMidplaneChanged(bool);
    void onReversedChanged(bool);
    void onButtonFace(const bool checked = true);
    void onFaceName(const QString& text);
    virtual void onModeChanged(int);

protected:
    void setCheckboxes(Modes mode, Type type);
    void setupDialog();
    void readValuesFromHistory();
    void changeEvent(QEvent *e) override;
    App::PropertyLinkSub* propReferenceAxis;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;

    double getOffset() const;
    bool   getAlongSketchNormal() const;
    bool   getCustom() const;
    std::string getReferenceAxis() const;
    double getXDirection() const;
    double getYDirection() const;
    double getZDirection() const;
    bool   getReversed() const;
    bool   getMidplane() const;
    int    getMode() const;
    QString getFaceName() const;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    virtual void translateModeList(int index);
    virtual void updateUI(int index);
    void updateDirectionEdits();
    void setDirectionMode(int index);
    void handleLineFaceNameClick();
    void handleLineFaceNameNo();

private:
    void tryRecomputeFeature();
    void translateFaceName();
    void connectSlots();
    bool hasProfileFace(PartDesign::ProfileBased*) const;
    void selectedReferenceAxis(const Gui::SelectionChanges& msg);
    void clearFaceName();

protected:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPadPocketParameters> ui;
    bool selectionFace;
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskExtrudeParameters_H
