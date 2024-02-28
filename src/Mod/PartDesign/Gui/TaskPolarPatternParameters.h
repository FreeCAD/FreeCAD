/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#ifndef GUI_TASKVIEW_TaskPolarPatternParameters_H
#define GUI_TASKVIEW_TaskPolarPatternParameters_H

#include "TaskTransformedParameters.h"
#include "ViewProviderPolarPattern.h"


class QTimer;
class Ui_TaskPolarPatternParameters;

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
}

namespace PartDesignGui
{

class TaskMultiTransformParameters;

class TaskPolarPatternParameters: public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    explicit TaskPolarPatternParameters(ViewProviderTransformed* TransformedView,
                                        QWidget* parent = nullptr);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskPolarPatternParameters(TaskMultiTransformParameters* parentTask, QWidget* parameterWidget);
    ~TaskPolarPatternParameters() override;

    void apply() override;

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private Q_SLOTS:
    void onUpdateViewTimer();
    void onAxisChanged(int num);
    void onModeChanged(int mode);
    void onCheckReverse(bool on);
    void onAngle(double angle);
    void onOffset(double offset);
    void onOccurrences(uint number);
    void onUpdateView(bool /*unused*/) override;

private:
    void setupParameterUI(QWidget* widget) override;
    void retranslateParameterUI(QWidget* widget) override;

    void connectSignals();
    void updateUI();
    void kickUpdateViewTimer() const;
    void adaptVisibilityToMode();

    void getAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    const std::string getStdAxis() const;
    const std::string getAxis() const;
    bool getReverse() const;
    int getMode() const;
    double getAngle() const;
    unsigned getOccurrences() const;

private:
    std::unique_ptr<Ui_TaskPolarPatternParameters> ui;
    QTimer* updateViewTimer = nullptr;

    ComboLinks axesLinks;
};


/// simulation dialog for the TaskView
class TaskDlgPolarPatternParameters: public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgPolarPatternParameters(ViewProviderPolarPattern* PolarPatternView);
};

}  // namespace PartDesignGui

#endif  // GUI_TASKVIEW_TASKAPPERANCE_H
