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

namespace MeshGui {
class Ui_DlgDecimating;
class DlgDecimating : public QWidget
{
    Q_OBJECT

public:
    DlgDecimating(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgDecimating();
    void setNumberOfTriangles(int);
    double tolerance() const;
    double reduction() const;
    bool isAbsoluteNumber() const;
    int targetNumberOfTriangles() const;

private Q_SLOTS:
    void on_checkAbsolueNumber_toggled(bool);

private:
    int numberOfTriangles;
    std::unique_ptr<Ui_DlgDecimating> ui;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskDecimating : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDecimating();
    ~TaskDecimating();

public:
    bool accept();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }

private:
    DlgDecimating* widget;
};

}

#endif // MESHGUI_DLGDECIMATING_H
