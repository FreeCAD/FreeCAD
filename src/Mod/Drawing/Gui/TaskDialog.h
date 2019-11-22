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


#ifndef DRAWINGGUI_TASKDIALOG
#define DRAWINGGUI_TASKDIALOG

#include <QWidget>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class QCheckBox;

namespace DrawingGui
{

/**
 * Embed the panel into a task dialog.
 */
class TaskProjection : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskProjection();
    ~TaskProjection();

public:
    bool accept();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }

private:
    QWidget* widget;
    std::vector<QCheckBox*> boxes;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace DrawingGui



#endif // DRAWINGGUI_TASKDIALOG
