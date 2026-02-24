// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#ifndef MESH_GLOBAL_H
# include <Mod/Mesh/MeshGlobal.h>
#endif

class QButtonGroup;

namespace MeshGui
{

class Selection;
class Ui_DlgSmoothing;
class DlgSmoothing: public QWidget
{
    Q_OBJECT

public:
    enum Smooth
    {
        None,
        Taubin,
        Laplace,
        MedianFilter
    };

    explicit DlgSmoothing(QWidget* parent = nullptr);
    ~DlgSmoothing() override;
    int iterations() const;
    double lambdaStep() const;
    double microStep() const;
    Smooth method() const;
    bool smoothSelection() const;

private:
    void methodClicked(int);
    void onCheckBoxSelectionToggled(bool);

Q_SIGNALS:
    void toggledSelection(bool);

private:
    Ui_DlgSmoothing* ui;
    QButtonGroup* bg;

    Q_DISABLE_COPY_MOVE(DlgSmoothing)
};

/**
 * Embed the panel into a dialog.
 */
class MeshGuiExport SmoothingDialog: public QDialog
{
    Q_OBJECT

public:
    explicit SmoothingDialog(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~SmoothingDialog() override;

    int iterations() const
    {
        return widget->iterations();
    }
    double lambdaStep() const
    {
        return widget->lambdaStep();
    }
    double microStep() const
    {
        return widget->microStep();
    }
    DlgSmoothing::Smooth method() const
    {
        return widget->method();
    }
    bool smoothSelection() const
    {
        return widget->smoothSelection();
    }

private:
    DlgSmoothing* widget;

    Q_DISABLE_COPY_MOVE(SmoothingDialog)
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSmoothing: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSmoothing();

public:
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }
    bool isAllowedAlterDocument() const override
    {
        return true;
    }

private:
    DlgSmoothing* widget;
    Selection* selection;
};

}  // namespace MeshGui
