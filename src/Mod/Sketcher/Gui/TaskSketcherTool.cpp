/***************************************************************************
 *   Copyright (c) 2022 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost_bind_bind.hpp>
#endif

#include "ui_TaskSketcherTool.h"
#include "TaskSketcherTool.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <QEvent>

#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Gui::TaskView;
namespace bp = boost::placeholders;

SketcherToolWidget::SketcherToolWidget(QWidget *parent, ViewProviderSketch* sketchView)
  : QWidget(parent), ui(new Ui_TaskSketcherTool), sketchView(sketchView)
{
    ui->setupUi(this);
    
    // connecting the needed signals
    connect(ui->parameterOne, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameter(double)));
    connect(ui->parameterTwo, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameterTwo(double)));
    connect(ui->parameterThree, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameterThree(double)));
    connect(ui->parameterFour, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameterFour(double)));
    connect(ui->parameterFive, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameterFive(double)));
    ui->parameterOne->installEventFilter(this);
    ui->parameterTwo->installEventFilter(this);
    ui->parameterThree->installEventFilter(this);
    ui->parameterFour->installEventFilter(this);
    ui->parameterFive->installEventFilter(this);
    
    isWidgetActive = 0;
}

SketcherToolWidget::~SketcherToolWidget(){}

//pre-select the number of the spinbox when it gets the focus.
bool SketcherToolWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui->parameterOne && event->type() == QEvent::FocusIn) {
        ui->parameterOne->selectNumber();
    }
    else if (object == ui->parameterTwo && event->type() == QEvent::FocusIn) {
        ui->parameterTwo->selectNumber();
    }
    else if (object == ui->parameterThree && event->type() == QEvent::FocusIn) {
        ui->parameterThree->selectNumber();
    }
    else if (object == ui->parameterFour && event->type() == QEvent::FocusIn) {
        ui->parameterFour->selectNumber();
    }
    else if (object == ui->parameterFive && event->type() == QEvent::FocusIn) {
        ui->parameterFive->selectNumber();
    }
    return false;
}

void SketcherToolWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control)
    {
        sketchView->keyPressed(0, SoKeyboardEvent::LEFT_CONTROL);
    }
    if (event->key() == Qt::Key_Shift)
    {
        sketchView->keyPressed(1, SoKeyboardEvent::RIGHT_SHIFT);
    }
    if (event->key() == Qt::Key_Escape)
    {
        sketchView->keyPressed(0, SoKeyboardEvent::ESCAPE);
    }
    //It whould be best to catch all keys than manually mapping them here...
    //But I can't find how to. Maybe with this below function
    //const SoEvent* keyboard->translateEvent(event)
    //sketchView->keyPressed(0, sokey);
    //QWidget::keyReleaseEvent(event);
}


void SketcherToolWidget::emitSetparameter(double val)
{
    isSettingSet[0] = 1;
    toolParameters[0] = val;
    QMetaObject::invokeMethod(ui->parameterTwo, "setFocus", Qt::QueuedConnection);
    ui->parameterTwo->setEnabled(1);

    //Make a mousemove to update geometry.
    if(isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterTwo(double val)
{
    isSettingSet[1] = 1;
    toolParameters[1] = val;
    QMetaObject::invokeMethod(ui->parameterThree, "setFocus", Qt::QueuedConnection);
    ui->parameterOne->setEnabled(0);
    ui->parameterTwo->setEnabled(0);
    ui->parameterThree->setEnabled(1);
    ui->parameterFour->setEnabled(1);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterThree(double val)
{
    isSettingSet[2] = 1;
    toolParameters[2] = val;
    QMetaObject::invokeMethod(ui->parameterFour, "setFocus", Qt::QueuedConnection);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterFour(double val)
{
    isSettingSet[3] = 1;
    toolParameters[3] = val;
    QMetaObject::invokeMethod(ui->parameterFive, "setFocus", Qt::QueuedConnection);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterFive(double val)
{
    isSettingSet[4] = 1;
    toolParameters[4] = val;

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}

void SketcherToolWidget::setSettings(int toolSelected)
{
    //Set names and hide excess parameters
    //For reference unit can be changed with : 
    //setUnit(Base::Unit::Length); Base::Unit::Angle
    switch (toolSelected) {
    case 0: //none, reset and hide all settings
        isWidgetActive = 0;
        toolParameters.clear();
        isSettingSet.clear();

        ui->label->setVisible(0);
        ui->label2->setVisible(0);
        ui->label3->setVisible(0);
        ui->label4->setVisible(0);
        ui->label5->setVisible(0);
        setparameter(0, 0);
        setparameter(0, 1);
        setparameter(0, 2);
        setparameter(0, 3);
        setparameter(0, 4);

        ui->parameterOne->setVisible(0);
        ui->parameterTwo->setVisible(0);
        ui->parameterThree->setVisible(0);
        ui->parameterFour->setVisible(0);
        ui->parameterFive->setVisible(0);
        break;
    case 1: //rectangle : DrawSketchHandlerBox
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (along x axis)"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (along y axis)"));
        
        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 2: //Round corner rectangle : DrawSketchHandlerOblong
        toolParameters.resize(5, 0);
        isSettingSet.resize(5, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label5->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (along x axis)"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (along y axis)"));
        ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "Corner radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);
        ui->parameterFive->setVisible(1);
        ui->parameterFive->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 3: //Circle : DrawSketchHandlerCircle & arc
        toolParameters.resize(3, 0);
        isSettingSet.resize(3, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_circle", "Radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 4: //Point : DrawSketchHandlerPoint
        toolParameters.resize(2, 0);
        isSettingSet.resize(2, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_point", "x of point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_point", "y of point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 5: //Line : DrawSketchHandlerLine & arcby3points & circle by 3 points
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "x of 2nd point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "y of 2nd point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 6: //PolyLine (from second line)
        toolParameters.resize(4, 0);
        isSettingSet.resize(4, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_polyline", "x of n-1 point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_polyline", "y of n-1 point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_polyline", "x of n point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_polyline", "y of n point"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(0);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(0);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(1);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(1);

        QMetaObject::invokeMethod(ui->parameterThree, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    case 7: //Ellipse : DrawSketchHandlerEllipse
        toolParameters.resize(5, 0);
        isSettingSet.resize(5, 0);

        ui->label->setVisible(1);
        ui->label2->setVisible(1);
        ui->label3->setVisible(1);
        ui->label4->setVisible(1);
        ui->label5->setVisible(1);
        ui->label->setText(QApplication::translate("TaskSketcherTool_p1_Ellipse", "x of 1st point"));
        ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_Ellipse", "y of 1st point"));
        ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_Ellipse", "x of 2nd point"));
        ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_Ellipse", "y of 2nd point"));
        ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Ellipse", "Second radius"));

        ui->parameterOne->setVisible(1);
        ui->parameterOne->setEnabled(1);
        ui->parameterTwo->setVisible(1);
        ui->parameterTwo->setEnabled(1);
        ui->parameterThree->setVisible(1);
        ui->parameterThree->setEnabled(0);
        ui->parameterFour->setVisible(1);
        ui->parameterFour->setEnabled(0);
        ui->parameterFive->setVisible(1);
        ui->parameterFive->setEnabled(0);

        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
        isWidgetActive = 1;
        break;
    }
}

void SketcherToolWidget::setparameter(double val, int i)
{
    if (i == 0) {
        ui->parameterOne->setValue(Base::Quantity(val, Base::Unit::Length));
    }
    else if (i == 1) {
        ui->parameterTwo->setValue(Base::Quantity(val, Base::Unit::Length));
    }
    else if (i == 2) {
        ui->parameterThree->setValue(Base::Quantity(val, Base::Unit::Length));
    }
    else if (i == 3) {
        ui->parameterFour->setValue(Base::Quantity(val, Base::Unit::Length));
    }
    else if (i == 4) {
        ui->parameterFive->setValue(Base::Quantity(val, Base::Unit::Length));
    }
}

void SketcherToolWidget::setParameterActive(bool val, int i)
{
    if (i == 0) {
        ui->parameterOne->setEnabled(val);
    }
    else if (i == 1) {
        ui->parameterTwo->setEnabled(val);
    }
    else if (i == 2) {
        ui->parameterThree->setEnabled(val);
    }
    else if (i == 3) {
        ui->parameterFour->setEnabled(val);
    }
    else if (i == 4) {
        ui->parameterFive->setEnabled(val);
    }
}

void SketcherToolWidget::setParameterFocus(int i)
{
    if (i == 0) {
        ui->parameterOne->selectNumber();
        QMetaObject::invokeMethod(ui->parameterOne, "setFocus", Qt::QueuedConnection);
    }
    else if (i == 1) {
        ui->parameterTwo->selectNumber();
        QMetaObject::invokeMethod(ui->parameterTwo, "setFocus", Qt::QueuedConnection);
    }
    else if (i == 2) {
        ui->parameterThree->selectNumber();
        QMetaObject::invokeMethod(ui->parameterThree, "setFocus", Qt::QueuedConnection);
    }
    else if (i == 3) {
        ui->parameterFour->selectNumber();
        QMetaObject::invokeMethod(ui->parameterFour, "setFocus", Qt::QueuedConnection);
    }
    else if (i == 4) {
        ui->parameterFive->selectNumber();
        QMetaObject::invokeMethod(ui->parameterFive, "setFocus", Qt::QueuedConnection);
    }
}

void SketcherToolWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

// ----------------------------------------------------------------------------

TaskSketcherTool::TaskSketcherTool(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Tool settings"),true, 0)
    , sketchView(sketchView)
{
    widget = new SketcherToolWidget(this, sketchView);
    this->groupLayout()->addWidget(widget);

    widget->setSettings(0);

    Gui::Selection().Attach(this);

    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(boost::bind
        (&TaskSketcherTool::onChangedSketchView, this, bp::_1, bp::_2));
}

TaskSketcherTool::~TaskSketcherTool()
{
    Gui::Selection().Detach(this);
    //Content.erase(Content.begin());
}

void TaskSketcherTool::onChangedSketchView(const Gui::ViewProvider& vp,
                                              const App::Property& prop)
{
    if (sketchView == &vp) {
    }
}



/// @cond DOXERR
void TaskSketcherTool::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                   Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(Reason);
    //if (Reason.Type == SelectionChanges::AddSelection ||
    //    Reason.Type == SelectionChanges::RmvSelection ||
    //    Reason.Type == SelectionChanges::SetSelection ||
    //    Reason.Type == SelectionChanges::ClrSelection) {
    //}
}
/// @endcond DOXERR


#include "moc_TaskSketcherTool.cpp"
