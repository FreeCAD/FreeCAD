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
#include <Gui/MainWindow.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <QMdiSubWindow>

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
    connect(ui->checkBoxPreserveFilletChamferCorner, SIGNAL(toggled(bool)),
        this, SLOT(emitTogglePreserveFilletChamferCorner(bool)));
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
    connect(ui->parameterSix, SIGNAL(valueChanged(double)),
        this, SLOT(emitSetparameterSix(double)));
    this->installEventFilter(this);
    ui->parameterOne->installEventFilter(this);
    ui->parameterTwo->installEventFilter(this);
    ui->parameterThree->installEventFilter(this);
    ui->parameterFour->installEventFilter(this);
    ui->parameterFive->installEventFilter(this);
    ui->parameterSix->installEventFilter(this);
    ui->notice->setWordWrap(true);
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
    if (event->type() == QEvent::KeyPress) {
        /*If a key shortcut is required to work on sketcher when a tool using Tool Setting widget
        is being used, then you have to add this key to the below section such that the spinbox
        doesn't keep the keypress event for itself. Note if you want the event to be handled by 
        the spinbox too, you can return false.*/
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Control)
        {
            sketchView->keyPressed(1, SoKeyboardEvent::LEFT_CONTROL);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Shift)
        {
            sketchView->keyPressed(1, SoKeyboardEvent::RIGHT_SHIFT);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Alt)
        {
            sketchView->keyPressed(1, SoKeyboardEvent::RIGHT_ALT);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Escape)
        {
            sketchView->keyPressed(1, SoKeyboardEvent::ESCAPE);
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Space)
        {
            sketchView->keyPressed(1, SoKeyboardEvent::SPACE);
            return true;
        }
    }
    return false;
}

