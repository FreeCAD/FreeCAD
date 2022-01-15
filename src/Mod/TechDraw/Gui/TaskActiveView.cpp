/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#endif // #ifndef _PreComp_

#include <QApplication>
#include <QStatusBar>
#include <QGraphicsScene>
#include <QTemporaryFile>

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
#include <Mod/TechDraw/App/DrawViewSymbol.h>

#include <Mod/TechDraw/Gui/ui_TaskActiveView.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "Grabber3d.h"
#include "Rez.h"

#include "TaskActiveView.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for creation
TaskActiveView::TaskActiveView(TechDraw::DrawPage* pageFeat) :
    ui(new Ui_TaskActiveView),
    m_pageFeat(pageFeat),
    m_symbolFeat(nullptr),
    m_btnOK(nullptr),
    m_btnCancel(nullptr)
{
    ui->setupUi(this);

    ui->qsbWidth->setUnit(Base::Unit::Length);
    ui->qsbHeight->setUnit(Base::Unit::Length);
    ui->qsbBorder->setUnit(Base::Unit::Length);

    setUiPrimary();
}

TaskActiveView::~TaskActiveView()
{
}

void TaskActiveView::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskActiveView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskActiveView::setUiPrimary()
{
//    Base::Console().Message("TAV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("ActiveView to TD View"));
}

void TaskActiveView::blockButtons(bool b)
{
    Q_UNUSED(b);
}

//******************************************************************************
TechDraw::DrawViewSymbol* TaskActiveView::createActiveView(void)
{
//    Base::Console().Message("TAV::createActiveView()\n");

    std::string symbolName = m_pageFeat->getDocument()->getUniqueObjectName("ActiveView");
    std::string symbolType = "TechDraw::DrawViewSymbol";

    std::string pageName = m_pageFeat->getNameInDocument();

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       symbolType.c_str(),symbolName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), symbolName.c_str());

    App::Document* appDoc = m_pageFeat->getDocument();
    QTemporaryFile tempFile;
    if (!tempFile.open()) {                 //open() creates temp file
        Base::Console().Error("TAV::createActiveView - could not open temp file\n");
        return nullptr;
    }
    tempFile.close();

    std::string fileSpec = Base::Tools::toStdString(tempFile.fileName());
    fileSpec = Base::Tools::escapeEncodeFilename(fileSpec);

    //double estScale =
    Grabber3d::copyActiveViewToSvgFile(appDoc, fileSpec,
                                        ui->qsbWidth->rawValue(),
                                        ui->qsbHeight->rawValue(),
                                        ui->cbbg->isChecked(),
                                        ui->ccBgColor->color(),
                                        ui->qsbWeight->rawValue(),
                                        ui->qsbBorder->rawValue(),
                                        ui->cbMode->currentIndex());
    Command::doCommand(Command::Doc,"f = open(\"%s\",'r')",(const char*)fileSpec.c_str());
    Command::doCommand(Command::Doc,"svg = f.read()");
//    Command::doCommand(Command::Doc,"print('length of svg: {}'.format(len(svg)))");

    Command::doCommand(Command::Doc,"f.close()");
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Symbol = svg",symbolName.c_str());

    App::DocumentObject* newObj = m_pageFeat->getDocument()->getObject(symbolName.c_str());
    TechDraw::DrawViewSymbol* newSym = dynamic_cast<TechDraw::DrawViewSymbol*>(newObj);
    if ( (newObj == nullptr) ||
         (newSym == nullptr) ) {
        throw Base::RuntimeError("TaskActiveView - new symbol object not found");
    }

    return newSym;
}

//******************************************************************************

void TaskActiveView::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskActiveView::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

//******************************************************************************

bool TaskActiveView::accept()
{
//    Base::Console().Message("TAV::accept()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create ActiveView"));
    m_symbolFeat = createActiveView();
//    m_symbolFeat->requestPaint();
    m_symbolFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskActiveView::reject()
{
//    Base::Console().Message("TAV::reject()\n");
      //nothing to remove.

    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgActiveView::TaskDlgActiveView(TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskActiveView(page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ActiveView"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgActiveView::~TaskDlgActiveView()
{
}

void TaskDlgActiveView::update()
{
//    widget->updateTask();
}

void TaskDlgActiveView::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgActiveView::open()
{
}

void TaskDlgActiveView::clicked(int)
{
}

bool TaskDlgActiveView::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgActiveView::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskActiveView.cpp>
