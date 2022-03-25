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
#include <QButtonGroup>
#include <QStatusBar>
#include <QGraphicsScene>
#endif // #ifndef _PreComp_

#include <BRepBuilderAPI_MakeEdge.hxx>

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

#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "ui_TaskSelectLineAttributes.h"
#include "TaskSelectLineAttributes.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGVPage.h"
#include "QGIPrimPath.h"
#include "QGIView.h"
#include "Rez.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

enum class EdgeStyle {
    solid = 1,
    dashed = 2,
    dotted = 3,
    dashdotted = 4
};

enum class EdgeWidth {
    small = 1,
    middle = 2,
    thick = 3
};

enum class EdgeColor {
    black = 1,
    grey = 2,
    red = 3,
    green = 4,
    blue = 5,
    magenta = 6,
    cyan = 7,
    yellow = 8
};

//===========================================================================
// managing global line attributes
//===========================================================================

lineAttributes::lineAttributes(void)
{
    style = int(EdgeStyle::dotted);
    width = int(EdgeWidth::middle);
    color = int(EdgeColor::black);
}

void lineAttributes::setStyle(int newStyle)
{
    style = newStyle;
}

void lineAttributes::setWidth(int newWidth)
{
    width = newWidth;
}

float lineAttributes::getWidthValue(void)
{
    switch(EdgeWidth(width)) {
        case EdgeWidth::small:
            return 0.18f;
            break;
        case EdgeWidth::middle:
            return 0.35f;
            break;
        case EdgeWidth::thick:
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
    switch (EdgeColor(color)) {
    case EdgeColor::black:
        return App::Color(0.0f, 0.0f, 0.0f);
        break;
    case EdgeColor::grey:
        return App::Color(0.7f, 0.7f, 0.7f);
        break;
    case EdgeColor::red:
        return App::Color(1.0f, 0.0f, 0.0f);
        break;
    case EdgeColor::green:
        return App::Color(0.0f, 1.0f, 0.0f);
        break;
    case EdgeColor::blue:
        return App::Color(0.0f, 0.0f, 1.0f);
        break;
    case EdgeColor::magenta:
        return App::Color(1.0f, 0.0f, 1.0f);
        break;
    case EdgeColor::cyan:
        return App::Color(0.0f, 1.0f, 1.0f);
        break;
    case EdgeColor::yellow:
        return App::Color(1.0f, 1.0f, 0.0f);
        break;
    default:
        return App::Color(0.0f, 0.0f, 0.0f);
    }
}

//===========================================================================
// managing global dimension attributes
//===========================================================================

dimAttributes::dimAttributes(void)
{
    cascadeSpacing = 7.0;
    lineStretch = 2.0;
}

void dimAttributes::setCascadeSpacing(double spacing)
{
    cascadeSpacing = spacing;
}

void dimAttributes::setLineStretch(double stretch)
{
    lineStretch = stretch;
}

dimAttributes activeDimAttributes; // container holding dimension attributes

//===========================================================================
// TaskSelectLineAttributes
//===========================================================================

TaskSelectLineAttributes::TaskSelectLineAttributes(lineAttributes * ptActiveAttributes) :
    activeAttributes(ptActiveAttributes),
    ui(new Ui_TaskSelectLineAttributes)
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
    setWindowTitle(tr("Select line attributes"));
    int lineStyle = activeAttributes->getStyle();
    switch(EdgeStyle(lineStyle)) {
        case EdgeStyle::solid:
            ui->rbSolid->setChecked(true);
            break;
        case EdgeStyle::dashed:
            ui->rbDashed->setChecked(true);
            break;
        case EdgeStyle::dotted:
            ui->rbDotted->setChecked(true);
            break;
        case EdgeStyle::dashdotted:
            ui->rbDashDotted->setChecked(true);
            break;
        default:
            ui->rbDashDotted->setChecked(true);
    }

    int lineWidth = activeAttributes->getWidth();
    switch(EdgeWidth(lineWidth)) {
        case EdgeWidth::small:
            ui->rbThin->setChecked(true);
            break;
        case EdgeWidth::middle:
            ui->rbMiddle->setChecked(true);
            break;
        case EdgeWidth::thick:
            ui->rbThick->setChecked(true);
            break;
        default:
            ui->rbMiddle->setChecked(true);
    }

    int lineColor = activeAttributes->getColor();
    switch(EdgeColor(lineColor)) {
        case EdgeColor::black:
            ui->rbBlack->setChecked(true);
            break;
        case EdgeColor::grey:
            ui->rbGrey->setChecked(true);
            break;
        case EdgeColor::red:
            ui->rbRed->setChecked(true);
            break;
        case EdgeColor::green:
            ui->rbGreen->setChecked(true);
            break;
        case EdgeColor::blue:
            ui->rbBlue->setChecked(true);
            break;
        case EdgeColor::magenta:
            ui->rbMagenta->setChecked(true);
            break;
        case EdgeColor::cyan:
            ui->rbCyan->setChecked(true);
            break;
        case EdgeColor::yellow:
            ui->rbYellow->setChecked(true);
            break;
        default:
            ui->rbBlack->setChecked(true);
    }

    double cascadeSpacing = activeDimAttributes.getCascadeSpacing();
    ui->sbSpacing->setValue(cascadeSpacing);
    double lineStretching = activeDimAttributes.getLineStretch();
    ui->sbStretch->setValue(lineStretching);

}

