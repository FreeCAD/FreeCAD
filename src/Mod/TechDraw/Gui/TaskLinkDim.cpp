/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <cmath>
# include <QTreeWidget>
#endif // #ifndef _PreComp_

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>

#include "TaskLinkDim.h"
#include "ui_TaskLinkDim.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;


TaskLinkDim::TaskLinkDim(std::vector<App::DocumentObject*> parts, std::vector<std::string>& subs, TechDraw::DrawPage* page) :
    ui(new Ui_TaskLinkDim),
    m_parts(parts),
    m_subs(subs),
    m_page(page)
{
    ui->setupUi(this);
    ui->selector->setAvailableLabel(tr("Available"));
    ui->selector->setSelectedLabel(tr("Selected"));

    connect(ui->selector->availableTreeWidget(), &QTreeWidget::currentItemChanged,
            this, &TaskLinkDim::onCurrentItemChanged);
    connect(ui->selector->selectedTreeWidget(), &QTreeWidget::currentItemChanged,
            this, &TaskLinkDim::onCurrentItemChanged);

    loadAvailDims();

    ui->leFeature1->setText(QString::fromStdString(parts.at(0)->getNameInDocument()));
    ui->leGeometry1->setText(QString::fromStdString(subs.at(0)));

    if (subs.size() > 1) {
        ui->leGeometry2->setText(QString::fromStdString(subs.at(1)));
        if (parts.at(0)->getNameInDocument() != parts.at(1)->getNameInDocument()) {
            ui->leFeature2->setText(QString::fromStdString(parts.at(1)->getNameInDocument()));
        } else {
            ui->leFeature2->clear();
        }
    }
}

TaskLinkDim::~TaskLinkDim()
{
}

void TaskLinkDim::loadAvailDims()
{
    App::Document* doc = m_page->getDocument();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(doc);
    if (!guiDoc)
        return;

    std::vector<App::DocumentObject*> pageViews = m_page->Views.getValues();
    std::vector<App::DocumentObject*>::iterator itView = pageViews.begin();
    std::string result;
    int selRefType = TechDraw::DrawViewDimension::getRefTypeSubElements(m_subs);
    //int found = 0;
    for (; itView != pageViews.end(); itView++) {
        if ((*itView)->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
            TechDraw::DrawViewDimension* dim = static_cast<TechDraw::DrawViewDimension*>((*itView));
            int dimRefType = dim->getRefType();
            if (dimRefType == selRefType) {                                     //potential matches
    //            found++;
                if (dim->has3DReferences()) {
                    if (dimReferencesSelection(dim))  {
                        loadToTree(dim, true, guiDoc);
                    } else {
                        continue;                                               //already linked to something else
                    }
                } else {
                    loadToTree(dim, false, guiDoc);
                }
            }
        }
    }
    //if (found == 0) { "No matching Dimensions found in %s", m_page->getNameInDocument())
}

void TaskLinkDim::loadToTree(const TechDraw::DrawViewDimension* dim, const bool selected, Gui::Document* guiDoc)
{
    QString label = QString::fromUtf8(dim->Label.getValue());
    QString name = QString::fromUtf8(dim->getNameInDocument());
    QString tooltip = label + QString::fromUtf8(" / ") + name;

    QTreeWidgetItem* child = new QTreeWidgetItem();
    child->setText(0, label);
    child->setToolTip(0, tooltip);
    child->setData(0, Qt::UserRole, name);
    Gui::ViewProvider* vp = guiDoc->getViewProvider(dim);
    if (vp) child->setIcon(0, vp->getIcon());
    if (selected) {
        ui->selector->selectedTreeWidget()->addTopLevelItem(child);
    } else {
        ui->selector->availableTreeWidget()->addTopLevelItem(child);
    }
}

//! does this dim already have a reference to the selection?
bool TaskLinkDim::dimReferencesSelection(const TechDraw::DrawViewDimension* dim) const
{
    if (!dim->has3DReferences()) {
        return false;
    }

    std::vector<App::DocumentObject*> refParts = dim->References3D.getValues();
    std::vector<std::string> refSubs = dim->References3D.getSubValues();
    if (refParts.size() != m_parts.size()) {
        return false;
    }

    if(refParts.empty()) {
        //shouldn't happen!
    } else if (refParts.size() == 1) {
        if ((refParts[0] == m_parts[0]) &&
                (refSubs[0] == m_subs[0]) ) {         //everything matches
            return true;
        }
    } else if (refParts.size() == 2) {
        if (( (refParts[0] == m_parts[0]) &&
                (refParts[1] == m_parts[1]) )  &&
            ( (refSubs[0] == m_subs[0])   &&
                (refSubs[1] == m_subs[1]) ) ) {
            return true;
        } else if (( (refParts[0] == m_parts[1]) &&
                        (refParts[1] == m_parts[0]) )  &&
                    ( (refSubs[0] == m_subs[1])   &&
                        (refSubs[1] == m_subs[0]) ) ) {
            return true;
        }
    }

    return false;
}

