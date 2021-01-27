/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKVIEW_TASKAPPERANCE_H
#define GUI_TASKVIEW_TASKAPPERANCE_H

#include "TaskView.h"
#include <Gui/Selection.h>
#include <boost_signals2.hpp>


namespace App {
class Property;
}

namespace Gui {
class ViewProvider;

namespace TaskView {

typedef boost::signals2::connection TaskAppearance_Connection;
class Ui_TaskAppearance;

class TaskAppearance : public TaskBox, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskAppearance(QWidget *parent = 0);
    ~TaskAppearance();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);

private Q_SLOTS:
    void on_changeMode_activated(const QString&);
    void on_changePlot_activated(const QString&);
    void on_spinTransparency_valueChanged(int);
    void on_spinPointSize_valueChanged(int);
    void on_spinLineWidth_valueChanged(int);

protected:
    void changeEvent(QEvent *e);

private:
    void slotChangedObject(const Gui::ViewProvider&, const App::Property& Prop);
    void setDisplayModes(const std::vector<Gui::ViewProvider*>&);
    void setPointSize(const std::vector<Gui::ViewProvider*>&);
    void setLineWidth(const std::vector<Gui::ViewProvider*>&);
    void setTransparency(const std::vector<Gui::ViewProvider*>&);
    std::vector<Gui::ViewProvider*> getSelection() const;

private:
    QWidget* proxy;
    Ui_TaskAppearance* ui;
    TaskAppearance_Connection connectChangedObject;
};

} //namespace TaskView
} //namespace Gui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
