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

#define solid 1
#define dashed 2
#define dotted 3
#define dashdotted 4

//enum EdgeType{solid, dashed, dotted, dashdotted};
//enum EdgeWidth{small, middle, thick };
#define small 1
#define middle 2
#define thick 3
//enum EdgeColor{black, grey, red, green, blue, magenta, cyan, yellow};

#define black 1
#define grey 2
#define red 3
#define green 4
#define blue 5
#define magenta 6
#define cyan 7
#define yellow 8

//===========================================================================
// managing global line attributes
//===========================================================================

lineAttributes::lineAttributes(void)
{
    style = dotted;
    width = middle;
    color = black;
}

void lineAttributes::setStyle(int newStyle)
{
    style = newStyle;
}

void lineAttributes::setWidth(float newWidth)
{
    width = newWidth;
}

float lineAttributes::getWidthValue(void)
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

void lineAttributes::setColor(int newColor)
{
    color = newColor;
}

App::Color lineAttributes::getColorValue(void)
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

TaskSelectLineAttributes::TaskSelectLineAttributes(lineAttributes * ptActiveAttributes) :
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
    int lineStyle = activeAttributes->getStyle();
    switch(lineStyle){
        case solid:
            ui->rbSolid->setChecked(true);
            break;
        case dashed:
            ui->rbDashed->setChecked(true);
            break;
        case dotted:
            ui->rbDotted->setChecked(true);
            break;
        case dashdotted:
            ui->rbDashDotted->setChecked(true);
            break;
        default:
            ui->rbDashDotted->setChecked(true);
    }

    int lineWidth = activeAttributes->getWidth();
    switch(lineWidth){
        case small:
            ui->rbThin->setChecked(true);
            break;
        case middle:
            ui->rbMiddle->setChecked(true);
            break;
        case thick:
            ui->rbThick->setChecked(true);
            break;
        default:
            ui->rbMiddle->setChecked(true);
    }

    int lineColor = activeAttributes->getColor();
    switch(lineColor){
        case black:
            ui->rbBlack->setChecked(true);
            break;
        case grey:
            ui->rbGrey->setChecked(true);
            break;
        case red:
            ui->rbRed->setChecked(true);
            break;
        case green:
            ui->rbGreen->setChecked(true);
            break;
        case blue:
            ui->rbBlue->setChecked(true);
            break;
        case magenta:
            ui->rbMagenta->setChecked(true);
            break;
        case cyan:
            ui->rbCyan->setChecked(true);
            break;
        case yellow:
            ui->rbGreen->setChecked(true);
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
        activeAttributes->setStyle(solid); 
    }
    else if (ui->rbDashed->isChecked()){
        activeAttributes->setStyle(dashed);
    }
    else if (ui->rbDotted->isChecked()){
        activeAttributes->setStyle(dotted);
    }
    else if (ui->rbDashDotted->isChecked()){
        activeAttributes->setStyle(dashdotted);
    }
    else {
        activeAttributes->setStyle(dashdotted);
    }

    if (ui->rbThin->isChecked()){
        activeAttributes->setWidth(small); 
    }
    else if (ui->rbMiddle->isChecked()){
        activeAttributes->setWidth(middle);
    }
    else if (ui->rbThick->isChecked()){
        activeAttributes->setWidth(thick);
    }
    else {
        activeAttributes->setWidth(middle);
    }

    if (ui->rbBlack->isChecked()){
        activeAttributes->setColor(black); 
    }
    else if (ui->rbGrey->isChecked()){
        activeAttributes->setColor(grey);
    }
    else if (ui->rbRed->isChecked()){
        activeAttributes->setColor(red);
    }
    else if (ui->rbGreen->isChecked()){
        activeAttributes->setColor(green);
    }
    else if (ui->rbBlue->isChecked()){
        activeAttributes->setColor(blue);
    }
    else if (ui->rbMagenta->isChecked()){
        activeAttributes->setColor(magenta);
    }
    else if (ui->rbCyan->isChecked()){
        activeAttributes->setColor(cyan);
    }
    else if (ui->rbYellow->isChecked()){
        activeAttributes->setColor(yellow);
    }
    else {
        activeAttributes->setColor(black);
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

TaskDlgSelectLineAttributes::TaskDlgSelectLineAttributes(lineAttributes * ptActiveAttributes)
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