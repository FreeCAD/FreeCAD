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

#ifndef PARTGUI_DLGBOOLEANOPERATION_H
#define PARTGUI_DLGBOOLEANOPERATION_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <boost_signals2.hpp>

class QTreeWidgetItem;

namespace App {
class DocumentObject;
class Property;
}
namespace PartGui {

typedef boost::signals2::connection Connection;
class Ui_DlgBooleanOperation;
class DlgBooleanOperation : public QWidget
{
    Q_OBJECT

public:
    DlgBooleanOperation(QWidget* parent = nullptr);
    ~DlgBooleanOperation();
    void accept();

private:
    void findShapes();
    bool indexOfCurrentItem(QTreeWidgetItem*, int&, int&) const;
    void slotCreatedObject(const App::DocumentObject&);
    void slotChangedObject(const App::DocumentObject&, const App::Property&);
    bool hasSolids(const App::DocumentObject*) const;

protected:
    void changeEvent(QEvent *e);

public Q_SLOTS:
    void on_swapButton_clicked();

private Q_SLOTS:
    void currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*);

private:
    std::unique_ptr<Ui_DlgBooleanOperation> ui;
    Connection connectNewObject;
    Connection connectModObject;
    std::list<const App::DocumentObject*> observe;
};

class TaskBooleanOperation : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskBooleanOperation();
    ~TaskBooleanOperation();

public:
    void clicked(int);

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Apply | QDialogButtonBox::Close; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }
    virtual bool needsFullSpace() const
    { return true; }

private:
    DlgBooleanOperation* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_DLGBOOLEANOPERATION_H
