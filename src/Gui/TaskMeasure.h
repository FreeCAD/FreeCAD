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

#include <App/Application.h>

#include <Mod/Measure/App/MeasureBase.h>

#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"
#include "Selection.h"

namespace Gui {

class TaskMeasure : public TaskView::TaskDialog, public Gui::SelectionObserver {

public:
    TaskMeasure();
    ~TaskMeasure() override;

    void modifyStandardButtons(QDialogButtonBox* box) override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override {
        return QDialogButtonBox::Ok | QDialogButtonBox::Abort | QDialogButtonBox::Reset;
    }

    void invoke();
    void update();
    void close();
    bool accept() override;
    bool reject() override;
    void reset();

    void addElement(const char* mod, const char* obName, const char* subName);
    bool hasSelection();
    void clearSelection();
    void gatherSelection();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void setMeasureObject(Measure::MeasureBase* obj);

protected:


private:
    QColumnView* dialog{nullptr};

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    Measure::MeasureBase *_mMeasureObject = nullptr;

    QLineEdit* valueResult{nullptr};
    QLabel* labelResult{nullptr};
    QComboBox* modeSwitch{nullptr};

    void removeObject();
    void onModeChanged(int index);
    void setModeSilent(App::MeasureType* mode);
    App::MeasureType* getMeasureType();
    void enableAnnotateButton(bool state);

    // Store a list of picked elements and subelements
    App::MeasureSelection selection;

    // List of measure types
    std::vector<App::DocumentObject> measureObjects;

    // Stores if the mode is explicitly set by the user or implicitly through the selection
    bool explicitMode = false;

};

} // namespace Gui
