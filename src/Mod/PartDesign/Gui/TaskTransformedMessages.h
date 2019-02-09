/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKVIEW_TaskTransformedMessages_H
#define GUI_TASKVIEW_TaskTransformedMessages_H

#include <Gui/TaskView/TaskView.h>
#include <boost/signals2.hpp>

class QCheckBox;
class Ui_TaskTransformedMessages;
typedef boost::signals2::connection Connection;

namespace App {
class Property;
}

namespace PartDesignGui { 

class ViewProviderTransformed;

class TaskTransformedMessages : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskTransformedMessages(ViewProviderTransformed *transformedView);
    ~TaskTransformedMessages();

    void slotDiagnosis(QString msg);
    QCheckBox *getCheckBox();

private Q_SLOTS:
    
protected:
    ViewProviderTransformed *transformedView;
    Connection connectionDiagnosis;

private:
    QWidget* proxy;
    Ui_TaskTransformedMessages* ui;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskTransformedMessages_H
