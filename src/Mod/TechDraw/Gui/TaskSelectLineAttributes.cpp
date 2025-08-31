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

#include <Gui/BitmapFactory.h>

#include <Mod/TechDraw/App/LineGenerator.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "ui_TaskSelectLineAttributes.h"
#include "TaskSelectLineAttributes.h"
#include "DrawGuiUtil.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

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

// line attributes are now in App/LineFormat

//===========================================================================
// managing global dimension attributes
//===========================================================================

dimAttributes::dimAttributes()
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

TaskSelectLineAttributes::TaskSelectLineAttributes() :
    ui(new Ui_TaskSelectLineAttributes)
{
    m_lineGenerator = new TechDraw::LineGenerator;

    ui->setupUi(this);

    setUiEdit();
}

TaskSelectLineAttributes::~TaskSelectLineAttributes()
{
    delete m_lineGenerator;
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
    setWindowTitle(tr("Select Line Attributes"));
    int lineStyle = LineFormat::getCurrentLineFormat().getStyle();
    // line numbering starts at 1, not 0
    DrawGuiUtil::loadLineStyleChoices(ui->cbLineStyle, m_lineGenerator);
    if (ui->cbLineStyle->count() >= lineStyle ) {
        ui->cbLineStyle->setCurrentIndex(lineStyle - 1);
    }

    // TODO: how to handle translation of a string with arg parameters in it?
    ui->rbThin->setText(QStringLiteral("Thin %1").arg(QString::number(TechDraw::LineGroup::getDefaultWidth("Thin"))));
    ui->rbMiddle->setText(QStringLiteral("Middle %1").arg(QString::number(TechDraw::LineGroup::getDefaultWidth("Graphic"))));
    ui->rbThick->setText(QStringLiteral("Thick %1").arg(QString::number(TechDraw::LineGroup::getDefaultWidth("Thick"))));

    double lineWidth = LineFormat::getCurrentLineFormat().getWidth();
    if (lineWidth <= TechDraw::LineGroup::getDefaultWidth("Thin")) {
        ui->rbThin->setChecked(true);
    } else if (lineWidth <= TechDraw::LineGroup::getDefaultWidth("Graphic")) {
        ui->rbMiddle->setChecked(true);
    } else if (lineWidth <= TechDraw::LineGroup::getDefaultWidth("Thick")) {
        ui->rbThick->setChecked(true);
    } else {
        ui->rbMiddle->setChecked(true);
    }

    QColor lineColor = LineFormat::getCurrentLineFormat().getQColor();
    ui->cbColor->setColor(lineColor);

    double cascadeSpacing = activeDimAttributes.getCascadeSpacing();
    ui->sbSpacing->setValue(cascadeSpacing);
    double lineStretching = activeDimAttributes.getLineStretch();
    ui->sbStretch->setValue(lineStretching);

}

bool TaskSelectLineAttributes::accept()
{
    LineFormat::getCurrentLineFormat().setStyle(ui->cbLineStyle->currentIndex() + 1);
    LineFormat::getCurrentLineFormat().setLineNumber(ui->cbLineStyle->currentIndex() + 1);

    if (ui->rbThin->isChecked()){
        LineFormat::getCurrentLineFormat().setWidth(TechDraw::LineGroup::getDefaultWidth("Thin"));
    }
    else if (ui->rbMiddle->isChecked()){
        LineFormat::getCurrentLineFormat().setWidth(TechDraw::LineGroup::getDefaultWidth("Graphic"));
    }
    else if (ui->rbThick->isChecked()){
        LineFormat::getCurrentLineFormat().setWidth(TechDraw::LineGroup::getDefaultWidth("Thick"));
    }
    else {
        LineFormat::getCurrentLineFormat().setWidth(TechDraw::LineGroup::getDefaultWidth("Graphic"));
    }

    QColor qTemp = ui->cbColor->color();
    Base::Color temp;
    temp.set(qTemp.redF(), qTemp.greenF(), qTemp.blueF(), 1.0 - qTemp.alphaF());
    LineFormat::getCurrentLineFormat().setColor(temp);

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

TaskDlgSelectLineAttributes::TaskDlgSelectLineAttributes()
    : TaskDialog()
{
    widget  = new TaskSelectLineAttributes();
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
