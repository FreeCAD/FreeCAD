/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView,bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_Additive_Loft",tr("Loft parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLoftParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonRemvove(bool)));
    connect(ui->checkBoxRuled, SIGNAL(toggled(bool)),
            this, SLOT(onRuled(bool)));
    connect(ui->checkBoxClosed, SIGNAL(toggled(bool)),
            this, SLOT(onClosed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    for(QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);
    
    //add the profiles
    for(auto obj : static_cast<PartDesign::Loft*>(LoftView->getObject())->Sections.getValues()) {
    
        QString objn = QString::fromLatin1(obj->getNameInDocument());
        if(!objn.isEmpty())
            ui->listWidgetReferences->addItem(objn);
    }        

    // activate and de-activate dialog elements as appropriate
    for(QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    updateUI(0);
}

void TaskLoftParameters::updateUI(int index)
{
    Q_UNUSED(index);
}

void TaskLoftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString objn = QString::fromStdString(msg.pObjectName);
                if(!objn.isEmpty())
                    ui->listWidgetReferences->addItem(objn);
            }
            else if (selectionMode == refRemove) {
                QString objn = QString::fromStdString(msg.pObjectName);
                if(!objn.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, objn);
            }
            clearButtons();
            //static_cast<ViewProviderLoft*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        }
        clearButtons();
        exitSelectionMode();
    }
}


bool TaskLoftParameters::referenceSelected(const Gui::SelectionChanges& msg) const {

    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;

        //every selection needs to be a profile in itself, hence currently only full objects are
        //supported, not individual edges of a part

        //change the references
        std::vector<App::DocumentObject*> refs = static_cast<PartDesign::Loft*>(vp->getObject())->Sections.getValues();
        App::DocumentObject* obj = vp->getObject()->getDocument()->getObject(msg.pObjectName);
        std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);

        if (selectionMode == refAdd) {
            if (f == refs.end())
                refs.push_back(obj);
            else
                return false; // duplicate selection
        } else {
            if (f != refs.end())
                refs.erase(f);
            else
                return false;
        }

        static_cast<PartDesign::Loft*>(vp->getObject())->Sections.setValues(refs);
        return true;
    }

    return false;
}

void TaskLoftParameters::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

void TaskLoftParameters::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
}

void TaskLoftParameters::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

TaskLoftParameters::~TaskLoftParameters()
{
    delete ui;
}

void TaskLoftParameters::changeEvent(QEvent * /*e*/)
{/*
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->spinOffset->blockSignals(true);
        ui->lengthEdit->blockSignals(true);
        ui->lengthEdit2->blockSignals(true);
        ui->lineFaceName->blockSignals(true);
        ui->changeMode->blockSignals(true);
        int index = ui->changeMode->currentIndex();
        ui->retranslateUi(proxy);
        ui->changeMode->clear();
        ui->changeMode->addItem(tr("Dimension"));
        ui->changeMode->addItem(tr("To last"));
        ui->changeMode->addItem(tr("To first"));
        ui->changeMode->addItem(tr("Up to face"));
        ui->changeMode->addItem(tr("Two dimensions"));
        ui->changeMode->setCurrentIndex(index);

        QStringList parts = ui->lineFaceName->text().split(QChar::fromLatin1(':'));
        QByteArray upToFace = ui->lineFaceName->property("FaceName").toByteArray();
        int faceId = -1;
        bool ok = false;
        if (upToFace.indexOf("Face") == 0) {
            faceId = upToFace.remove(0,4).toInt(&ok);
        }
#if QT_VERSION >= 0x040700
        ui->lineFaceName->setPlaceholderText(tr("No face selected"));
#endif
        ui->lineFaceName->setText(ok ?
                                  parts[0] + QString::fromLatin1(":") + tr("Face") + QString::number(faceId) :
                                  tr("No face selected"));
        ui->spinOffset->blockSignals(false);
        ui->lengthEdit->blockSignals(false);
        ui->lengthEdit2->blockSignals(false);
        ui->lineFaceName->blockSignals(false);
        ui->changeMode->blockSignals(false);
    }*/
}

void TaskLoftParameters::onClosed(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Closed.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onRuled(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Ruled.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onRefButtonAdd(bool checked) {
    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}

void TaskLoftParameters::onRefButtonRemvove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refRemove;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}



//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj)
   : TaskDlgSketchBasedParameters(LoftView)
{
    assert(LoftView);
    parameter  = new TaskLoftParameters(LoftView,newObj);

    Content.push_back(parameter);
}

TaskDlgLoftParameters::~TaskDlgLoftParameters()
{
}


bool TaskDlgLoftParameters::accept()
{
    // TODO Fill this with commands (2015-09-11, Fat-Zer)
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(vp->getObject());

    for(App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().hide(\"%s\")", obj->getNameInDocument());
    }


    return TaskDlgSketchBasedParameters::accept ();
}

//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
