/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_DLGDECIMATING_H
#define MESHGUI_DLGDECIMATING_H

#include <QDialog>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <memory>

namespace MeshGui
{
class Ui_DlgDecimating;
class DlgDecimating: public QWidget
{
    Q_OBJECT

public:
    explicit DlgDecimating(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgDecimating() override;
    void setNumberOfTriangles(int);
    double tolerance() const;
    double reduction() const;
    bool isAbsoluteNumber() const;
    int targetNumberOfTriangles() const;

private:
    void onCheckAbsoluteNumberToggled(bool);

private:
    int numberOfTriangles {0};
    std::unique_ptr<Ui_DlgDecimating> ui;

    Q_DISABLE_COPY_MOVE(DlgDecimating)
};

/**
 * Embed the panel into a task dialog.
 */
class TaskDecimating: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDecimating();

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
    DlgDecimating* widget;
};

}  // namespace MeshGui

#endif  // MESHGUI_DLGDECIMATING_H
