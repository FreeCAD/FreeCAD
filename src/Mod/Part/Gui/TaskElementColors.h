/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef PARTGUI_TASKELEMENTCOLORS_H
#define PARTGUI_TASKELEMENTCOLORS_H

#include <QListWidgetItem>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Gui {
    class Document;
    class ViewProvider;
}

namespace PartGui { 

class ViewProviderPartExt;

class ElementColors : public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    ElementColors(ViewProviderPartExt* vp, QWidget* parent = 0);
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

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void changeEvent(QEvent *e);
    void leaveEvent(QEvent *);
    void slotDeleteDocument(const Gui::Document&);
    void slotDeleteObject(const Gui::ViewProvider&);
private:
    class Private;
    std::unique_ptr<Private> d;
};

class TaskElementColors : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskElementColors(ViewProviderPartExt* vp);
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
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace PartGui

#endif // PARTGUI_TASKELEMENTCOLORS_H
