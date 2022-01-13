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


#ifndef GUI_TASKVIEW_TaskSketcherTool_H
#define GUI_TASKVIEW_TaskSketcherTool_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Selection.h>
#include <boost_signals2.hpp>

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace SketcherGui {

class Ui_TaskSketcherTool;
class ViewProviderSketch;

class SketcherToolWidget : public QWidget
{
    Q_OBJECT

public:
    SketcherToolWidget(QWidget *parent=0);
    ~SketcherToolWidget();
    
    //bool eventFilter(QObject *object, QEvent *event);

    void setparameter(double val, int i);
    void setParameterActive(bool val, int i);
    void setParameterFocus(int i);
    void setSettings(int toolSelected);
    std::vector<double> toolParameters;
    std::vector<bool> isSettingSet;

//Q_SIGNALS:
public Q_SLOTS:
    void emitSetparameter(double val);
    void emitSetparameterTwo(double val);
    void emitSetparameterThree(double val);
    void emitSetparameterFour(double val);
    void emitSetparameterFive(double val);

protected:
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_TaskSketcherTool> ui;
};

class TaskSketcherTool : public Gui::TaskView::TaskBox,
                            public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskSketcherTool(ViewProviderSketch *sketchView);
    ~TaskSketcherTool();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);
    SketcherToolWidget* widget;

//public Q_SLOTS:
    //void onSetparameter(double val, int i);
    //void onSetparameterTwo(double val);
    //void onSetparameterThree(double val);
    //void onSetparameterFour(double val);

private:
    void onChangedSketchView(const Gui::ViewProvider&,
                             const App::Property&);

private:
    ViewProviderSketch *sketchView;
    boost::signals2::scoped_connection changedSketchView;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TaskSketcherTool_H