void SketcherToolWidget::emitTogglePreserveFilletChamferCorner(bool val) {
    ui->checkBoxPreserveFilletChamferCorner->onSave();
}
void SketcherToolWidget::emitSetparameter(double val)
{
    isSettingSet[0] = 1;
    toolParameters[0] = val;
    if (unitChangeTriggeredValueChanged) {
        unitChangeTriggeredValueChanged = 0;
    }
    else setParameterFocus(1);

    //Make a mousemove to update geometry when parameter is set.
    if(isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterTwo(double val)
{
    isSettingSet[1] = 1;
    toolParameters[1] = val;
    if (unitChangeTriggeredValueChanged) {
        unitChangeTriggeredValueChanged = 0;
    }
    else setParameterFocus(2);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterThree(double val)
{
    isSettingSet[2] = 1;
    toolParameters[2] = val;
    if (unitChangeTriggeredValueChanged) {
        unitChangeTriggeredValueChanged = 0;
    }
    else setParameterFocus(3);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterFour(double val)
{
    isSettingSet[3] = 1;
    toolParameters[3] = val;
    if (unitChangeTriggeredValueChanged) {
        unitChangeTriggeredValueChanged = 0;
    }
    else setParameterFocus(4);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterFive(double val)
{
    isSettingSet[4] = 1;
    toolParameters[4] = val;
    if (unitChangeTriggeredValueChanged) {
        unitChangeTriggeredValueChanged = 0;
    }
    else setParameterFocus(5);

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}
void SketcherToolWidget::emitSetparameterSix(double val)
{
    isSettingSet[5] = 1;
    toolParameters[5] = val;

    if (isWidgetActive) {
        sketchView->mouseMove(sketchView->prvMoveCursorPos, sketchView->prvMoveViewer);
    }
}

bool SketcherToolWidget::isCheckBoxChecked(int i) {
    if (i == 1) {
        return ui->checkBoxPreserveFilletChamferCorner->isChecked();
    }
    else if (i == 2) {
        return ui->checkBoxTS2->isChecked();
    }
    return 0;
}

void SketcherToolWidget::setSettings(int toolSelected)
{
    //reset and hide all settings (case 0)
    isWidgetActive = 0;
    toolParameters.clear();
    isSettingSet.clear();

    ui->label->setVisible(0);
    ui->label2->setVisible(0);
    ui->label3->setVisible(0);
    ui->label4->setVisible(0);
    ui->label5->setVisible(0);
    ui->label6->setVisible(0);
    ui->notice->setVisible(0);
    setparameter(0, 0);
    setparameter(0, 1);
    setparameter(0, 2);
    setparameter(0, 3);
    setparameter(0, 4);
    setparameter(0, 5);
    setUnit(Base::Unit::Length, 0);
    setUnit(Base::Unit::Length, 1);
    setUnit(Base::Unit::Length, 2);
    setUnit(Base::Unit::Length, 3);
    setUnit(Base::Unit::Length, 4);
    setUnit(Base::Unit::Length, 5);
    unitChangeTriggeredValueChanged = 0;

    ui->parameterOne->setVisible(0);
    ui->parameterTwo->setVisible(0);
    ui->parameterThree->setVisible(0);
    ui->parameterFour->setVisible(0);
    ui->parameterFive->setVisible(0);
    ui->parameterSix->setVisible(0);
    ui->checkBoxPreserveFilletChamferCorner->setVisible(0);
    ui->checkBoxTS2->setVisible(0);
    //sketchView->toolSettings->hideGroupBox();

    //Give the focus back to the viewproviderSketcher
    //Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    //mdi->setFocus();
    QMdiArea* mdi = qobject_cast<QMdiArea*>(Gui::MainWindow::getInstance()->centralWidget());
    if (!mdi) return;
    mdi->activeSubWindow()->widget()->setFocus();

    switch (toolSelected) {
        case 1://rectangle : DrawSketchHandlerBox
        { 
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

            setParameterFocus(0);
            isWidgetActive = 1;
            break; 
        }
        case 2: //Round corner rectangle : DrawSketchHandlerOblong
        {
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

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 3: //Circle : DrawSketchHandlerCircle & arc
        {
            toolParameters.resize(3, 0);
            isSettingSet.resize(3, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of Center point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of Center point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_circle", "Radius"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 4: //Point : DrawSketchHandlerPoint
        {
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

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 5: //Line : DrawSketchHandlerLine & arcby3points & circle by 3 points
        {
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

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 6: //PolyLine (from second line)
        {
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

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 7: //Ellipse : DrawSketchHandlerEllipse
        {
            toolParameters.resize(5, 0);
            isSettingSet.resize(5, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label4->setVisible(1);
            ui->label5->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_Ellipse", "x of 1st point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_Ellipse", "y of 1st point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_Ellipse", "First radius"));
            ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_Ellipse", "Angle to HAxis"));
            ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Ellipse", "Second radius"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);
            ui->parameterFour->setVisible(1);
            ui->parameterFour->setEnabled(0);
            setUnit(Base::Unit::Angle, 3);
            isSettingSet[3] = 0; //setUnit triggers ValuedChanged.
            ui->parameterFive->setVisible(1);
            ui->parameterFive->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 8: //Constraint mm
        {
            toolParameters.resize(1, 0);
            isSettingSet.resize(1, 0);

            ui->label->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_Constrain_Distance", "Distance"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 9: //Constraint 2 * mm
        {
            toolParameters.resize(2, 0);
            isSettingSet.resize(2, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_Constrain_DistanceX", "Distance X"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_Constrain_DistanceY", "Distance Y"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 10: //Constraint angle
        {
            toolParameters.resize(1, 0);
            isSettingSet.resize(1, 0);

            ui->label->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_Constrain_Distance", "Angle"));
            setUnit(Base::Unit::Angle, 0);

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);

            setParameterFocus(0);
            isWidgetActive = 1;
            isSettingSet[0] = 0; //setUnit triggers ValuedChanged.
            break;
        }
        case 11: //Fillet
        {
            toolParameters.resize(2, 0);
            isSettingSet.resize(2, 0);

            ui->label->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_Fillet", "Radius"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_Fillet", "Number Of Lines"));

            ui->parameterTwo->setValue(Base::Quantity(1, Base::Unit::Length)); //TODO : Unit should be none.
            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setEnabled(1);

            ui->checkBoxPreserveFilletChamferCorner->setVisible(1);
            ui->checkBoxPreserveFilletChamferCorner->setText(QApplication::translate("TaskSketcherTool_Fillet", "Preserve corner and most constraints"));
            ui->checkBoxTS2->setVisible(1);
            ui->checkBoxTS2->setChecked(0);
            ui->checkBoxTS2->setText(QApplication::translate("TaskSketcherTool_Fillet", "Inward"));

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 12: //Slot
        {
            toolParameters.resize(5, 0);
            isSettingSet.resize(5, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label4->setVisible(1);
            ui->label5->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_slot", "Length"));
            ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_slot", "Angle to H axis)"));
            ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_slot", "Radius"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);
            ui->parameterFour->setVisible(1);
            ui->parameterFour->setEnabled(0);
            setUnit(Base::Unit::Angle, 3);
            isSettingSet[3] = 0; //setUnit triggers ValuedChanged.
            ui->parameterFive->setVisible(1);
            ui->parameterFive->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 13: //Arc Slot
        {
            toolParameters.resize(6, 0);
            isSettingSet.resize(6, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label4->setVisible(1);
            ui->label5->setVisible(1);
            ui->label6->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Radius"));
            ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Start angle"));
            ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "Arc angle"));
            ui->label6->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "Slot radius"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);
            ui->parameterFour->setVisible(1);
            ui->parameterFour->setEnabled(0);
            setUnit(Base::Unit::Angle, 3);
            isSettingSet[3] = 0; //setUnit triggers ValuedChanged.
            ui->parameterFive->setVisible(1);
            ui->parameterFive->setEnabled(0);
            setUnit(Base::Unit::Angle, 4);
            isSettingSet[4] = 0; //setUnit triggers ValuedChanged.
            ui->parameterSix->setVisible(1);
            ui->parameterSix->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 14: //Rectangle Slot
        {
            toolParameters.resize(6, 0);
            isSettingSet.resize(6, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label4->setVisible(1);
            ui->label5->setVisible(1);
            ui->label6->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "1st radius"));
            ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Start angle"));
            ui->label5->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "Arc angle"));
            ui->label6->setText(QApplication::translate("TaskSketcherTool_p5_Oblong", "2nd radius"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(1);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(1);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);
            ui->parameterFour->setVisible(1);
            ui->parameterFour->setEnabled(0);
            setUnit(Base::Unit::Angle, 3);
            isSettingSet[3] = 0; //setUnit triggers ValuedChanged.
            ui->parameterFive->setVisible(1);
            ui->parameterFive->setEnabled(0);
            setUnit(Base::Unit::Angle, 4);
            isSettingSet[4] = 0; //setUnit triggers ValuedChanged.
            ui->parameterSix->setVisible(1);
            ui->parameterSix->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 15: //Frame : DrawSketchHandlerFrame
        {
            toolParameters.resize(5, 0);
            isSettingSet.resize(5, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label4->setVisible(1);
            ui->label5->setVisible(1);
            ui->notice->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (along x axis)"));
            ui->label4->setText(QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (along y axis)"));
            ui->label5->setText(QApplication::translate("TaskSketcherTool_Frame", "Thickness"));
            ui->notice->setText(QApplication::translate("TaskSketcherTool_Frame", "Positive thickness is inward, negative outward."));

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

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }
        case 16: //Insert : DrawSketchHandlerInsert
        {
            toolParameters.resize(3, 0);
            isSettingSet.resize(3, 0);

            ui->label->setVisible(1);
            ui->label2->setVisible(1);
            ui->label3->setVisible(1);
            ui->label->setText(QApplication::translate("TaskSketcherTool_p1_insert", "Start length"));
            ui->label2->setText(QApplication::translate("TaskSketcherTool_p2_insert", "Insert length"));
            ui->label3->setText(QApplication::translate("TaskSketcherTool_p3_insert", "Insert depth"));

            ui->parameterOne->setVisible(1);
            ui->parameterOne->setEnabled(0);
            ui->parameterTwo->setVisible(1);
            ui->parameterTwo->setEnabled(0);
            ui->parameterThree->setVisible(1);
            ui->parameterThree->setEnabled(0);

            setParameterFocus(0);
            isWidgetActive = 1;
            break;
        }

    }

}

void SketcherToolWidget::loadSettings()
{
    ui->checkBoxPreserveFilletChamferCorner->onRestore();
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
    else if (i == 5) {
        ui->parameterSix->setValue(Base::Quantity(val, Base::Unit::Length));
    }
}
void SketcherToolWidget::setUnit(Base::Unit unit, int i)
{
    unitChangeTriggeredValueChanged = 1;
    //For reference unit can be changed with : 
    //setUnit(Base::Unit::Length); Base::Unit::Angle
    if (i == 0) {
        ui->parameterOne->setUnit(unit);
    }
    else if (i == 1) {
        ui->parameterTwo->setUnit(unit);
    }
    else if (i == 2) {
        ui->parameterThree->setUnit(unit);
    }
    else if (i == 3) {
        ui->parameterFour->setUnit(unit);
    }
    else if (i == 4) {
        ui->parameterFive->setUnit(unit);
    }
    else if (i == 5) {
        ui->parameterSix->setUnit(unit);
    }
}
void SketcherToolWidget::setLabel(const QString& atext, int i)
{
    if (i == 0) {
        ui->label->setVisible(1);
        ui->label->setText(atext);
    }
    else if (i == 1) {
        ui->label2->setVisible(1);
        ui->label2->setText(atext);
    }
    else if (i == 2) {
        ui->label3->setVisible(1);
        ui->label3->setText(atext);
    }
    else if (i == 3) {
        ui->label4->setVisible(1);
        ui->label4->setText(atext);
    }
    else if (i == 4) {
        ui->label5->setVisible(1);
        ui->label5->setText(atext);
    }
    else if (i == 5) {
        ui->label6->setVisible(1);
        ui->label6->setText(atext);
    }
    else if (i == 6) {
        ui->notice->setVisible(1);
        ui->notice->setText(atext);
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
    else if (i == 5) {
        ui->parameterSix->setEnabled(val);
    }
    else if (i == 6) {
        ui->checkBoxPreserveFilletChamferCorner->setEnabled(val);
    }
    else if (i == 7) {
        ui->checkBoxTS2->setEnabled(val);
    }
}

void SketcherToolWidget::setParameterVisible(bool val, int i)
{
    if (i == 0) {
        ui->label->setVisible(val);
        ui->parameterOne->setVisible(val);
    }
    else if (i == 1) {
        ui->label2->setVisible(val);
        ui->parameterTwo->setVisible(val);
    }
    else if (i == 2) {
        ui->label3->setVisible(val);
        ui->parameterThree->setVisible(val);
    }
    else if (i == 3) {
        ui->label4->setVisible(val);
        ui->parameterFour->setVisible(val);
    }
    else if (i == 4) {
        ui->label5->setVisible(val);
        ui->parameterFive->setVisible(val);
    }
    else if (i == 5) {
        ui->label6->setVisible(val);
        ui->parameterSix->setVisible(val);
    }
    else if (i == 6) {
        ui->checkBoxPreserveFilletChamferCorner->setVisible(val);
    }
    else if (i == 7) {
        ui->checkBoxTS2->setVisible(val);
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
    else if (i == 5) {
        ui->parameterSix->selectNumber();
        QMetaObject::invokeMethod(ui->parameterSix, "setFocus", Qt::QueuedConnection);
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

    widget->loadSettings();

    Gui::Selection().Attach(this);

    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(boost::bind
        (&TaskSketcherTool::onChangedSketchView, this, bp::_1, bp::_2));
    widget->setSettings(0);
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
