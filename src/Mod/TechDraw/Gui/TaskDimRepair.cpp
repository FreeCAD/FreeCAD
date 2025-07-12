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
#include <string>
#include <vector>

#include <QMessageBox>
#include <QTableWidgetItem>
#endif// #ifndef _PreComp_

#include <App/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "DimensionValidators.h"
#include "TaskDimRepair.h"
#include "ui_TaskDimRepair.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskDimRepair::TaskDimRepair(TechDraw::DrawViewDimension* inDvd)
    : ui(new Ui_TaskDimRepair),
      m_dim(inDvd)
{
    ui->setupUi(this);

    connect(ui->pbSelection, &QPushButton::clicked, this, &TaskDimRepair::slotUseSelection);

    saveDimState();
    setUiPrimary();
}

TaskDimRepair::~TaskDimRepair()
{}

void TaskDimRepair::setUiPrimary()
{
    setWindowTitle(QObject::tr("Dimension Repair"));
    ui->leName->setReadOnly(true);
    ui->leLabel->setReadOnly(true);

    ui->leName->setText(QString::fromStdString(m_dim->getNameInDocument()));
    ui->leLabel->setText(QString::fromStdString(m_dim->Label.getValue()));

    DrawViewPart* viewPart = m_dim->getViewPart();
    std::string objName = viewPart ?  viewPart->getNameInDocument() : "";
    std::string objLabel = viewPart ?  viewPart->Label.getValue() : "";
    ui->leObject2d->setText(QString::fromStdString(objName + " / " + objLabel));
    const std::vector<std::string>& subElements2d = m_dim->References2D.getSubValues();
    std::vector<std::string> labelsInOut(subElements2d.size());
    fillList(ui->lwGeometry2d, labelsInOut, subElements2d);

    QStringList headers;
    headers << tr("Object name") << tr("Object label") << tr("Sub-element");
    ui->twReferences3d->setHorizontalHeaderLabels(headers);

    ReferenceVector references3d = m_dim->getReferences3d();
    loadTableWidget(ui->twReferences3d, references3d);
}

void TaskDimRepair::saveDimState()
{
    m_saveMeasureType = m_dim->MeasureType.getValue();
    m_saveDimType = m_dim->Type.getValue();
    m_saveRefs3d = m_dim->getReferences3d();
    m_saveRefs2d = m_dim->getReferences2d();
    m_saveDvp = m_dim->getViewPart();
}

//restore the start conditions
void TaskDimRepair::restoreDimState()
{
    if (m_dim) {
        m_dim->setReferences2d(m_saveRefs2d);
        m_dim->setReferences3d(m_saveRefs3d);
    }
}

//similar to code in CommandCreateDims.cpp
//use the current selection to replace the references in dim
void TaskDimRepair::slotUseSelection()
{
    const std::vector<App::DocumentObject*> dimObjects =
        Gui::Selection().getObjectsOfType(TechDraw::DrawViewDimension::getClassTypeId());
    if (dimObjects.empty()) {
        //selection does not include a dimension, so we need to add our dimension to keep the
        //validators happy
        //bool accepted =
        static_cast<void>(Gui::Selection().addSelection(m_dim->getDocument()->getName(),
                                                        m_dim->getNameInDocument()));
    }
    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* dvp = TechDraw::getReferencesFromSelection(references2d, references3d);
     if (dvp != m_saveDvp) {
        int ret = QMessageBox::warning(Gui::getMainWindow(),
                                       QObject::tr("Incorrect Selection?"),
                                       QObject::tr("This will change the dimension's owner view. Continue?"),
                                       QMessageBox::Cancel | QMessageBox::Ok);
        if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    StringVector acceptableGeometry({ "Edge", "Vertex", "Face" });
    std::vector<int> minimumCounts({1, 1, 1});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys;//accept anything
    DimensionGeometry geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == DimensionGeometry::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Cannot make dimension from selection"));
        return;
    }
    //what 3d geometry configuration did we receive?
    DimensionGeometry geometryRefs3d(DimensionGeometry::isInvalid);
    if (geometryRefs2d == DimensionGeometry::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(
            dvp, references3d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
        if (geometryRefs3d == DimensionGeometry::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect selection"),
                                 QObject::tr("Cannot make dimension from selection"));
            return;
        }
    }

    m_toApply2d = references2d;
    if (references3d.empty()) {
        m_toApply3d.clear();
    } else {
        m_toApply3d = references3d;
    }
    updateUi();
}

