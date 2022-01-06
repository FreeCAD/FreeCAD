/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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
#include <cmath>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

#endif // #ifndef _PreComp_

#include <BRepBuilderAPI_MakeEdge.hxx>

#include <QButtonGroup>
#include <QStatusBar>
#include <QGraphicsScene>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskSelectLineAttributes.h> 

#include "DrawGuiStd.h"
#include "PreferencesGui.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "Rez.h"

#include "TaskSelectLineAttributes.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//===========================================================================
// managing global line attributes
//===========================================================================

LineAttributes::LineAttributes(void)
{
    style = dotted;
    width = middle;
    color = black;
}

void LineAttributes::setStyle(edgeStyle newStyle)
{
    style = newStyle;
}

void LineAttributes::setWidth(edgeWidth newWidth)
{
    width = newWidth;
}

float LineAttributes::getWidthValue(void)
{
    switch(width){
        case small:
            return 0.18f;
            break;
        case middle:
            return 0.35f;
            break;
        case thick:
            return 0.5f;
            break;
        default:
            return 0.35f;
    }
}

void LineAttributes::setColor(edgeColor newColor)
{
    color = newColor;
}

App::Color LineAttributes::getColorValue(void)
{
    switch(color){
        case black:
            return App::Color(0.0,0.0,0.0);
            break;
        case grey:
            return App::Color(0.7,0.7,0.7);
            break;
        case red:
            return App::Color(1.0,0.0,0.0);
            break;
        case green:
            return App::Color(0.0,1.0,0.0);
            break;
        case blue:
            return App::Color(0.0,0.0,1.0);
            break;
        case magenta:
            return App::Color(1.0,0.0,1.0);
            break;
        case cyan:
            return App::Color(0.0,1.0,1.0);
            break;
        case yellow:
            return App::Color(1.0,1.0,0.0);
            break;
        default:
            return App::Color(0.0,0.0,0.0);
    }
}

//===========================================================================
// managing global dimension attributes
//===========================================================================

dimAttributes::dimAttributes(void)
{
    cascadeSpacing = 7.0;
}

void dimAttributes::setCascadeSpacing(double spacing)
{
    cascadeSpacing = spacing;
}

dimAttributes activeDimAttributes; // container holding dimension attributes

//===========================================================================
// TaskSelectLineAttributes
//===========================================================================

TaskSelectLineAttributes::TaskSelectLineAttributes(LineAttributes * ptActiveAttributes) :
    ui(new Ui_TaskSelectLineAttributes),
    activeAttributes(ptActiveAttributes)

{

    ui->setupUi(this);

    setUiEdit();
}

TaskSelectLineAttributes::~TaskSelectLineAttributes()
{

}

void TaskSelectLineAttributes::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}


void TaskSelectLineAttributes::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskSelectLineAttributes::setUiEdit()
{
    setWindowTitle(QObject::tr("Select Line Attributes"));
    LineAttributes::edgeStyle lineStyle = activeAttributes->getStyle();
    switch(lineStyle){
        case LineAttributes::solid:
            ui->rbSolid->setChecked(true);
            break;
        case LineAttributes::dashed:
            ui->rbDashed->setChecked(true);
            break;
        case LineAttributes::dotted:
            ui->rbDotted->setChecked(true);
            break;
        case LineAttributes::dashdotted:
            ui->rbDashDotted->setChecked(true);
            break;
        default:
            ui->rbDashDotted->setChecked(true);
    }

    LineAttributes::edgeWidth lineWidth = activeAttributes->getWidth();
    switch(lineWidth){
        case LineAttributes::small:
            ui->rbThin->setChecked(true);
            break;
        case LineAttributes::middle:
            ui->rbMiddle->setChecked(true);
            break;
        case LineAttributes::thick:
            ui->rbThick->setChecked(true);
            break;
        default:
            ui->rbMiddle->setChecked(true);
    }

    LineAttributes::edgeColor lineColor = activeAttributes->getColor();
    switch(lineColor){
        case LineAttributes::black:
            ui->rbBlack->setChecked(true);
            break;
        case LineAttributes::grey:
            ui->rbGrey->setChecked(true);
            break;
        case LineAttributes::red:
            ui->rbRed->setChecked(true);
            break;
        case LineAttributes::green:
            ui->rbGreen->setChecked(true);
            break;
        case LineAttributes::blue:
            ui->rbBlue->setChecked(true);
            break;
        case LineAttributes::magenta:
            ui->rbMagenta->setChecked(true);
            break;
        case LineAttributes::cyan:
            ui->rbCyan->setChecked(true);
            break;
        case LineAttributes::yellow:
            ui->rbYellow->setChecked(true);
            break;
        default:
            ui->rbBlack->setChecked(true);
    }

    double cascadeSpacing = activeDimAttributes.getCascadeSpacing();
    ui->sbSpacing->setValue(cascadeSpacing);

}