void TaskLinkDim::updateDims()
{
    int iDim;
    int count = ui->selector->selectedTreeWidget()->topLevelItemCount();
    for (iDim=0; iDim<count; iDim++) {
        QTreeWidgetItem* child = ui->selector->selectedTreeWidget()->topLevelItem(iDim);
        QString name = child->data(0, Qt::UserRole).toString();
        App::DocumentObject* obj = m_page->getDocument()->getObject(name.toStdString().c_str());
        TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(obj);
        if (!dim)
            continue;
//        std::vector<App::DocumentObject*> parts;
//        for (unsigned int iPart = 0; iPart < m_subs.size(); iPart++) {
//            parts.push_back(m_part);
//        }
        dim->References3D.setValues(m_parts, m_subs);
        std::string DimName = dim->getNameInDocument();
        std::string measureType = "True";
        Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.MeasureType = \'%s\'",
                            DimName.c_str(), measureType.c_str());
        //dim->MeasureType.setValue("True");
    }
    count = ui->selector->availableTreeWidget()->topLevelItemCount();
    for (iDim=0; iDim < count; iDim++) {
        QTreeWidgetItem* child = ui->selector->availableTreeWidget()->topLevelItem(iDim);
        QString name = child->data(0, Qt::UserRole).toString();
        App::DocumentObject* obj = m_page->getDocument()->getObject(name.toStdString().c_str());
        TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(obj);
        if (dim && dimReferencesSelection(dim))  {
           std::string measureType = "Projected";
           std::string DimName = dim->getNameInDocument();
           Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().%s.MeasureType = \'%s\'",
                            DimName.c_str(), measureType.c_str());
           dim->References3D.setValue(nullptr, "");            //DVD.References3D
           dim->clear3DMeasurements();                  //DVD.measurement.References3D
        }
    }
}

void TaskLinkDim::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
//    if (previous) {
//        Base::Console().Message("TRACE - TLD::onCurrent - text: %s data: %s is previous\n",
//                                qPrintable(previous->text(0)), qPrintable(previous->data(0, Qt::UserRole).toString()));
//        if (previous->treeWidget() == ui->selector->selectedTreeWidget()) {
//            Base::Console().Message("TRACE - TLD::onCurrent - previous belongs to selected\n");
//        }
//        if (previous->treeWidget() == ui->selector->availableTreeWidget()) {
//            Base::Console().Message("TRACE - TLD::onCurrent - previous belongs to available\n");
//        }
//    }
//    if (current) {
//        Base::Console().Message("TRACE - TLD::onCurrent - text: %s data: %s is current\n",
//                                 qPrintable(current->text(0)), qPrintable(current->data(0, Qt::UserRole).toString()));
//        if (current->treeWidget() == ui->selector->selectedTreeWidget()) {
//            Base::Console().Message("TRACE - TLD::onCurrent - current belongs to selected\n");
//        }
//        if (current->treeWidget() == ui->selector->availableTreeWidget()) {
//            Base::Console().Message("TRACE - TLD::onCurrent - current belongs to available\n");
//        }
//    }
}

bool TaskLinkDim::accept()
{
    updateDims();
    return true;
}

bool TaskLinkDim::reject()
{
    return true;
}

void TaskLinkDim::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLinkDim::TaskDlgLinkDim(std::vector<App::DocumentObject*> parts, std::vector<std::string>& subs, TechDraw::DrawPage* page) :
    TaskDialog()
{
    widget  = new TaskLinkDim(parts, subs, page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("TechDraw_LinkDimension"),
                                         widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgLinkDim::~TaskDlgLinkDim()
{
}

void TaskDlgLinkDim::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgLinkDim::open()
{
}

void TaskDlgLinkDim::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgLinkDim::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgLinkDim::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskLinkDim.cpp>
