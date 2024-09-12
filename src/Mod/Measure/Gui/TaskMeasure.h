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
#include <Gui/Selection.h>

namespace Gui
{

class TaskMeasure: public TaskView::TaskDialog, public Gui::SelectionObserver
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
    void close();
    bool apply();
    bool reject() override;
    void reset();

    bool hasSelection();
    void clearSelection();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void setMeasureObject(Measure::MeasureBase* obj);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    App::Document* _mDocument = nullptr;
    Gui::Document* _mGuiDocument = nullptr;
    App::MeasureType* _mMeasureType = nullptr;
    Measure::MeasureBase* _mMeasureObject = nullptr;
    Gui::ViewProviderDocumentObject* _mViewObject = nullptr;

    QLineEdit* valueResult {nullptr};
    QComboBox* modeSwitch {nullptr};
    QCheckBox* showDelta {nullptr};
    QLabel* showDeltaLabel {nullptr};

    void removeObject();
    void onModeChanged(int index);
    void showDeltaChanged(int checkState);
    void setModeSilent(App::MeasureType* mode);
    App::MeasureType* getMeasureType();
    void enableAnnotateButton(bool state);
    App::DocumentObject* createObject(const App::MeasureType* measureType);
    Gui::ViewProviderDocumentObject* createViewObject(App::DocumentObject* measureObj);
    void saveObject();
    void ensureGroup(Measure::MeasureBase* measurement);
    void setDeltaPossible(bool possible);


    // List of measure types
    std::vector<App::DocumentObject> measureObjects;

    // Stores if the mode is explicitly set by the user or implicitly through the selection
    bool explicitMode = false;

    // Stores if delta measures shall be shown
    bool delta = true;
};

}  // namespace Gui
