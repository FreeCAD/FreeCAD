/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_TASKELEMENTCOLORS_H
#define GUI_TASKELEMENTCOLORS_H

#include <QListWidgetItem>
#include "TaskView/TaskView.h"
#include "TaskView/TaskDialog.h"

namespace Gui {
class Document;
class ViewProvider;
class ViewProviderDocumentObject;

class GuiExport ElementColors : public QWidget, public SelectionObserver
{
    Q_OBJECT

public:
    ElementColors(ViewProviderDocumentObject* vp, bool noHide=false);
    ~ElementColors();

    bool accept();
    bool reject();

private Q_SLOTS:
    void on_removeSelection_clicked();
    void on_addSelection_clicked();
    void on_removeAll_clicked();
    void on_elementList_itemDoubleClicked(QListWidgetItem *item);
    void on_elementList_itemSelectionChanged();
    void on_elementList_itemEntered(QListWidgetItem *item);
    void on_recompute_clicked(bool checked);
    void on_onTop_clicked(bool checked);
    void on_hideSelection_clicked();
    void on_boxSelect_clicked();

protected:
    void onSelectionChanged(const SelectionChanges& msg);
    void changeEvent(QEvent *e);
    void leaveEvent(QEvent *);
    void slotDeleteDocument(const Document&);
    void slotDeleteObject(const ViewProvider&);
private:
    class Private;
    Private *d;
};

class GuiExport TaskElementColors : public TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskElementColors(ViewProviderDocumentObject* vp, bool noHide=false);
    ~TaskElementColors();

public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);

    QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    ElementColors* widget;
    TaskView::TaskBox* taskbox;
};

} //namespace Gui

#endif // GUI_TASKELEMENTCOLORS_H
