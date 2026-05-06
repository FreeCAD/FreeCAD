// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Gui/InputVector.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


namespace PartGui
{

class Ui_DlgRevolution;
class DlgRevolution: public QDialog, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit DlgRevolution(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgRevolution() override;
    void accept() override;

    Base::Vector3d getDirection() const;
    Base::Vector3d getPosition() const;
    void getAxisLink(App::PropertyLinkSub& lnk) const;
    double getAngle() const;

    void setDirection(Base::Vector3d dir);
    void setPosition(Base::Vector3d dir);
    void setAxisLink(const App::PropertyLinkSub& lnk);
    void setAxisLink(const char* objname, const char* subname);

    std::vector<App::DocumentObject*> getShapesToRevolve() const;

    bool validate();

protected:
    void changeEvent(QEvent* e) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    void setupConnections();
    void onSelectLineClicked();
    void onButtonXClicked();
    void onButtonYClicked();
    void onButtonZClicked();
    void onAxisLinkTextChanged(QString);

private:
    void findShapes();
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    /// returns link to any of selected source shapes. Throws if nothing is selected for extrusion.
    App::DocumentObject& getShapeToRevolve() const;

    /// automatically checks Solid checkbox depending on input shape
    void autoSolid();

private:
    std::unique_ptr<Ui_DlgRevolution> ui;
    class EdgeSelection;
    EdgeSelection* filter;
};

class TaskRevolution: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskRevolution();

public:
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    DlgRevolution* widget;
};

}  // namespace PartGui
