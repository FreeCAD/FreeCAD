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


#ifndef MESHPARTGUI_TESSELLATION_H
#define MESHPARTGUI_TESSELLATION_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <memory>

class QButtonGroup;

namespace MeshPartGui {

class Ui_Tessellation;
class Tessellation : public QWidget
{
    Q_OBJECT

public:
    Tessellation(QWidget* parent = 0);
    ~Tessellation();
    bool accept();

protected:
    void changeEvent(QEvent *e);

private:
    void findShapes();

private Q_SLOTS:
    void meshingMethod(int id);
    void on_comboFineness_currentIndexChanged(int);
    void on_checkSecondOrder_toggled(bool);
    void on_checkQuadDominated_toggled(bool);

private:
    QString document;
    QButtonGroup* buttonGroup;
    std::auto_ptr<Ui_Tessellation> ui;
};

class TaskTessellation : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskTessellation();
    ~TaskTessellation();

public:
    virtual void open();
    virtual void clicked(int);
    virtual bool accept();
    virtual bool reject();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

private:
    Tessellation* widget;
};

} // namespace MeshPartGui

#endif // MESHPARTGUI_TESSELLATION_H
