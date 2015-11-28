/***************************************************************************
 *   Copyright (c) 2015 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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


#ifndef GUI_TASKVIEW_TaskSketcherSolverAdvanced_H
#define GUI_TASKVIEW_TaskSketcherSolverAdvanced_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost/signals.hpp>

class Ui_TaskSketcherSolverAdvanced;

namespace App {
class Property;
}

namespace SketcherGui { 

class ViewProviderSketch;

class TaskSketcherSolverAdvanced : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskSketcherSolverAdvanced(ViewProviderSketch *sketchView);
    ~TaskSketcherSolverAdvanced();

private Q_SLOTS:
    void on_comboBoxDefaultSolver_currentIndexChanged(int index); 
    void on_comboBoxDogLegGaussStep_currentIndexChanged(int index);    
    void on_spinBoxMaxIter_valueChanged(int i);
    void on_checkBoxSketchSizeMultiplier_stateChanged(int state);    
    void on_lineEditConvergence_editingFinished();
    void on_comboBoxQRMethod_currentIndexChanged(int index);
    void on_lineEditQRPivotThreshold_editingFinished();
    void on_comboBoxRedundantDefaultSolver_currentIndexChanged(int index);
    void on_lineEditRedundantConvergence_editingFinished();
    void on_spinBoxRedundantSolverMaxIterations_valueChanged(int i);
    void on_checkBoxRedundantSketchSizeMultiplier_stateChanged(int state);
    void on_comboBoxDebugMode_currentIndexChanged(int index);
    void on_lineEditSolverParam1_editingFinished();
    void on_lineEditRedundantSolverParam1_editingFinished();
    void on_lineEditSolverParam2_editingFinished();
    void on_lineEditRedundantSolverParam2_editingFinished();
    void on_lineEditSolverParam3_editingFinished();
    void on_lineEditRedundantSolverParam3_editingFinished();
    void on_pushButtonDefaults_clicked(bool checked = false);
    void on_pushButtonSolve_clicked(bool checked = false);

protected:
    void updateDefaultMethodParameters(void);
    void updateRedundantMethodParameters(void);
    void updateSketchObject(void); 
protected:
    ViewProviderSketch *sketchView;

private:
    QWidget* proxy;
    Ui_TaskSketcherSolverAdvanced* ui;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TaskSketcherSolverAdvanced_H
