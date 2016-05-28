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
#include <cmath>
#endif // #ifndef _PreComp_

#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>

#include "TaskLinkDim.h"
#include "ui_TaskLinkDim.h"

using namespace Gui;
using namespace TechDrawGui;


TaskLinkDim::TaskLinkDim(Part::Feature* part, std::vector<std::string>& subs, TechDraw::DrawPage* page) :
    ui(new Ui_TaskLinkDim),
    m_part(part),
    m_subs(subs),
    m_page(page)
{
    ui->setupUi(this);
    ui->selector->setAvailableLabel(tr("Available"));
    ui->selector->setSelectedLabel(tr("Selected"));

    connect(ui->selector->availableTreeWidget(), SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(ui->selector->selectedTreeWidget(), SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

    loadAvailDims();

    ui->leFeature->setText(QString::fromStdString(part->getNameInDocument()));
    ui->leGeometry1->setText(QString::fromStdString(subs.at(0)));
    if (subs.size() > 1) {
        ui->leGeometry2->setText(QString::fromStdString(subs.at(1)));
    }
}

TaskLinkDim::~TaskLinkDim()
{
    delete ui;
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
    for (; itView != pageViews.end(); itView++) {
        if ((*itView)->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
            TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>((*itView));
            if (dim->References2D.getValues().size() == m_subs.size()) {
                QString label = QString::fromUtf8((*itView)->Label.getValue());
                QString name = QString::fromUtf8((*itView)->getNameInDocument());
                QString tooltip = label + QString::fromUtf8(" / ") + name;

                QTreeWidgetItem* child = new QTreeWidgetItem();
                child->setText(0, label);
                child->setToolTip(0, tooltip);
                child->setData(0, Qt::UserRole, name);
                Gui::ViewProvider* vp = guiDoc->getViewProvider(*itView);
                if (vp) child->setIcon(0, vp->getIcon());
                ui->selector->availableTreeWidget()->addTopLevelItem(child);
            }
        }
    }
}

void TaskLinkDim::updateDims()
{
    int count = ui->selector->selectedTreeWidget()->topLevelItemCount();
    if (count == 0) {
        return;
    }
    for (int iDim=0; iDim<count; iDim++) {
        QTreeWidgetItem* child = ui->selector->selectedTreeWidget()->topLevelItem(iDim);
        QString name = child->data(0, Qt::UserRole).toString();
        App::DocumentObject* obj = m_page->getDocument()->getObject(name.toStdString().c_str());
        TechDraw::DrawViewDimension* dim = dynamic_cast<TechDraw::DrawViewDimension*>(obj);
        std::vector<App::DocumentObject*> parts;
        for (unsigned int iPart = 0; iPart < m_subs.size(); iPart++) {
            parts.push_back(m_part);
        }
        dim->References3D.setValues(parts,m_subs);
        //dim->setMeasurement(m_part,m_subs);
        dim->MeasureType.setValue("True");
    }
}

void TaskLinkDim::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    //if (previous) {
        //picked item on "selected" side
    //}
    //if (current) {
        //picked item on "available" side
    //}
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

void TaskLinkDim::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLinkDim::TaskDlgLinkDim(Part::Feature* part,std::vector<std::string>& subs, TechDraw::DrawPage* page) :
    TaskDialog()
{
    widget  = new TaskLinkDim(part,subs,page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("LinkDimension"),
                                         widget->windowTitle(), true, 0);
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

#include "moc_TaskLinkDim.cpp"
