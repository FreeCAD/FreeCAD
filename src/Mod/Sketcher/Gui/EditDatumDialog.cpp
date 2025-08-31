/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
/// Qt Include Files
#include <Inventor/sensors/SoSensor.h>
#include <QApplication>
#include <QDialog>
#endif

#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Document.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <App/Datums.h>

#include "EditDatumDialog.h"
#include "CommandSketcherTools.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SketcherSettings.h"
#include "ui_InsertDatum.h"

#include <numeric>


using namespace SketcherGui;

/* TRANSLATOR SketcherGui::EditDatumDialog */

bool SketcherGui::checkConstraintName(const Sketcher::SketchObject* sketch,
                                      std::string constraintName)
{
    if (constraintName != Base::Tools::getIdentifier(constraintName)) {
        Gui::NotifyUserError(
            sketch,
            QT_TRANSLATE_NOOP("Notifications", "Value Error"),
            QT_TRANSLATE_NOOP("Notifications",
                              "Invalid constraint name (must only contain alphanumericals and "
                              "underscores, and must not start with digit)"));
        return false;
    }

    return true;
}


EditDatumDialog::EditDatumDialog(ViewProviderSketch* vp, int ConstrNbr)
    : ConstrNbr(ConstrNbr)
    , success(false)
{
    sketch = vp->getSketchObject();
    const std::vector<Sketcher::Constraint*>& Constraints = sketch->Constraints.getValues();
    Constr = Constraints[ConstrNbr];
}

EditDatumDialog::EditDatumDialog(Sketcher::SketchObject* pcSketch, int ConstrNbr)
    : sketch(pcSketch)
    , ConstrNbr(ConstrNbr)
{
    const std::vector<Sketcher::Constraint*>& Constraints = sketch->Constraints.getValues();
    Constr = Constraints[ConstrNbr];
}

EditDatumDialog::~EditDatumDialog()
{}

