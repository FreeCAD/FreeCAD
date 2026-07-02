// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Wanderer Fan <wandererfan@gmail.com>               *
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

#include <QDialog>
#include <string>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/FeatureFlex.h>

class TopoDS_Shape;

namespace PartGui
{

class Ui_DlgFlex;
class DlgFlex: public QDialog
{
    Q_OBJECT

public:
    DlgFlex(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgFlex() = default;
    void accept() override;
    void apply();
    void reject() override;

    std::vector<App::DocumentObject*> getShapesToFlex() const;

    bool validate();

    void writeParametersToFeature(App::DocumentObject& feature, App::DocumentObject* base) const;

protected:
    void findShapes();
    bool canFlex(const TopoDS_Shape&) const;
    void findEdges();
    bool isEdge(const TopoDS_Shape& shape) const;
    void changeEvent(QEvent* e) override;

private:
    void setupConnections();
    void onModeChanged(int index);

private:
    /// returns link to any of selected source shapes. Throws if nothing is selected for scaling.
    App::DocumentObject& getShapeToFlex() const;

    std::unique_ptr<Ui_DlgFlex> ui;
    std::string m_document, m_label;
};

class TaskFlex: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskFlex();

public:
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close;
    }

private:
    DlgFlex* widget;
};

}  // namespace PartGui
