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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewImage.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "TaskActiveView.h"
#include "ui_TaskActiveView.h"
#include "Grabber3d.h"
#include "ViewProviderImage.h"
#include "Rez.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

constexpr int SXGAWidth{1280};
constexpr int SXGAHeight{1024};

// ctor for creation
TaskActiveView::TaskActiveView(TechDraw::DrawPage* pageFeat)
    : ui(new Ui_TaskActiveView), m_pageFeat(pageFeat), m_imageFeat(nullptr),
      m_previewImageFeat(nullptr), m_btnOK(nullptr), m_btnCancel(nullptr)
{
    ui->setupUi(this);

    ui->qsbWidth->setUnit(Base::Unit::Length);
    ui->qsbHeight->setUnit(Base::Unit::Length);

    setUiPrimary();
    connect(ui->cbCrop, &QCheckBox::clicked, this, &TaskActiveView::onCropChanged);

    // For live preview
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create ActiveView"));

    m_previewImageFeat = createActiveView();
    if (!m_previewImageFeat) {
        Gui::Command::abortCommand();
        this->setEnabled(false);
        return;
    }

    connect(ui->qsbWidth, &Gui::QuantitySpinBox::editingFinished, this, &TaskActiveView::updatePreview);
    connect(ui->qsbHeight, &Gui::QuantitySpinBox::editingFinished, this, &TaskActiveView::updatePreview);
    connect(ui->cbUse3d, &QCheckBox::clicked, this, &TaskActiveView::updatePreview);
    connect(ui->cbNoBG, &QCheckBox::clicked, this, &TaskActiveView::updatePreview);
    connect(ui->ccBgColor, &QPushButton::clicked, this, &TaskActiveView::updatePreview);
    connect(ui->cbCrop, &QCheckBox::clicked, this, &TaskActiveView::updatePreview);

    updatePreview();
}

TaskActiveView::~TaskActiveView()
{
    if (m_previewImageFeat) {
        Gui::Command::abortCommand();
    }
}

bool TaskActiveView::accept()
{
    if (m_previewImageFeat) {
        Gui::Command::commitCommand();
        m_imageFeat = m_previewImageFeat;
        m_previewImageFeat = nullptr;
    }
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

bool TaskActiveView::reject()
{
    if (m_previewImageFeat) {
        Gui::Command::abortCommand();
        m_previewImageFeat = nullptr;
    }
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return true;
}

void TaskActiveView::updatePreview()
{
    if (!m_previewImageFeat) {
        return;
    }

    View3DInventor* view3d = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (!view3d) {
        Gui::Document* pageGuiDocument =
            Gui::Application::Instance->getDocument(m_pageFeat->getDocument()->getName());
        auto views3dAll = pageGuiDocument->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
        if (!views3dAll.empty()) {
            view3d = qobject_cast<View3DInventor*>(views3dAll.front());
        } else {
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
        Base::Console().warning("TaskActiveView::updatePreview - No 3D View found.\n");
        return;
    }

    App::Document* doc = m_previewImageFeat->getDocument();
    std::string pageName = m_pageFeat->getNameInDocument();
    std::string imageName = m_previewImageFeat->getNameInDocument();

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

    int imageWidth{SXGAWidth};
    int imageHeight{SXGAHeight};
    if (ui->cbCrop->isChecked()) {
        imageWidth = Rez::guiX(ui->qsbWidth->rawValue());
        imageHeight = Rez::guiX(ui->qsbHeight->rawValue());
    }

    QImage image(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    Grabber3d::quickView(view3d, bg, image);
    if (!image.save(QString::fromStdString(tempName), "PNG")) {
         Base::Console().error("ActiveView could not save file: %s\n", tempName.c_str());
    }

    tempName = DU::cleanFilespecBackslash(tempName);
    m_previewImageFeat->ImageFile.setValue(tempName);
    m_previewImageFeat->Width.setValue(ui->qsbWidth->rawValue());
    m_previewImageFeat->Height.setValue(ui->qsbHeight->rawValue());

    if (auto* guiDoc = Gui::Application::Instance->getDocument(doc)) {
        if (auto* vp = guiDoc->getViewProvider(m_previewImageFeat)) {
            if (auto* vpImage = freecad_cast<ViewProviderImage*>(vp)) {
                vpImage->Crop.setValue(ui->cbCrop->isChecked());
            }
        }
    }

    m_previewImageFeat->recomputeFeature();
}

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

void TaskActiveView::blockButtons(bool b) { Q_UNUSED(b); }

// Slots
void TaskActiveView::onCropChanged()
{
    enableCrop(ui->cbCrop->isChecked());
}

// Private helper methods
void TaskActiveView::setUiPrimary()
{
    setWindowTitle(QObject::tr("Insert Active View"));
    ui->cbCrop->setChecked(false);
    enableCrop(false);
    ui->qsbWidth->setValue(Rez::appX(SXGAWidth));
    ui->qsbHeight->setValue(Rez::appX(SXGAHeight));
}

void TaskActiveView::enableCrop(bool state)
{
    ui->qsbHeight->setEnabled(state);
    ui->qsbWidth->setEnabled(state);
}

TechDraw::DrawViewImage* TaskActiveView::createActiveView()
{
    View3DInventor* view3d = qobject_cast<View3DInventor*>(Gui::getMainWindow()->activeWindow());
    if (!view3d) {
        // Fallback 1: Try to find a 3D view in the page's document
        Gui::Document* pageGuiDocument =
            Gui::Application::Instance->getDocument(m_pageFeat->getDocument()->getName());
        if (pageGuiDocument) {
            auto views3dAll = pageGuiDocument->getMDIViewsOfType(Gui::View3DInventor::getClassTypeId());
            if (!views3dAll.empty()) {
                view3d = qobject_cast<View3DInventor*>(views3dAll.front());
            }
        }
    }
    if (!view3d) {
        // This check is simplified as the more complex fallback is in updatePreview
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No 3D Viewer"),
                             QObject::tr("Can not find a 3D viewer"));
        return nullptr;
    }

    App::Document* pageDocument = m_pageFeat->getDocument();
    const std::string objectName{"ActiveView"};
    const std::string imageType = "TechDraw::DrawViewImage";

    std::string sObjName = pageDocument->getUniqueObjectName(objectName.c_str());

    pageDocument->addObject(imageType.c_str(), sObjName.c_str());
    App::DocumentObject* newObj = pageDocument->getObject(sObjName.c_str());

    m_pageFeat->addView(newObj);
    newObj->Label.setValue("ActiveView");

    return static_cast<TechDraw::DrawViewImage*>(newObj);
}

void TaskActiveView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskActiveView::updateTask()
{
}

TaskDlgActiveView::TaskDlgActiveView(TechDraw::DrawPage* page) : TaskDialog()
{
    widget = new TaskActiveView(page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ActiveView"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgActiveView::~TaskDlgActiveView() {}

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

void TaskDlgActiveView::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

void TaskDlgActiveView::open() {}

void TaskDlgActiveView::clicked(int) {}

void TaskDlgActiveView::update() {}


#include <Mod/TechDraw/Gui/moc_TaskActiveView.cpp>
