/***************************************************************************
 *   Copyright (c) 2020 FreeCAD Developers                                 *
 *   Author: Uwe Stöhr <uwestoehr@lyx.org>                                 *
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
#endif // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "PreferencesGui.h"
#include "ViewProviderHatch.h"
#include "TaskHatch.h"
#include <Mod/TechDraw/Gui/ui_TaskHatch.h>

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for creation
TaskHatch::TaskHatch(TechDraw::DrawViewPart* inDvp, std::vector<std::string> subs) :
    ui(new Ui_TaskHatch),
    m_hatch(nullptr),
    m_dvp(inDvp),
    m_subs(subs)
{
    ui->setupUi(this);

    connect(ui->fcFile, SIGNAL(fileNameSelected(QString)), this, SLOT(onFileChanged()));
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    connect(ui->ccColor, SIGNAL(changed()), this, SLOT(onColorChanged()));

    setUiPrimary();
}

//ctor for edit
TaskHatch::TaskHatch(TechDrawGui::ViewProviderHatch* inVp) :
    ui(new Ui_TaskHatch),
    m_vp(inVp)
{
//    Base::Console().Message("TH::TH() - edit\n");
    ui->setupUi(this);
    m_hatch = m_vp->getViewObject();
    App::DocumentObject* obj = m_hatch->Source.getValue();
    m_dvp = static_cast<TechDraw::DrawViewPart*>(obj);

    connect(ui->fcFile, SIGNAL(fileNameSelected(QString)), this, SLOT(onFileChanged()));
    connect(ui->sbScale, SIGNAL(valueChanged(double)), this, SLOT(onScaleChanged()));
    connect(ui->ccColor, SIGNAL(changed()), this, SLOT(onColorChanged()));

    saveHatchState();
    setUiEdit();
}
TaskHatch::~TaskHatch()
{
}

void TaskHatch::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Face Hatch"));
    ui->fcFile->setFileName(Base::Tools::fromStdString(DrawHatch::prefSvgHatch()));
    ui->fcFile->setFilter(QString::fromUtf8(
            "SVG files (*.svg *.SVG);;Bitmap files(*.jpg *.jpeg *.png *.bmp);;All files (*)"));
    ui->sbScale->setValue(1.0);
    ui->sbScale->setSingleStep(0.1);
    ui->ccColor->setColor(TechDraw::DrawHatch::prefSvgHatchColor().asValue<QColor>());
}

void TaskHatch::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Face Hatch"));
    ui->fcFile->setFileName(Base::Tools::fromStdString(m_saveFile));
    ui->fcFile->setFilter(QString::fromUtf8(
            "SVG files (*.svg *.SVG);;Bitmap files(*.jpg *.jpeg *.png *.bmp);;All files (*)"));
    ui->sbScale->setValue(m_saveScale);
    ui->sbScale->setSingleStep(0.1);
    ui->ccColor->setColor(m_saveColor.asValue<QColor>());
}

void TaskHatch::saveHatchState()
{
    m_saveFile = m_hatch->HatchPattern.getValue();
    m_saveScale = m_vp->HatchScale.getValue();
    m_saveColor = m_vp->HatchColor.getValue();
}

//restore the start conditions
void TaskHatch::restoreHatchState()
{
//    Base::Console().Message("TH::restoreHatchState()\n");
    if (m_hatch) {
        m_hatch->HatchPattern.setValue(m_saveFile);
        m_vp->HatchScale.setValue(m_saveScale);
        m_vp->HatchColor.setValue(m_saveColor);
    }
}

void TaskHatch::onFileChanged()
{
    m_file = Base::Tools::toStdString(ui->fcFile->fileName());
    apply();
}

void TaskHatch::onScaleChanged()
{
    m_scale = ui->sbScale->value().getValue();
    apply();
}

void TaskHatch::onColorChanged()
{
    m_color.setValue<QColor>(ui->ccColor->color());
    apply();
}

void TaskHatch::apply(bool forceUpdate)
{
    Q_UNUSED(forceUpdate)
//    Base::Console().Message("TH::apply() - m_hatch: %X\n", m_hatch);
    if (!m_hatch) {
        createHatch();
    }
    if (m_hatch) {
        updateHatch();
    }

    if (m_dvp) {
        //only need requestPaint to hatch the face
//        m_dvp->requestPaint();
        //need a recompute in order to claimChildren in tree
        m_dvp->recomputeFeature();
    }
}

void TaskHatch::createHatch()
{
    Base::Console().Message("TH::createHatch()\n");
    App::Document* doc = m_dvp->getDocument();
    std::string FeatName = doc->getUniqueObjectName("Hatch");
    std::stringstream featLabel;
    featLabel << FeatName << "F" <<
                    TechDraw::DrawUtil::getIndexFromName(m_subs.at(0)); //use 1st face# for label

    Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Hatch"));

    Command::doCommand(Command::Doc, "App.activeDocument().addObject('TechDraw::DrawHatch', '%s')", FeatName.c_str());
    Command::doCommand(Command::Doc, "App.activeDocument().%s.Label = '%s'", FeatName.c_str(), featLabel.str().c_str());

    m_hatch = static_cast<TechDraw::DrawHatch *>(doc->getObject(FeatName.c_str()));
    m_hatch->Source.setValue(m_dvp, m_subs);

    Command::doCommand(Command::Doc, "App.activeDocument().%s.HatchPattern = '%s'",
                       FeatName.c_str(),
                       Base::Tools::toStdString(ui->fcFile->fileName()).c_str());

    //view provider properties
    Gui::ViewProvider* vp = Gui::Application::Instance->getDocument(doc)->getViewProvider(m_hatch);
    m_vp = dynamic_cast<TechDrawGui::ViewProviderHatch*>(vp);
    if (m_vp) {
        App::Color ac;
        ac.setValue<QColor>(ui->ccColor->color());
        m_vp->HatchColor.setValue(ac);
        m_vp->HatchScale.setValue(ui->sbScale->value().getValue());
    } else {
        Base::Console().Error("TaskHatch - Hatch has no ViewProvider\n");
    }
    Command::commitCommand();
}

void TaskHatch::updateHatch()
{
    Base::Console().Message("TH::updateHatch()\n");
    std::string FeatName = m_hatch->getNameInDocument();

    Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update Hatch"));

    Command::doCommand(Command::Doc, "App.activeDocument().%s.HatchPattern = '%s'",
                       FeatName.c_str(),
                       Base::Tools::toStdString(ui->fcFile->fileName()).c_str());

    App::Color ac;
    ac.setValue<QColor>(ui->ccColor->color());
    m_vp->HatchColor.setValue(ac);
    m_vp->HatchScale.setValue(ui->sbScale->value().getValue());
    Command::commitCommand();
}

bool TaskHatch::accept()
{
//    Base::Console().Message("TH::accept()\n");
    apply(true);

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskHatch::reject()
{
//    Base::Console().Message("TH::reject()\n");
    restoreHatchState();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return false;
}

void TaskHatch::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgHatch::TaskDlgHatch(TechDraw::DrawViewPart* inDvp, std::vector<std::string> subs) :
    TaskDialog()
{
    widget  = new TaskHatch(inDvp, subs);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_TreeHatch"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgHatch::TaskDlgHatch(TechDrawGui::ViewProviderHatch* inVp) :
    TaskDialog()
{
    widget  = new TaskHatch(inVp);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_TreeHatch"),
                                         widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgHatch::~TaskDlgHatch()
{
}

void TaskDlgHatch::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgHatch::open()
{
}

void TaskDlgHatch::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgHatch::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgHatch::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskHatch.cpp>