bool TaskSelectLineAttributes::accept()
{
    if (ui->rbSolid->isChecked()){
        activeAttributes->setStyle(LineAttributes::solid); 
    }
    else if (ui->rbDashed->isChecked()){
        activeAttributes->setStyle(LineAttributes::dashed);
    }
    else if (ui->rbDotted->isChecked()){
        activeAttributes->setStyle(LineAttributes::dotted);
    }
    else if (ui->rbDashDotted->isChecked()){
        activeAttributes->setStyle(LineAttributes::dashdotted);
    }
    else {
        activeAttributes->setStyle(LineAttributes::dashdotted);
    }

    if (ui->rbThin->isChecked()){
        activeAttributes->setWidth(LineAttributes::small); 
    }
    else if (ui->rbMiddle->isChecked()){
        activeAttributes->setWidth(LineAttributes::middle);
    }
    else if (ui->rbThick->isChecked()){
        activeAttributes->setWidth(LineAttributes::thick);
    }
    else {
        activeAttributes->setWidth(LineAttributes::middle);
    }

    if (ui->rbBlack->isChecked()){
        activeAttributes->setColor(LineAttributes::black); 
    }
    else if (ui->rbGrey->isChecked()){
        activeAttributes->setColor(LineAttributes::grey);
    }
    else if (ui->rbRed->isChecked()){
        activeAttributes->setColor(LineAttributes::red);
    }
    else if (ui->rbGreen->isChecked()){
        activeAttributes->setColor(LineAttributes::green);
    }
    else if (ui->rbBlue->isChecked()){
        activeAttributes->setColor(LineAttributes::blue);
    }
    else if (ui->rbMagenta->isChecked()){
        activeAttributes->setColor(LineAttributes::magenta);
    }
    else if (ui->rbCyan->isChecked()){
        activeAttributes->setColor(LineAttributes::cyan);
    }
    else if (ui->rbYellow->isChecked()){
        activeAttributes->setColor(LineAttributes::yellow);
    }
    else {
        activeAttributes->setColor(LineAttributes::black);
    }

    double cascadeSpacing = ui->sbSpacing->value();
    activeDimAttributes.setCascadeSpacing(cascadeSpacing);

    return true;
}

bool TaskSelectLineAttributes::reject()
{
    //there's nothing to do.
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    return false;
}

//===========================================================================
// TaskDlgSelectLineAttributes
//===========================================================================

TaskDlgSelectLineAttributes::TaskDlgSelectLineAttributes(LineAttributes * ptActiveAttributes)
    : TaskDialog()
{
    widget  = new TaskSelectLineAttributes(ptActiveAttributes);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_ExtensionSelectLineAttributes"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgSelectLineAttributes::~TaskDlgSelectLineAttributes()
{
}

void TaskDlgSelectLineAttributes::update()
{
//    widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgSelectLineAttributes::open()
{
}

void TaskDlgSelectLineAttributes::clicked(int)
{
}

bool TaskDlgSelectLineAttributes::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgSelectLineAttributes::reject()
{
    widget->reject();
    return true;
}


#include <Mod/TechDraw/Gui/moc_TaskSelectLineAttributes.cpp>