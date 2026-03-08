// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <memory>
#include <vector>

#include <App/DocumentObserver.h>
#include <Base/Vector3D.h>
#include <Gui/TaskView/TaskDialog.h>


class SoGroup;
namespace Sketcher
{
class SketchObject;
}

namespace SketcherGui
{

class Ui_TaskSketcherValidation;
class SketcherValidation: public QWidget
{
    Q_OBJECT

public:
    explicit SketcherValidation(Sketcher::SketchObject* Obj, QWidget* parent = nullptr);
    ~SketcherValidation() override;

protected:
    void changeEvent(QEvent* e) override;

private:
    void setupConnections();
    void onFindButtonClicked();
    void onFixButtonClicked();
    void onHighlightButtonClicked();
    void onFindConstraintClicked();
    void onFixConstraintClicked();
    void onFindReversedClicked();
    void onSwapReversedClicked();
    void onOrientLockEnableClicked();
    void onOrientLockDisableClicked();
    void onDelConstrExtrClicked();
    void onFindDegeneratedClicked();
    void onFixDegeneratedClicked();

private:
    void showPoints(const std::vector<Base::Vector3d>&);
    void hidePoints();

private:
    std::unique_ptr<Ui_TaskSketcherValidation> ui;
    App::WeakPtrT<Sketcher::SketchObject> sketch;
    SoGroup* coincidenceRoot;
};

class TaskSketcherValidation: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskSketcherValidation(Sketcher::SketchObject* Obj);
    ~TaskSketcherValidation() override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Close;
    }
};

}  // namespace SketcherGui
