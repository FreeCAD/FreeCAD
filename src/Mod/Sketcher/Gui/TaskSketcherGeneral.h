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
#include <boost_signals2.hpp>

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace SketcherGui {

class Ui_TaskSketcherGeneral;
class ViewProviderSketch;

class SketcherGeneralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SketcherGeneralWidget(QWidget *parent=nullptr);
    ~SketcherGeneralWidget() override;
    
    bool eventFilter(QObject *object, QEvent *event) override;

    void saveSettings();
    void saveOrderingOrder();
    void loadSettings();
    void loadOrderingOrder();
    void setGridSize(double val);
    void checkGridView(bool);
    void checkGridSnap(bool);
    void checkAutoconstraints(bool);
    void checkAvoidRedundant(bool);
    bool isCheckedAutoUpdate();
    void enableGridSettings(bool);
    void enableAvoidRedundant(bool);
    void saveAutoUpdate();
    void saveAutoRemoveRedundants();

Q_SIGNALS:
    void emitToggleGridView(bool);
    void emitToggleGridSnap(bool);
    void emitSetGridSize(double);
    void emitToggleAutoconstraints(bool);
    void emitToggleAvoidRedundant(bool);
    void emitToggleAutoUpdate(bool);
    void emitRenderOrderChanged();
    void emitToggleAutoRemoveRedundants(bool);

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui_TaskSketcherGeneral> ui;
};

class TaskSketcherGeneral : public Gui::TaskView::TaskBox,
                            public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit TaskSketcherGeneral(ViewProviderSketch *sketchView);
    ~TaskSketcherGeneral() override;
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason) override;

public Q_SLOTS:
    void onToggleGridView(bool on);
    void onSetGridSize(double val);
    void onToggleGridSnap(bool on);
    void onToggleAutoconstraints(bool on);
    void onToggleAvoidRedundant(bool);
    void onToggleAutoRemoveRedundants(bool);
    void onToggleAutoUpdate(bool);
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
