/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#include <QPushButton>

#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Quantity.h>
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
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/Gui/ui_TaskComplexSection.h>

#include "DrawGuiUtil.h"

#include "TaskComplexSection.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskComplexSection::TaskComplexSection(TechDraw::DrawPage* page,
                                       TechDraw::DrawViewPart* baseView,
                                       std::vector<App::DocumentObject*> shapes,
                                       std::vector<App::DocumentObject*> xShapes,
                                       App::DocumentObject* profileObject,
                                       std::vector<std::string> profileSubs) :
    ui(new Ui_TaskComplexSection),
    m_page(page),
    m_baseView(baseView),
    m_section(nullptr),
    m_shapes(shapes),
    m_xShapes(xShapes),
    m_profileObject(profileObject),
    m_profileSubs(profileSubs),
    m_sectionName(std::string())
{
    ui->setupUi(this);
    setUiPrimary();

    connect(ui->pbSectionObjects, SIGNAL(clicked()), this, SLOT(onSectionObjectsUseSelectionClicked()));
    connect(ui->pbProfileObject, SIGNAL(clicked()), this, SLOT(onProfileObjectsUseSelectionClicked()));
}

void TaskComplexSection::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskComplexSection::setUiPrimary()
{
    setWindowTitle(QObject::tr("New Complex Section"));

    ui->dsbXNormal->setEnabled(true);
    ui->dsbYNormal->setEnabled(true);
    ui->dsbZNormal->setEnabled(true);
    std::pair<Base::Vector3d, Base::Vector3d> dirs = DrawGuiUtil::get3DDirAndRot();
    ui->dsbXNormal->setValue(dirs.first.x);
    ui->dsbYNormal->setValue(dirs.first.y);
    ui->dsbZNormal->setValue(dirs.first.z);
    m_saveXDir = dirs.second;

    ui->leSectionObjects->setText(sourcesToString());
    ui->leProfileObject->setText(Base::Tools::fromStdString(m_profileObject->getNameInDocument()) +
                                 QString::fromUtf8(" / ") +
                                 Base::Tools::fromStdString(m_profileObject->Label.getValue()));
    if (m_baseView) {
        ui->dsbScale->setValue(m_baseView->getScale());
        ui->cmbScaleType->setCurrentIndex(m_baseView->ScaleType.getValue());
    } else {
        ui->dsbScale->setValue(Preferences::scale());
        ui->cmbScaleType->setCurrentIndex(Preferences::scaleType());
    }
}

void TaskComplexSection::onSectionObjectsUseSelectionClicked()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    std::vector<App::DocumentObject*> newSelection;
    std::vector<App::DocumentObject*> newXSelection;
    for (auto& sel : selection) {
        if (sel.getObject()->isDerivedFrom(App::LinkElement::getClassTypeId()) ||
            sel.getObject()->isDerivedFrom(App::LinkGroup::getClassTypeId())   ||
            sel.getObject()->isDerivedFrom(App::Link::getClassTypeId()) ) {
            newXSelection.push_back(sel.getObject());
        } else {
            newSelection.push_back(sel.getObject());
        }
    }
    m_shapes = newSelection;
    m_xShapes = newXSelection;
    ui->leSectionObjects->setText(sourcesToString());
}

void TaskComplexSection::onProfileObjectsUseSelectionClicked()
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    //check for single selection and ability to make profile from selected object
    if (!selection.empty()) {
        m_profileObject = selection.front().getObject();
        ui->leProfileObject->setText(Base::Tools::fromStdString(m_profileObject->getNameInDocument()) +
                                     QString::fromUtf8(" / ") +
                                     Base::Tools::fromStdString(m_profileObject->Label.getValue()));
    }
}

QString TaskComplexSection::sourcesToString()
{
    QString result;
    if (m_baseView) {
        for (auto& obj : m_baseView->Source.getValues()) {
            result += Base::Tools::fromStdString(obj->getNameInDocument()) +
                      QString::fromUtf8(" / ") +
                      Base::Tools::fromStdString(obj->Label.getValue()) +
                      QString::fromUtf8(", ");
        }
        for (auto& obj : m_baseView->XSource.getValues()) {
            result += Base::Tools::fromStdString(obj->getNameInDocument()) +
                      QString::fromUtf8(" / ") +
                      Base::Tools::fromStdString(obj->Label.getValue()) +
                      QString::fromUtf8(", ");
        }
    } else {
        for (auto& obj : m_shapes) {
            result += Base::Tools::fromStdString(obj->getNameInDocument()) +
                      QString::fromUtf8(" / ") +
                      Base::Tools::fromStdString(obj->Label.getValue()) +
                      QString::fromUtf8(", ");
        }
        for (auto& obj : m_xShapes) {
            result += Base::Tools::fromStdString(obj->getNameInDocument()) +
                      QString::fromUtf8(" / ") +
                      Base::Tools::fromStdString(obj->Label.getValue()) +
                      QString::fromUtf8(", ");
        }
    }
    return result;
}

