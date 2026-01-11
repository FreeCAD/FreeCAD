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

#include <QApplication>
#include <QMessageBox>


#include <App/DocumentObserver.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>

#include "ui_TaskPreviewParameters.h"

#include "TaskFeatureParameters.h"
#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/*********************************************************************
 *                      Task Feature Parameters                      *
 *********************************************************************/

TaskPreviewParameters::TaskPreviewParameters(ViewProvider* vp, QWidget* parent)
    : TaskBox(BitmapFactory().pixmap("tree-pre-sel"), tr("Preview"), true, parent)
    , vp(vp)
    , ui(std::make_unique<Ui_TaskPreviewParameters>())
{
    vp->showPreviousFeature(!hGrp->GetBool("ShowFinal", false));
    vp->showPreview(hGrp->GetBool("ShowTransparentPreview", true));

    auto* proxy = new QWidget(this);
    ui->setupUi(proxy);

    ui->showFinalCheckBox->setChecked(vp->isVisible());
    ui->showTransparentPreviewCheckBox->setChecked(vp->isPreviewEnabled());

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(
        ui->showTransparentPreviewCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &TaskPreviewParameters::onShowPreviewChanged
    );
    connect(
        ui->showFinalCheckBox,
        &QCheckBox::checkStateChanged,
        this,
        &TaskPreviewParameters::onShowFinalChanged
    );
#else
    connect(
        ui->showTransparentPreviewCheckBox,
        &QCheckBox::stateChanged,
        this,
        &TaskPreviewParameters::onShowPreviewChanged
    );
    connect(
        ui->showFinalCheckBox,
        &QCheckBox::stateChanged,
        this,
        &TaskPreviewParameters::onShowFinalChanged
    );
#endif

    groupLayout()->addWidget(proxy);
}

TaskPreviewParameters::~TaskPreviewParameters() = default;

void TaskPreviewParameters::onShowFinalChanged(bool show)
{
    vp->showPreviousFeature(!show);
}

void TaskPreviewParameters::onShowPreviewChanged(bool show)
{
    vp->showPreview(show);
}

TaskFeatureParameters::TaskFeatureParameters(
    PartDesignGui::ViewProvider* vp,
    QWidget* parent,
    const std::string& pixmapname,
    const QString& parname
)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname.c_str()), parname, true, parent)
    , vp(vp)
    , blockUpdate(false)
{
    Gui::Document* doc = vp->getDocument();
    this->attachDocument(doc);
}

void TaskFeatureParameters::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (this->vp == &Obj) {
        this->vp = nullptr;
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
        auto* feature = getObject<PartDesign::Feature>();
        assert(feature);

        feature->recomputeFeature();
        feature->recomputePreview();
    }
}

/*********************************************************************
 *                            Task Dialog                            *
 *********************************************************************/
TaskDlgFeatureParameters::TaskDlgFeatureParameters(PartDesignGui::ViewProvider* vp)
    : preview(new TaskPreviewParameters(vp))
    , vp(vp)
{
    assert(vp);
}

TaskDlgFeatureParameters::~TaskDlgFeatureParameters() = default;

bool TaskDlgFeatureParameters::accept()
{
    App::DocumentObject* feature = getObject();
    bool isUpdateBlocked = false;
    try {
        // Iterate over parameter dialogs and apply all parameters from them
        for (QWidget* wgt : Content) {
            TaskFeatureParameters* param = qobject_cast<TaskFeatureParameters*>(wgt);
            if (!param) {
                continue;
            }

            param->saveHistory();
            param->apply();
            isUpdateBlocked |= param->isUpdateBlocked();
        }
        // Make sure the feature is what we are expecting
        // Should be fine but you never know...
        if (!feature->isDerivedFrom<PartDesign::Feature>()) {
            throw Base::TypeError("Bad object processed in the feature dialog.");
        }

        if (isUpdateBlocked) {
            Gui::cmdAppDocument(feature, "recompute()");
        }
        else {
            // object was already computed, nothing more to do with it...
            Gui::cmdAppDocument(feature, "purgeTouched()");

            if (!feature->isValid()) {
                throw Base::RuntimeError(getObject()->getStatusString());
            }

            // ...but touch parents to signal the change...
            for (auto obj : feature->getInList()) {
                obj->touch();
            }
            // ...and recompute them
            Gui::cmdAppDocument(feature->getDocument(), "recompute()");
        }

        if (!feature->isValid()) {
            throw Base::RuntimeError(getObject()->getStatusString());
        }

        App::DocumentObject* previous = static_cast<PartDesign::Feature*>(feature)->getBaseObject(
            /* silent = */ true
        );
        Gui::cmdAppObjectHide(previous);

        // detach the task panel from the selection to avoid to invoke
        // eventually onAddSelection when the selection changes
        std::vector<QWidget*> subwidgets = getDialogContent();
        for (auto it : subwidgets) {
            TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
            if (param) {
                param->detachSelection();
            }
        }

        Gui::cmdGuiDocument(feature, "resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {

        QString errorText = QString::fromUtf8(e.what());
        QString statusText = QString::fromUtf8(getObject()->getStatusString());

        // generic, fallback error message
        if (errorText == QStringLiteral("Error") || errorText.isEmpty()) {
            if (!statusText.isEmpty() && statusText != QStringLiteral("Error")) {
                errorText = statusText;
            }
            else {
                errorText = tr(
                    "The feature could not be created with the given parameters.\n"
                    "The geometry may be invalid or the parameters may be incompatible.\n"
                    "Please adjust the parameters and try again."
                );
            }
        }
        QMessageBox::warning(Gui::getMainWindow(), tr("Input error"), errorText);
        return false;
    }
    return true;
}

bool TaskDlgFeatureParameters::reject()
{
    auto feature = getObject<PartDesign::Feature>();
    App::DocumentObjectWeakPtrT weakptr(feature);
    App::Document* document = feature->getDocument();

    PartDesign::Body* body = PartDesign::Body::findBodyOf(feature);

    // Find out previous feature we won't be able to do it after abort
    // (at least in the body case)
    App::DocumentObject* previous = feature->getBaseObject(/* silent = */ true);

    // detach the task panel from the selection to avoid to invoke
    // eventually onAddSelection when the selection changes
    std::vector<QWidget*> subwidgets = getDialogContent();
    for (auto it : subwidgets) {
        TaskSketchBasedParameters* param = qobject_cast<TaskSketchBasedParameters*>(it);
        if (param) {
            param->detachSelection();
        }
    }

    // roll back the done things which may delete the feature
    Gui::Command::abortCommand();

    // if abort command deleted the object make the previous feature visible again
    if (weakptr.expired()) {
        // Make the tip or the previous feature visible again with preference to the previous one
        // TODO: ViewProvider::onDelete has the same code. May be this one is excess?
        if (previous && Gui::Application::Instance->getViewProvider(previous)) {
            Gui::Application::Instance->getViewProvider(previous)->show();
        }
        else if (body) {
            App::DocumentObject* tip = body->Tip.getValue();
            if (tip && Gui::Application::Instance->getViewProvider(tip)) {
                Gui::Application::Instance->getViewProvider(tip)->show();
            }
        }
    }

    Gui::cmdAppDocument(document, "recompute()");
    Gui::cmdGuiDocument(document, "resetEdit()");

    return true;
}

#include "moc_TaskFeatureParameters.cpp"