void TaskDimRepair::updateUi()
{
    // if the dimension is very broken, it may not have a valid 2d view reference.  This can happen if the
    // restore process breaks and the reference target object does not get properly loaded.

    DrawViewPart* viewPart = m_dim->getViewPart();
    if (!viewPart && !m_toApply2d.empty()) {
        viewPart = Base::freecad_cast<DrawViewPart*>(m_toApply2d.front().getObject());
    }

    std::string objName = viewPart ?  viewPart->getNameInDocument() : "";
    std::string objLabel = viewPart ?  viewPart->Label.getValue() : "";
    ui->leObject2d->setText(QString::fromStdString(objName + " / " + objLabel));

    std::vector<std::string> subElements2d;
    for (auto& ref : m_toApply2d) {
        subElements2d.push_back(ref.getSubName());
    }
    std::vector<std::string> labelsInOut(subElements2d.size());
    fillList(ui->lwGeometry2d, labelsInOut, subElements2d);

    loadTableWidget(ui->twReferences3d, m_toApply3d);
}

void TaskDimRepair::loadTableWidget(QTableWidget* tw, ReferenceVector refs)
{
    tw->clearContents();
    tw->setRowCount(refs.size() + 1);
    size_t iRow = 0;
    for (auto& ref : refs) {
        QString qName = QString::fromStdString(ref.getObject()->getNameInDocument());
        QTableWidgetItem* itemName = new QTableWidgetItem(qName);
        itemName->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tw->setItem(iRow, 0, itemName);
        QString qLabel = QString::fromStdString(std::string(ref.getObject()->Label.getValue()));
        QTableWidgetItem* itemLabel = new QTableWidgetItem(qLabel);
        itemLabel->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tw->setItem(iRow, 1, itemLabel);
        QString qSubName = QString::fromStdString(ref.getSubName());
        QTableWidgetItem* itemSubName = new QTableWidgetItem(qSubName);
        itemSubName->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tw->setItem(iRow, 2, itemSubName);
        iRow++;
    }
}

void TaskDimRepair::fillList(QListWidget* lwItems, std::vector<std::string> labels,
                             std::vector<std::string> names)
{
    QListWidgetItem* item;
    QString qLabel;
    QString qName;
    QString qText;
    int labelCount = labels.size();
    int i = 0;
    lwItems->clear();
    for (; i < labelCount; i++) {
        qLabel = QString::fromStdString(labels[i]);
        qName = QString::fromStdString(names[i]);
        qText = QStringLiteral("%1 %2").arg(qName, qLabel);
        item = new QListWidgetItem(qText, lwItems);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item->setData(Qt::UserRole, qName);
    }
}
void TaskDimRepair::replaceReferences()
{
    if (!m_dim || m_toApply2d.empty()) {
        return;
    }

    m_dim->setReferences2d(m_toApply2d);

    if (!m_toApply3d.empty()) {
        m_dim->setReferences3d(m_toApply3d);
    }
}

bool TaskDimRepair::accept()
{
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    Gui::Command::openCommand(tr("Repair dimension").toStdString().c_str());
    replaceReferences();
    Gui::Command::commitCommand();

    m_dim->recomputeFeature();
    Gui::Selection().clearSelection();
    return true;
}

bool TaskDimRepair::reject()
{
    restoreDimState();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Selection().clearSelection();
    return false;
}

void TaskDimRepair::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgDimReference::TaskDlgDimReference(TechDraw::DrawViewDimension* inDvd)
    : TaskDialog()
{
    widget = new TaskDimRepair(inDvd);
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("TechDraw_DimensionRepair"), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}


TaskDlgDimReference::~TaskDlgDimReference()
{}

void TaskDlgDimReference::update()
{
    //widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgDimReference::open()
{}

void TaskDlgDimReference::clicked(int i)
{
    Q_UNUSED(i);
}

bool TaskDlgDimReference::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgDimReference::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskDimRepair.cpp>