void TaskComplexSection::updateUi()
{
}

//pointer to created view is not returned, but stored in m_section
void TaskComplexSection::createComplexSection()
{
//    Base::Console().Message("TCS::createComplexSection()\n");

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create ComplexSection"));
    if (!m_section) {
        m_sectionName = m_page->getDocument()->getUniqueObjectName("ComplexSection");
        std::string sectionType = "TechDraw::DrawComplexSection";

        Command::doCommand(Command::Doc, "App.ActiveDocument.addObject('%s', '%s')",
                           sectionType.c_str(), m_sectionName.c_str());
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.addView(App.ActiveDocument.%s)",
                           m_page->getNameInDocument(), m_sectionName.c_str());

        QString qTemp    = ui->leSymbol->text();
        std::string temp = Base::Tools::toStdString(qTemp);
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.SectionSymbol = '%s'",
                           m_sectionName.c_str(),
                           temp.c_str());
        std::string lblText = "Section " +
                              temp +
                              " - " +
                              temp;
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Label = '%s'",
                           m_sectionName.c_str(),
                           lblText.c_str());

        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.Scale = %0.6f",
                           m_sectionName.c_str(),
                           ui->dsbScale->value());
        int scaleType = ui->cmbScaleType->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ScaleType = %d",
                           m_sectionName.c_str(), scaleType);
        int projectionStrategy = ui->cmbStrategy->currentIndex();
        Command::doCommand(Command::Doc, "App.ActiveDocument.%s.ProjectionStrategy = %d",
                           m_sectionName.c_str(), projectionStrategy);

        Command::doCommand(Command::Doc, "App.activeDocument().%s.Direction = FreeCAD.Vector(%.3f, %.3f, %.3f)",
                                         m_sectionName.c_str(), ui->dsbXNormal->value(),
                                         ui->dsbYNormal->value(), ui->dsbZNormal->value());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.SectionNormal = FreeCAD.Vector(%.3f, %.3f, %.3f)",
                                          m_sectionName.c_str(), ui->dsbXNormal->value(),
                                          ui->dsbYNormal->value(), ui->dsbZNormal->value());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.XDirection = FreeCAD.Vector(%.3f, %.3f, %.3f)",
                                          m_sectionName.c_str(), m_saveXDir.x, m_saveXDir.y, m_saveXDir.z);
        Command::doCommand(Command::Doc, "App.activeDocument().%s.SectionOrigin = FreeCAD.Vector(0.0, 0.0, 0.0)",
                                          m_sectionName.c_str());
        Command::doCommand(Command::Doc, "App.activeDocument().%s.SectionDirection = 'Aligned'",
                                          m_sectionName.c_str());

        App::DocumentObject* newObj = m_page->getDocument()->getObject(m_sectionName.c_str());
        m_section = dynamic_cast<TechDraw::DrawComplexSection*>(newObj);
        if (!newObj || !m_section)  {
            throw Base::RuntimeError("TaskComplexSection - new section object not found");
        }
        if (m_baseView) {
            m_section->Source.setValues(m_baseView->Source.getValues());
            m_section->XSource.setValues(m_baseView->XSource.getValues());
            Command::doCommand(Command::Doc, "App.ActiveDocument.%s.BaseView = App.ActiveDocument.%s",
                               m_sectionName.c_str(), m_baseView->getNameInDocument());

        } else {
            m_section->Source.setValues(m_shapes);
            m_section->XSource.setValues(m_xShapes);
        }
        m_section->CuttingToolWireObject.setValue(m_profileObject);
    }
    Gui::Command::commitCommand();
    if (m_section) {
        m_section->recomputeFeature();
    }
    return;
}

void TaskComplexSection::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskComplexSection::enableTaskButtons(bool button)
{
    m_btnOK->setEnabled(button);
    m_btnCancel->setEnabled(button);
}

//******************************************************************************
bool TaskComplexSection::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_page->getDocument());
    if (!doc)
        return false;

    createComplexSection();

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskComplexSection::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_page->getDocument());
    if (!doc)
        return false;

    //make sure any dangling objects are cleaned up
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgComplexSection::TaskDlgComplexSection(TechDraw::DrawPage* page,
                                             TechDraw::DrawViewPart *baseView,
                                             std::vector<App::DocumentObject*> shapes,
                                             std::vector<App::DocumentObject*> xShapes,
                                             App::DocumentObject* profileObject,
                                             std::vector<std::string> profileSubs)
    : TaskDialog()
{
    widget  = new TaskComplexSection(page, baseView, shapes, xShapes, profileObject, profileSubs);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_ComplexSection"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgComplexSection::~TaskDlgComplexSection()
{
}

void TaskDlgComplexSection::update()
{
//    widget->updateTask();
}

void TaskDlgComplexSection::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgComplexSection::open()
{
}

void TaskDlgComplexSection::clicked(int)
{
}

bool TaskDlgComplexSection::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgComplexSection::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskComplexSection.cpp>
