/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKVIEW_TaskSketcherMessages_H
#define GUI_TASKVIEW_TaskSketcherMessages_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost/signals.hpp>

class Ui_TaskSketcherMessages;
typedef boost::signals::connection Connection;

namespace App {
class Property;
}

namespace SketcherGui { 

class ViewProviderSketch;

class TaskSketcherMessages : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskSketcherMessages(ViewProviderSketch *sketchView);
    ~TaskSketcherMessages();

    void slotSetUp(QString msg);
    void slotSolved(QString msg);

private Q_SLOTS:
    void on_labelConstrainStatus_linkActivated(const QString &);
    void on_autoUpdate_stateChanged(int state);
    void on_manualUpdate_clicked(bool checked);
    
protected:
    ViewProviderSketch *sketchView;
    Connection connectionSetUp;
    Connection connectionSolved;

private:
    QWidget* proxy;
    Ui_TaskSketcherMessages* ui;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TaskSketcherMessages_H
