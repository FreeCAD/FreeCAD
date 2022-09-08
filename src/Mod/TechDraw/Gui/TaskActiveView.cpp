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
#include <QApplication>
#include <QPushButton>
#include <QStatusBar>
#include <QGraphicsScene>
#include <QTemporaryFile>
#endif // #ifndef _PreComp_


#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <App/Document.h>

#include <Base/FileInfo.h>

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
#include <Mod/TechDraw/App/DrawViewImage.h>

#include <Mod/TechDraw/Gui/ui_TaskActiveView.h>

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
    m_imageFeat(nullptr),
    m_btnOK(nullptr),
    m_btnCancel(nullptr)
{
    ui->setupUi(this);

    ui->qsbWidth->setUnit(Base::Unit::Length);
    ui->qsbHeight->setUnit(Base::Unit::Length);

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

TechDraw::DrawViewImage* TaskActiveView::createActiveView()
{
//    Base::Console().Message("TAV::createActiveView()\n");

    std::string imageName = m_pageFeat->getDocument()->getUniqueObjectName("ActiveView");
    std::string imageType = "TechDraw::DrawViewImage";

    std::string pageName = m_pageFeat->getNameInDocument();

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       imageType.c_str(),imageName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), imageName.c_str());

    App::Document* appDoc = m_pageFeat->getDocument();

    App::Document* doc = m_pageFeat->getDocument();
    std::string special = "/" + imageName + "image.png";
    std::string dir = doc->TransientDir.getValue();
    std::string fileSpec = dir + special;

    QColor bg = ui->ccBgColor->color();
    if (ui->cbUse3d->isChecked()) {
        bg = QColor();
    } else if (ui->cbNoBG->isChecked()) {
        bg = QColor(Qt::transparent);
    }


//    QImage image;
    QImage image(100, 100, QImage::Format_RGB32);
    image.fill(QColor(Qt::transparent));
    Grabber3d:: quickView(appDoc,
                          Rez::guiX(ui->qsbWidth->rawValue()),      //mm to scene units
                          Rez::guiX(ui->qsbHeight->rawValue()),     //mm to scene units
                          bg,
                          image);
    bool success = image.save(Base::Tools::fromStdString(fileSpec));

    if (!success) {
        Base::Console().Error("ActiveView could not save file: %s\n", fileSpec.c_str());
    }
    Command::doCommand(Command::Doc,"App.activeDocument().%s.ImageFile = '%s'",imageName.c_str(), fileSpec.c_str());

    App::DocumentObject* newObj = m_pageFeat->getDocument()->getObject(imageName.c_str());
    TechDraw::DrawViewImage* newImg = dynamic_cast<TechDraw::DrawViewImage*>(newObj);
    if ( (newObj == nullptr) ||
         (newImg == nullptr) ) {
        throw Base::RuntimeError("TaskActiveView - new image object not found");
    }

    return newImg;
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
    m_imageFeat = createActiveView();
//    m_imageFeat->requestPaint();
    m_imageFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskActiveView::reject()
{
//    Base::Console().Message("TAV::reject()\n");
      //nothing to remove.

    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgActiveView::TaskDlgActiveView(TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskActiveView(page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ActiveView"),
                                             widget->windowTitle(), true, nullptr);
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
