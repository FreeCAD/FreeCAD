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
#include "TaskView/TaskDialog.h"
#include "TaskView/TaskView.h"


namespace Gui {
class Document;
class ViewProvider;
class ViewProviderDocumentObject;

class GuiExport ElementColors : public QWidget, public SelectionObserver
{
    Q_OBJECT

public:
    explicit ElementColors(ViewProviderDocumentObject* vp, bool noHide=false);
    ~ElementColors() override;

    bool accept();
    bool reject();

private:
    void setupConnections();
    void onRemoveSelectionClicked();
    void onAddSelectionClicked();
    void onRemoveAllClicked();
    void onElementListItemDoubleClicked(QListWidgetItem *item);
    void onElementListItemSelectionChanged();
    void onElementListItemEntered(QListWidgetItem *item);
    void onRecomputeClicked(bool checked);
    void onTopClicked(bool checked);
    void onHideSelectionClicked();
    void onBoxSelectClicked();

protected:
    void onSelectionChanged(const SelectionChanges& msg) override;
    void changeEvent(QEvent *e) override;
    void leaveEvent(QEvent *) override;
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
    explicit TaskElementColors(ViewProviderDocumentObject* vp, bool noHide=false);
    ~TaskElementColors() override;

public:
    void open() override;
    bool accept() override;
    bool reject() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    ElementColors* widget;
    TaskView::TaskBox* taskbox;
};

} //namespace Gui

#endif // GUI_TASKELEMENTCOLORS_H
