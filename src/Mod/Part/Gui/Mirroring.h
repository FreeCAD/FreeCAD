/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_MIRRORING_H
#define PARTGUI_MIRRORING_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class QTreeWidgetItem;

namespace App {
class DocumentObject;
class Property;
}
namespace PartGui {

class Ui_Mirroring;
class Mirroring : public QWidget
{
    Q_OBJECT

public:
    explicit Mirroring(QWidget* parent = nullptr);
    ~Mirroring() override;
    bool accept();

protected:
    void changeEvent(QEvent *e) override;

private:
    void findShapes();

private:
    QString document;
    std::unique_ptr<Ui_Mirroring> ui;
};

class TaskMirroring : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskMirroring();

public:
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }
    bool isAllowedAlterDocument() const override
    { return false; }
    bool needsFullSpace() const override
    { return false; }

private:
    Mirroring* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_MIRRORING_H
