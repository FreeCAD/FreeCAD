/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_TASKSETCOLORS_H
#define PARTGUI_TASKSETCOLORS_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>

namespace Gui {
    class Document;
    class ViewProvider;
}

namespace PartGui { 

class ViewProviderPartExt;

class FaceColors : public QWidget, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    FaceColors(ViewProviderPartExt* vp, QWidget* parent = 0);
    ~FaceColors();

    void open();
    bool accept();
    bool reject();

private Q_SLOTS:
    void on_colorButton_changed();
    void on_defaultButton_clicked();
    void on_boxSelection_clicked();

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void changeEvent(QEvent *e);
    void slotUndoDocument(const Gui::Document& Doc);
    void slotDeleteDocument(const Gui::Document&);
    void slotDeleteObject(const Gui::ViewProvider&);
    void updatePanel();

private:
    class Private;
    Private* d;
};

class TaskFaceColors : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskFaceColors(ViewProviderPartExt* vp);
    ~TaskFaceColors();

public:
    void open();
    bool accept();
    bool reject();
    void clicked(int);

    QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    FaceColors* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace PartGui

#endif // PARTGUI_TASKSETCOLORS_H
