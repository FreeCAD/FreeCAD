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


#ifndef GUI_TASKVIEW_TaskSketcherGerneral_H
#define GUI_TASKVIEW_TaskSketcherGerneral_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost/signals2.hpp>

class Ui_TaskSketcherGeneral;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace SketcherGui {

class ViewProviderSketch;

class SketcherGeneralWidget : public QWidget
{
    Q_OBJECT

public:
    SketcherGeneralWidget(QWidget *parent=0);
    ~SketcherGeneralWidget();

    void saveSettings();
    void loadSettings();
    void setGridSize(double val);
    void checkGridView(bool);
    void checkGridSnap(bool);
    void checkAutoconstraints(bool);

    bool isGridViewChecked() const;
    void saveGridViewChecked();

Q_SIGNALS:
    void emitToggleGridView(bool);
    void emitToggleGridSnap(int);
    void emitSetGridSize(double);
    void emitToggleAutoconstraints(int);
    void emitRenderOrderChanged();

private Q_SLOTS:
    void onToggleGridView(bool on);
    void onSetGridSize(double val);
    void onToggleGridSnap(int state);
    void onRenderOrderChanged();
    void on_checkBoxRedundantAutoconstraints_stateChanged(int);

protected:
    void changeEvent(QEvent *e);

private:
    Ui_TaskSketcherGeneral* ui;
};

class TaskSketcherGeneral : public Gui::TaskView::TaskBox,
                            public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskSketcherGeneral(ViewProviderSketch *sketchView);
    ~TaskSketcherGeneral();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);

public Q_SLOTS:
    void onToggleGridView(bool on);
    void onSetGridSize(double val);
    void onToggleGridSnap(int state);
    void onToggleAutoconstraints(int state);
    void onRenderOrderChanged();

private:
    void onChangedSketchView(const Gui::ViewProvider&,
                             const App::Property&);

private:
    ViewProviderSketch *sketchView;
    SketcherGeneralWidget* widget;
    boost::signals2::scoped_connection changedSketchView;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TaskSketcherGerneral_H