bool TaskSelectLineAttributes::accept()
{
    if (ui->rbSolid->isChecked()){
        activeAttributes->setStyle(int(EdgeStyle::solid));
    }
    else if (ui->rbDashed->isChecked()){
        activeAttributes->setStyle(int(EdgeStyle::dashed));
    }
    else if (ui->rbDotted->isChecked()){
        activeAttributes->setStyle(int(EdgeStyle::dotted));
    }
    else if (ui->rbDashDotted->isChecked()){
        activeAttributes->setStyle(int(EdgeStyle::dashdotted));
    }
    else {
        activeAttributes->setStyle(int(EdgeStyle::dashdotted));
    }

    if (ui->rbThin->isChecked()){
        activeAttributes->setWidth(int(EdgeWidth::small));
    }
    else if (ui->rbMiddle->isChecked()){
        activeAttributes->setWidth(int(EdgeWidth::middle));
    }
    else if (ui->rbThick->isChecked()){
        activeAttributes->setWidth(int(EdgeWidth::thick));
    }
    else {
        activeAttributes->setWidth(int(EdgeWidth::middle));
    }

    if (ui->rbBlack->isChecked()){
        activeAttributes->setColor(int(EdgeColor::black));
    }
    else if (ui->rbGrey->isChecked()){
        activeAttributes->setColor(int(EdgeColor::grey));
    }
    else if (ui->rbRed->isChecked()){
        activeAttributes->setColor(int(EdgeColor::red));
    }
    else if (ui->rbGreen->isChecked()){
        activeAttributes->setColor(int(EdgeColor::green));
    }
    else if (ui->rbBlue->isChecked()){
        activeAttributes->setColor(int(EdgeColor::blue));
    }
    else if (ui->rbMagenta->isChecked()){
        activeAttributes->setColor(int(EdgeColor::magenta));
    }
    else if (ui->rbCyan->isChecked()){
        activeAttributes->setColor(int(EdgeColor::cyan));
    }
    else if (ui->rbYellow->isChecked()){
        activeAttributes->setColor(int(EdgeColor::yellow));
    }
    else {
        activeAttributes->setColor(int(EdgeColor::black));
    }

    double cascadeSpacing = ui->sbSpacing->value();
    activeDimAttributes.setCascadeSpacing(cascadeSpacing);
    double lineStretching = ui->sbStretch->value();
    activeDimAttributes.setLineStretch(lineStretching);

    return true;
}

bool TaskSelectLineAttributes::reject()
{
    return true;
}

//===========================================================================
// TaskDlgSelectLineAttributes
//===========================================================================

TaskDlgSelectLineAttributes::TaskDlgSelectLineAttributes(lineAttributes * ptActiveAttributes)
    : TaskDialog()
{
    widget  = new TaskSelectLineAttributes(ptActiveAttributes);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_ExtensionSelectLineAttributes"),
                                             widget->windowTitle(), true, nullptr);
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
