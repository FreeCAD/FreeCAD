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
# include <regex>

# include <QMessageBox>
# include <QPushButton>
#endif // #ifndef _PreComp_
#include <Gui/View3DInventor.h>
#include <Gui/ViewProvider.h>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewImage.h>

#include "TaskActiveView.h"
#include "ui_TaskActiveView.h"
#include "Grabber3d.h"
#include "ViewProviderImage.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for creation
TaskActiveView::TaskActiveView(TechDraw::DrawPage* pageFeat)
    : ui(new Ui_TaskActiveView), m_pageFeat(pageFeat), m_imageFeat(nullptr), m_btnOK(nullptr),
      m_btnCancel(nullptr)
{
    ui->setupUi(this);

    ui->qsbWidth->setUnit(Base::Unit::Length);
    ui->qsbHeight->setUnit(Base::Unit::Length);

    setUiPrimary();
}

TaskActiveView::~TaskActiveView() {}

void TaskActiveView::updateTask()
{
    //    blockUpdate = true;

    //    blockUpdate = false;
}

void TaskActiveView::changeEvent(QEvent* e)
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

void TaskActiveView::blockButtons(bool b) { Q_UNUSED(b); }

TechDraw::DrawViewImage* TaskActiveView::createActiveView()
{
    //    Base::Console().Message("TAV::createActiveView()\n");

    //make sure there is an 3D MDI to grab!!
    if (!Gui::getMainWindow()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No Main Window"),
                             QObject::tr("Can not find the main window"));
        return nullptr;
    }

    App::Document* pageDocument = m_pageFeat->getDocument();
    std::string documentName = m_pageFeat->getDocument()->getName();
    Gui::Document* pageGuiDocument =
        Gui::Application::Instance->getDocument(pageDocument->getName());

    //if the active view is a 3d window, use that.
    View3DInventor* view3d = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (!view3d) {
        // active view is not a 3D view, try to find one in the current document
        auto views3dAll = pageGuiDocument->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
        if (!views3dAll.empty()) {
            view3d = qobject_cast<View3DInventor*>(views3dAll.front());
        }
        else {
            //this code is only for the rare case where the page's document does not have a
            //3D window.  It might occur if the user closes the 3D window, but leaves, for
            //example, a DrawPage window open.
            //the active window is not a 3D view, and the page's document does not have a
            //3D view, so try to find one somewhere among the open windows.
            auto mdiWindows = Gui::getMainWindow()->windows();
            for (auto& mdi : mdiWindows) {
                auto mdiView = qobject_cast<View3DInventor*>(mdi);
                if (mdiView) {
                    view3d = mdiView;
                    break;
                }
            }
        }
    }
    if (!view3d) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No 3D Viewer"),
                             QObject::tr("Can not find a 3D viewer"));
        return nullptr;
    }

    //we are sure we have a 3D window!

    const std::string objectName{"ActiveView"};
    std::string imageName = m_pageFeat->getDocument()->getUniqueObjectName(objectName.c_str());
    std::string generatedSuffix {imageName.substr(objectName.length())};
    std::string imageType = "TechDraw::DrawViewImage";

    std::string pageName = m_pageFeat->getNameInDocument();

    //the Page's document may not be the active one, so we need to get the right
    //document by name instead of using ActiveDocument
    Command::doCommand(Command::Doc, "App.getDocument('%s').addObject('%s','%s')",
                       documentName.c_str(), imageType.c_str(), imageName.c_str());

    Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawActiveView', 'ActiveView', '%s')",
              imageName.c_str(), imageName.c_str());

    Command::doCommand(Command::Doc, "App.getDocument('%s').%s.addView(App.getDocument('%s').%s)",
                       documentName.c_str(), pageName.c_str(), documentName.c_str(),
                       imageName.c_str());

    App::Document* doc = m_pageFeat->getDocument();
    std::string special = "/" + imageName + "image.png";
    std::string dir = doc->TransientDir.getValue();
    std::string fileSpec = dir + special;

    //fixes fail to create 2nd Active view with same name in old docs
    Base::FileInfo fi(fileSpec);
    if (fi.exists()) {
        //old filename were unique by pageName + imageName only
        fi.deleteFile();
    }

    //better way of making temp file name
    std::string baseName = pageName + imageName;
    std::string tempName =
        Base::FileInfo::getTempFileName(baseName.c_str(), doc->TransientDir.getValue()) + ".png";

    QColor bg = ui->ccBgColor->color();
    if (ui->cbUse3d->isChecked()) {
        bg = QColor();
    }
    else if (ui->cbNoBG->isChecked()) {
        bg = QColor(Qt::transparent);
    }

    QImage image(100, 100,
                 QImage::Format_RGB32);    //arbitrary initial image size. quickView will use
                                           //MdiView size in pixels
    image.fill(QColor(Qt::transparent));
    Grabber3d::quickView(view3d, bg, image);
    bool success = image.save(Base::Tools::fromStdString(tempName));

    if (!success) {
        Base::Console().Error("ActiveView could not save file: %s\n", fileSpec.c_str());
    }

    //backslashes in windows fileSpec upsets python
    std::regex rxBackslash("\\\\");    //this rx really means match to a single '\'
    std::string noBackslash = std::regex_replace(tempName, rxBackslash, "/");

    Command::doCommand(Command::Doc, "App.getDocument('%s').%s.ImageFile = '%s'",
                       documentName.c_str(), imageName.c_str(), noBackslash.c_str());
    Command::doCommand(Command::Doc, "App.getDocument('%s').%s.Width = %.5f", documentName.c_str(),
                       imageName.c_str(), ui->qsbWidth->rawValue());
    Command::doCommand(Command::Doc, "App.getDocument('%s').%s.Height = %.5f", documentName.c_str(),
                       imageName.c_str(), ui->qsbHeight->rawValue());

    App::DocumentObject* newObj = m_pageFeat->getDocument()->getObject(imageName.c_str());
    TechDraw::DrawViewImage* newImg = dynamic_cast<TechDraw::DrawViewImage*>(newObj);
    if (!newObj || !newImg)
        throw Base::RuntimeError("TaskActiveView - new image object not found");
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(newImg->getDocument());
    if (guiDoc) {
        Gui::ViewProvider* vp = guiDoc->getViewProvider(newImg);
        if (vp) {
            auto vpImage = dynamic_cast<ViewProviderImage*>(vp);
            if (vpImage) {
                vpImage->Crop.setValue(ui->cbCrop->isChecked());
            }
        }
    }

    return newImg;
}

//******************************************************************************

void TaskActiveView::saveButtons(QPushButton* btnOK, QPushButton* btnCancel)
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
    if (m_imageFeat) {
        m_imageFeat->recomputeFeature();
    }
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
TaskDlgActiveView::TaskDlgActiveView(TechDraw::DrawPage* page) : TaskDialog()
{
    widget = new TaskActiveView(page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ActiveView"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgActiveView::~TaskDlgActiveView() {}

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
void TaskDlgActiveView::open() {}

void TaskDlgActiveView::clicked(int) {}

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
