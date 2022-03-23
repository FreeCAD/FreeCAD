/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef SKETCHERGUI_TASKSKETCHERVALIDATION_H
#define SKETCHERGUI_TASKSKETCHERVALIDATION_H

#include <vector>
#include <memory>
#include <Base/Vector3D.h>
#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Mod/Sketcher/App/SketchAnalysis.h>

class SoGroup;
namespace Sketcher { class SketchObject; } 

namespace SketcherGui {

class Ui_TaskSketcherValidation;
class SketcherValidation : public QWidget
{
    Q_OBJECT

public:
    SketcherValidation(Sketcher::SketchObject* Obj, QWidget* parent = nullptr);
    ~SketcherValidation();

protected:
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void on_findButton_clicked();
    void on_fixButton_clicked();
    void on_highlightButton_clicked();
    void on_findConstraint_clicked();
    void on_fixConstraint_clicked();
    void on_findReversed_clicked();
    void on_swapReversed_clicked();
    void on_orientLockEnable_clicked();
    void on_orientLockDisable_clicked();
    void on_delConstrExtr_clicked();
    void on_findDegenerated_clicked();
    void on_fixDegenerated_clicked();

private:
    void showPoints(const std::vector<Base::Vector3d>&);
    void hidePoints();

private:
    std::unique_ptr<Ui_TaskSketcherValidation> ui;
    App::WeakPtrT<Sketcher::SketchObject> sketch;
    Sketcher::SketchAnalysis sketchAnalyser;
    SoGroup* coincidenceRoot;
};

class TaskSketcherValidation : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSketcherValidation(Sketcher::SketchObject* Obj);
    ~TaskSketcherValidation();
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Close; }
};

} //namespace SketcherGui

#endif // SKETCHERGUI_TASKSKETCHERVALIDATION_H
