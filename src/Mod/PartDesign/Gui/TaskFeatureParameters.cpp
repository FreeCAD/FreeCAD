/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
#include <QMessageBox>
#include <QCheckBox>
#endif

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/Body.h>

#include "TaskFeatureParameters.h"
#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/*********************************************************************
 *                      Task Feature Parameters                      *
 *********************************************************************/

TaskFeatureParameters::TaskFeatureParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname.c_str()),parname,true, parent),
      vp(vp), blockUpdate(false)
{
    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);
}

void TaskFeatureParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj)
        this->vp = nullptr;
}

void TaskFeatureParameters::slotUndoDocument(const Gui::Document &)
{
    refresh();
}

void TaskFeatureParameters::slotRedoDocument(const Gui::Document &)
{
    refresh();
}

void TaskFeatureParameters::refresh()
{
    if (!this->vp || !checkBoxNewSolid)
        return;

    auto feat = Base::freecad_dynamic_cast<PartDesign::Feature>(this->vp->getObject());
    if (feat && feat->NewSolid.getValue() != checkBoxNewSolid->isChecked()) {
        QSignalBlocker guard(checkBoxNewSolid);
        checkBoxNewSolid->setChecked(feat->NewSolid.getValue());
    }
}

void TaskFeatureParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    recomputeFeature();
}

void TaskFeatureParameters::recomputeFeature()
{
    if (!blockUpdate) {
        App::DocumentObject* obj = vp->getObject ();
        assert (obj);
        obj->getDocument()->recomputeFeature ( obj );
    }
}

void TaskFeatureParameters::onNewSolidChanged(bool on)
{
    if (vp) {
        PartDesign::Feature* pc =
            Base::freecad_dynamic_cast<PartDesign::Feature>(vp->getObject());
        if (pc) {
            pc->NewSolid.setValue(on);
            recomputeFeature();
        }
    }
}

void TaskFeatureParameters::addNewSolidCheckBox(QWidget *widget)
{
    if (!vp || !vp->getObject())
        return;

    auto *addsub = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(vp->getObject());
    if (addsub && addsub->getAddSubType() != PartDesign::FeatureAddSub::Additive)
        return;

    QBoxLayout * layout = qobject_cast<QBoxLayout*>(widget->layout());
    if (layout) {
        checkBoxNewSolid = new QCheckBox(widget);
        checkBoxNewSolid->setText(tr("New solid"));
        checkBoxNewSolid->setToolTip(tr("Make a new seperate solid using this feature"));
        layout->insertWidget(0, checkBoxNewSolid);
        connect(checkBoxNewSolid, SIGNAL(toggled(bool)), this, SLOT(onNewSolidChanged(bool)));
    }
}

/*********************************************************************
 *                            Task Dialog                            *
 *********************************************************************/
TaskDlgFeatureParameters::TaskDlgFeatureParameters(PartDesignGui::ViewProvider *vp)
    : TaskDialog(),vp(vp)
{
    assert(vp);
}

TaskDlgFeatureParameters::~TaskDlgFeatureParameters()
{

}

bool TaskDlgFeatureParameters::accept() {
    App::DocumentObject* feature = vp->getObject();

    try {
        // Iterate over parameter dialogs and apply all parameters from them
        for ( QWidget *wgt : Content ) {
            TaskFeatureParameters *param = qobject_cast<TaskFeatureParameters *> (wgt);
            if(!param)
                continue;
            
            param->saveHistory ();
            param->apply ();
        }
        // Make sure the feature is what we are expecting
        // Should be fine but you never know...
        if ( !feature->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ) {
            throw Base::TypeError("Bad object processed in the feature dialog.");
        }

        Gui::cmdAppDocument(feature, "recompute()");

        if (!feature->isValid()) {
            throw Base::RuntimeError(vp->getObject()->getStatusString());
        }

        App::DocumentObject* previous = static_cast<PartDesign::Feature*>(feature)->getBaseObject(/* silent = */ true );

        // detach the task panel from the selection to avoid to invoke
        // eventually onAddSelection when the selection changes
        std::vector<QWidget*> subwidgets = getDialogContent();
        for (auto it : subwidgets) {
            TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
            if (param)
                param->detachSelection();
        }

        Gui::cmdGuiDocument(feature, "resetEdit()");
        Gui::Command::commitCommand();

        Gui::cmdAppObjectHide(previous);

    } catch (const Base::Exception& e) {
        // Generally the only thing that should fail is feature->isValid() others should be fine
#if (QT_VERSION >= 0x050000)
        QString errorText = QApplication::translate(feature->getTypeId().getName(), e.what());
#else
        QString errorText = QApplication::translate(feature->getTypeId().getName(), e.what(), 0, QApplication::UnicodeUTF8);
#endif
        QMessageBox::warning(Gui::getMainWindow(), tr("Input error"), errorText);
        return false;
    }

    return true;
}

bool TaskDlgFeatureParameters::reject()
{
    PartDesign::Feature* feature = static_cast<PartDesign::Feature*>(vp->getObject());

    PartDesign::Body* body = PartDesign::Body::findBodyOf(feature);

    // Find out previous feature we won't be able to do it after abort
    // (at least in the body case)
    App::DocumentObject* previous = feature->getBaseObject(/* silent = */ true );

    // detach the task panel from the selection to avoid to invoke
    // eventually onAddSelection when the selection changes
    std::vector<QWidget*> subwidgets = getDialogContent();
    for (auto it : subwidgets) {
        TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
        if (param)
            param->detachSelection();
    }

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object make the previous feature visible again
    if (!Gui::Application::Instance->getViewProvider(feature)) {
        // Make the tip or the previous feature visible again with preference to the previous one
        // TODO: ViewProvider::onDelete has the same code. May be this one is excess?
        if (previous && Gui::Application::Instance->getViewProvider(previous)) {
            Gui::Application::Instance->getViewProvider(previous)->show();
        } else if (body != NULL) {
            App::DocumentObject* tip = body->Tip.getValue();
            if (tip && Gui::Application::Instance->getViewProvider(tip)) {
                Gui::Application::Instance->getViewProvider(tip)->show();
            }
        }
    }

    return true;
}

#include "moc_TaskFeatureParameters.cpp"
