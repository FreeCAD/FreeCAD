// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <qcolumnview.h>
#include <QString>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Gui/Document.h>

#include <Mod/Measure/App/MeasureBase.h>
#include <Mod/Measure/Gui/ViewProviderMeasureBase.h>


#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection/Selection.h>

#include <fastsignals/connection.h>

namespace MeasureGui
{

class TaskMeasure: public Gui::TaskView::TaskDialog, public Gui::SelectionObserver
{

public:
    TaskMeasure();
    ~TaskMeasure() override;

    void modifyStandardButtons(QDialogButtonBox* box) override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Apply | QDialogButtonBox::Abort | QDialogButtonBox::Reset;
    }

    void invoke();
    void update();
    void closeDialog();
    bool apply();
    bool apply(bool reset);
    bool reject() override;
    void reset();
    void closed() override;

    bool hasSelection();
    void clearSelection();

private:
    void setupShortcuts(QWidget* parent);
    void tryUpdate();
    void updateUnitDropdown(const App::MeasureType* measureType);
    void setUnitFromResultString();
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void onObjectDeleted(const App::DocumentObject& obj);
    void saveMeasurement();
    void quitMeasurement();

    Measure::MeasureBase* _mMeasureObject = nullptr;

    QLineEdit* valueResult {nullptr};
    QComboBox* modeSwitch {nullptr};
    QComboBox* unitSwitch {nullptr};
    QCheckBox* showDelta {nullptr};
    QLabel* showDeltaLabel {nullptr};
    QAction* autoSaveAction {nullptr};
    QAction* newMeasurementBehaviourAction {nullptr};
    QToolButton* mSettings {nullptr};

    fastsignals::connection m_deletedConnection;

    void removeObject();
    void onModeChanged(int index);
    void onUnitChanged(int index);
    void showDeltaChanged(int checkState);
    void autoSaveChanged(bool checked);
    void newMeasurementBehaviourChanged(bool checked);
    void setModeSilent(App::MeasureType* mode);
    App::MeasureType* getMeasureType();
    void enableAnnotateButton(bool state);
    void createObject(const App::MeasureType* measureType);
    void ensureGroup(Measure::MeasureBase* measurement);
    void setDeltaPossible(bool possible);
    void initViewObject(Measure::MeasureBase* measure);
    void updateResultWithUnit();

    // Stores if the mode is explicitly set by the user or implicitly through the selection
    bool explicitMode = false;

    // Stores if delta measures shall be shown
    bool delta = true;
    bool mAutoSave = false;
    QString mLastUnitSelection = QLatin1String("-");
};

}  // namespace MeasureGui