int EditDatumDialog::exec(bool atCursor)
{
    // Return if constraint doesn't have editable value
    if (Constr->isDimensional()) {

        if (sketch->hasConflicts()) {
            Gui::TranslatedUserWarning(sketch,
                                       QObject::tr("Dimensional constraint"),
                                       QObject::tr("Not allowed to edit the datum because the "
                                                   "sketch contains conflicting constraints"));
            return QDialog::Rejected;
        }

        Base::Quantity init_val;

        QDialog dlg(Gui::getMainWindow());
        if (!ui_ins_datum) {
            ui_ins_datum.reset(new Ui_InsertDatum);
            ui_ins_datum->setupUi(&dlg);
        }
        double datum = Constr->getValue();

        ui_ins_datum->labelEdit->setEntryName(QByteArray("DatumValue"));
        if (Constr->Type == Sketcher::Angle) {
            datum = Base::toDegrees<double>(datum);
            dlg.setWindowTitle(tr("Insert Angle"));
            init_val.setUnit(Base::Unit::Angle);
            ui_ins_datum->label->setText(tr("Angle:"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherAngle"));
        }
        else if (Constr->Type == Sketcher::Radius) {
            dlg.setWindowTitle(tr("Insert Radius"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum->label->setText(tr("Radius:"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }
        else if (Constr->Type == Sketcher::Diameter) {
            dlg.setWindowTitle(tr("Insert Diameter"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum->label->setText(tr("Diameter:"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }
        else if (Constr->Type == Sketcher::Weight) {
            dlg.setWindowTitle(tr("Insert Weight"));
            ui_ins_datum->label->setText(tr("Weight:"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherWeight"));
        }
        else if (Constr->Type == Sketcher::SnellsLaw) {
            dlg.setWindowTitle(tr("Refractive Index Ratio", "Constraint_SnellsLaw"));
            ui_ins_datum->label->setText(tr("Ratio n2/n1:", "Constraint_SnellsLaw"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherRefrIndexRatio"));
            ui_ins_datum->labelEdit->setSingleStep(0.05);
        }
        else {
            dlg.setWindowTitle(tr("Insert Length"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum->label->setText(tr("Length:"));
            ui_ins_datum->labelEdit->setParamGrpPath(
                QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }

        init_val.setValue(datum);

        ui_ins_datum->labelEdit->setValue(init_val);
        ui_ins_datum->labelEdit->pushToHistory();
        ui_ins_datum->labelEdit->selectNumber();
        ui_ins_datum->labelEdit->bind(sketch->Constraints.createPath(ConstrNbr));
        ui_ins_datum->name->setText(QString::fromStdString(Constr->Name));

        ui_ins_datum->cbDriving->setChecked(!Constr->isDriving);

        connect(ui_ins_datum->cbDriving,
                &QCheckBox::toggled,
                this,
                &EditDatumDialog::drivingToggled);
        connect(ui_ins_datum->labelEdit,
                qOverload<const Base::Quantity&>(&Gui::QuantitySpinBox::valueChanged),
                this,
                &EditDatumDialog::datumChanged);
        connect(ui_ins_datum->labelEdit,
                &Gui::QuantitySpinBox::showFormulaDialog,
                this,
                &EditDatumDialog::formEditorOpened);
        connect(&dlg, &QDialog::accepted, this, &EditDatumDialog::accepted);
        connect(&dlg, &QDialog::rejected, this, &EditDatumDialog::rejected);

        if (atCursor) {
            dlg.show();  // Need to show the dialog so geometry is computed
            QRect pg = dlg.parentWidget()->geometry();
            int Xmin = pg.x() + 10;
            int Ymin = pg.y() + 10;
            int Xmax = pg.x() + pg.width() - dlg.geometry().width() - 10;
            int Ymax = pg.y() + pg.height() - dlg.geometry().height() - 10;
            int x = Xmax < Xmin ? (Xmin + Xmax) / 2
                                : std::min(std::max(QCursor::pos().x(), Xmin), Xmax);
            int y = Ymax < Ymin ? (Ymin + Ymax) / 2
                                : std::min(std::max(QCursor::pos().y(), Ymin), Ymax);
            dlg.setGeometry(x, y, dlg.geometry().width(), dlg.geometry().height());
        }

        return dlg.exec();
    }

    return QDialog::Rejected;
}

void EditDatumDialog::accepted()
{
    Base::Quantity newQuant = ui_ins_datum->labelEdit->value();
    if (Constr->Type == Sketcher::SnellsLaw || Constr->Type == Sketcher::Weight
        || !newQuant.isDimensionless()) {

        // save the value for the history
        ui_ins_datum->labelEdit->pushToHistory();

        double newDatum = newQuant.getValue();

        try {

            /*if (ui_ins_datum->cbDriving->isChecked() == Constr->isDriving) {
                Gui::cmdAppObjectArgs(sketch, "toggleDriving(%i)", ConstrNbr);
            }*/

            if (!ui_ins_datum->cbDriving->isChecked()) {
                if (ui_ins_datum->labelEdit->hasExpression()) {
                    ui_ins_datum->labelEdit->apply();
                }
                else {
                    auto unitString = newQuant.getUnit().getString();
                    unitString = Base::Tools::escapeQuotesFromString(unitString);

                    performAutoScale(newDatum);

                    Gui::cmdAppObjectArgs(sketch,
                                          "setDatum(%i,App.Units.Quantity('%f %s'))",
                                          ConstrNbr,
                                          newDatum,
                                          unitString);
                }
            }

            std::string constraintName = ui_ins_datum->name->text().trimmed().toStdString();
            std::string currConstraintName = sketch->Constraints[ConstrNbr]->Name;

            if (constraintName != currConstraintName) {
                if (!SketcherGui::checkConstraintName(sketch, constraintName)) {
                    constraintName = currConstraintName;
                }

                Gui::cmdAppObjectArgs(sketch,
                                      "renameConstraint(%d, u'%s')",
                                      ConstrNbr,
                                      constraintName.c_str());
            }

            Gui::Command::commitCommand();

            // THIS IS A WORK-AROUND NOT TO DELAY 0.19 RELEASE
            //
            // depsAreTouched is not returning true in this case:
            //  https://forum.freecad.org/viewtopic.php?f=3&t=55633&p=481061#p478477
            //
            // It appears related to a drastic change in how dependencies are calculated, see:
            //  https://forum.freecad.org/viewtopic.php?f=3&t=55633&p=481061#p481061
            //
            // This is NOT the solution, as there is no point in systematically executing the
            // ExpressionEngine on every dimensional constraint change. Just a quick fix to avoid
            // clearly unwanted behaviour in absence of time to actually fix the root cause.

            // if (sketch->noRecomputes && sketch->ExpressionEngine.depsAreTouched()) {
            sketch->ExpressionEngine.execute();
            sketch->solve();
            //}

            tryAutoRecompute(sketch);
            success = true;
        }
        catch (const Base::Exception& e) {
            Gui::NotifyUserError(sketch,
                                 QT_TRANSLATE_NOOP("Notifications", "Value Error"),
                                 e.what());

            Gui::Command::abortCommand();

            if (sketch->noRecomputes) {  // if setdatum failed, it is highly likely that solver
                                         // information is invalid.
                sketch->solve();
            }
        }
    }
}

void EditDatumDialog::rejected()
{
    Gui::Command::abortCommand();
    sketch->recomputeFeature();
}

bool EditDatumDialog::isSuccess()
{
    return success;
}

void EditDatumDialog::drivingToggled(bool state)
{
    if (state) {
        ui_ins_datum->labelEdit->setToLastUsedValue();
    }
    sketch->setDriving(ConstrNbr, !state);
    if (!sketch->noRecomputes) {  // if noRecomputes, solve() is already done by setDriving()
        sketch->solve();
    }
}

void EditDatumDialog::datumChanged()
{
    if (ui_ins_datum->labelEdit->text()
        != std::as_const(ui_ins_datum->labelEdit)->getHistory()[0]) {
        ui_ins_datum->cbDriving->setChecked(false);
    }
}

void EditDatumDialog::formEditorOpened(bool state)
{
    if (state) {
        ui_ins_datum->cbDriving->setChecked(false);
    }
}


// This function checks an object's visible flag recursively in a Gui::Document
// assuming that lastParent (if provided) is visible
bool isVisibleUpTo(App::DocumentObject* obj, Gui::Document* doc, App::DocumentObject* lastParent)
{
    while (obj && obj != lastParent) {
        auto parentviewprovider = doc->getViewProvider(obj);

        if (!parentviewprovider || !parentviewprovider->isVisible()) {
            return false;
        }
        obj = obj->getFirstParent();
    }
    return true;
}
bool hasVisualFeature(App::DocumentObject* obj, App::DocumentObject* rootObj, Gui::Document* doc)
{
    auto docObjects = doc->getDocument()->getObjects();
    for (auto object : docObjects) {

        // Presumably, the sketch that is being edited has visual features, but
        // that's not interesting
        if (object == obj) {
            continue;
        }

        // No need to continue analysis if the object's visible flag is down
        bool visible = isVisibleUpTo(object, doc, rootObj);
        if (!visible) {
            continue;
        }

        App::DocumentObject* link = object->getLinkedObject();
        if (link->getDocument() != doc->getDocument()) {
            Gui::Document* linkDoc = Gui::Application::Instance->getDocument(link->getDocument());
            if (linkDoc && hasVisualFeature(link, link, linkDoc)) {
                return true;
            }
            continue;
        }

        // Skip objects that are not of geometric nature
        if (!object->isDerivedFrom<App::GeoFeature>()) {
            continue;
        }

        // Skip datum objects
        if (object->isDerivedFrom<App::DatumElement>()) {
            continue;
        }

        // Skip container objects because getting their bounging box might
        // return a valid bounding box around annotations or datums
        if (object->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            continue;
        }

        // Get the bounding box of the object
        auto viewProvider = doc->getViewProvider(object);
        if (viewProvider && viewProvider->getBoundingBox().IsValid()) {
            return true;
        }
    }
    return false;
}

void EditDatumDialog::performAutoScale(double newDatum)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");
    long autoScaleMode =
        hGrp->GetInt("AutoScaleMode", static_cast<int>(SketcherGui::AutoScaleMode::Always));

    // There is a single constraint in the sketch so it can
    // be used as a reference to scale the geometries around the origin
    // if there are external geometries, it is safe to assume that the sketch
    // was drawn with these geometries as scale references (use <= 2 because
    // the sketch axis are considered as external geometries)
    if ((autoScaleMode == static_cast<int>(SketcherGui::AutoScaleMode::Always)
         || (autoScaleMode
                 == static_cast<int>(SketcherGui::AutoScaleMode::WhenNoScaleFeatureIsVisible)
             && !hasVisualFeature(sketch, nullptr, Gui::Application::Instance->activeDocument())))
        && sketch->getExternalGeometryCount() <= 2) {
        try {
            // Handle the case where multiple datum constraints are present but only one is scale
            // defining e.g. a bunch of angle constraints and a single length constraint
            int scaleDefiningConstraint = sketch->getSingleScaleDefiningConstraint();
            if (scaleDefiningConstraint != ConstrNbr) {
                return;
            }

            double oldDatum = sketch->getDatum(ConstrNbr);
            double scaleFactor = newDatum / oldDatum;
            float initLabelDistance = sketch->Constraints[ConstrNbr]->LabelDistance;
            float initLabelPosition = sketch->Constraints[ConstrNbr]->LabelPosition;
            centerScale(sketch, scaleFactor);
            sketch->setLabelDistance(ConstrNbr, initLabelDistance * scaleFactor);

            // Label position or radii and diameters represent an angle, so
            // they should not be scaled
            Sketcher::ConstraintType type = sketch->Constraints[ConstrNbr]->Type;
            if (type == Sketcher::ConstraintType::Radius
                || type == Sketcher::ConstraintType::Diameter) {
                sketch->setLabelPosition(ConstrNbr, initLabelPosition);
            }
            else {
                sketch->setLabelPosition(ConstrNbr, initLabelPosition * scaleFactor);
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().error("Exception performing autoscale: %s\n", e.what());
        }
    }
}

#include "moc_EditDatumDialog.cpp"
