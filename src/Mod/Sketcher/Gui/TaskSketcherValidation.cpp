/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Precision.hxx>
#include <QDoubleValidator>
#include <QLocale>
#include <QMessageBox>
#include <algorithm>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Notifications.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskSketcherValidation.h"
#include "ui_TaskSketcherValidation.h"


using namespace SketcherGui;
using namespace Gui::TaskView;

/* TRANSLATOR SketcherGui::SketcherValidation */

SketcherValidation::SketcherValidation(Sketcher::SketchObject* Obj, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui_TaskSketcherValidation())
    , sketch(Obj)
    , sketchAnalyser(Obj)
    , coincidenceRoot(nullptr)
{
    ui->setupUi(this);
    setupConnections();

    ui->fixButton->setEnabled(false);
    ui->fixConstraint->setEnabled(false);
    ui->fixDegenerated->setEnabled(false);
    ui->swapReversed->setEnabled(false);
    ui->checkBoxIgnoreConstruction->setEnabled(true);
    double tolerances[8] = {Precision::Confusion() / 100,
                            Precision::Confusion() / 10,
                            Precision::Confusion(),
                            Precision::Confusion() * 10,
                            Precision::Confusion() * 100,
                            Precision::Confusion() * 1000,
                            Precision::Confusion() * 10000,
                            Precision::Confusion() * 100000};

    QLocale loc;
    for (int i = 0; i < 8; i++) {
        ui->comboBoxTolerance->addItem(loc.toString(tolerances[i]), QVariant(tolerances[i]));
    }
    ui->comboBoxTolerance->setCurrentIndex(5);
    ui->comboBoxTolerance->setEditable(true);
    ui->comboBoxTolerance->setValidator(new QDoubleValidator(0, 10, 10, this));
}

SketcherValidation::~SketcherValidation()
{
    hidePoints();
}

void SketcherValidation::setupConnections()
{
    connect(ui->findButton, &QPushButton::clicked, this, &SketcherValidation::onFindButtonClicked);
    connect(ui->fixButton, &QPushButton::clicked, this, &SketcherValidation::onFixButtonClicked);
    connect(ui->highlightButton,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onHighlightButtonClicked);
    connect(ui->findConstraint,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onFindConstraintClicked);
    connect(ui->fixConstraint,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onFixConstraintClicked);
    connect(ui->findReversed,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onFindReversedClicked);
    connect(ui->swapReversed,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onSwapReversedClicked);
    connect(ui->orientLockEnable,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onOrientLockEnableClicked);
    connect(ui->orientLockDisable,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onOrientLockDisableClicked);
    connect(ui->delConstrExtr,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onDelConstrExtrClicked);
    connect(ui->findDegenerated,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onFindDegeneratedClicked);
    connect(ui->fixDegenerated,
            &QPushButton::clicked,
            this,
            &SketcherValidation::onFixDegeneratedClicked);
}

void SketcherValidation::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void SketcherValidation::onFindButtonClicked()
{
    if (sketch.expired()) {
        return;
    }

    double prec = Precision::Confusion();
    bool ok;
    double conv;

    conv = QLocale::system().toDouble(ui->comboBoxTolerance->currentText(), &ok);

    if (ok) {
        prec = conv;
    }
    else {
        QVariant v = ui->comboBoxTolerance->itemData(ui->comboBoxTolerance->currentIndex());
        if (v.isValid()) {
            prec = v.toDouble();
        }
    }

    sketchAnalyser.detectMissingPointOnPointConstraints(
        prec,
        !ui->checkBoxIgnoreConstruction->isChecked());

    std::vector<Sketcher::ConstraintIds>& vertexConstraints =
        sketchAnalyser.getMissingPointOnPointConstraints();

    std::vector<Base::Vector3d> points;
    points.reserve(vertexConstraints.size());

    for (auto vc : vertexConstraints) {
        points.push_back(vc.v);
    }

    hidePoints();
    if (vertexConstraints.empty()) {
        Gui::TranslatedNotification(*sketch,
                                    tr("No missing coincidences"),
                                    tr("No missing coincidences found"));

        ui->fixButton->setEnabled(false);
    }
    else {
        showPoints(points);
        Gui::TranslatedUserWarning(
            *sketch,
            tr("Missing coincidences"),
            tr("%1 missing coincidences found").arg(vertexConstraints.size()));

        ui->fixButton->setEnabled(true);
    }
}

void SketcherValidation::onFixButtonClicked()
{
    if (sketch.expired()) {
        return;
    }

    // undo command open
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("add coincident constraint");

    sketchAnalyser.makeMissingPointOnPointCoincident();

    ui->fixButton->setEnabled(false);
    hidePoints();

    // finish the transaction and update
    Gui::WaitCursor wc;
    doc->commitTransaction();
    doc->recompute();
}

void SketcherValidation::onHighlightButtonClicked()
{
    if (sketch.expired()) {
        return;
    }

    std::vector<Base::Vector3d> points;

    points = sketchAnalyser.getOpenVertices();

    hidePoints();
    if (!points.empty()) {
        showPoints(points);
    }
}

void SketcherValidation::onFindConstraintClicked()
{
    if (sketch.expired()) {
        return;
    }

    if (sketch->evaluateConstraints()) {
        Gui::TranslatedNotification(*sketch,
                                    tr("No invalid constraints"),
                                    tr("No invalid constraints found"));

        ui->fixConstraint->setEnabled(false);
    }
    else {
        Gui::TranslatedUserError(*sketch,
                                 tr("Invalid constraints"),
                                 tr("Invalid constraints found"));

        ui->fixConstraint->setEnabled(true);
    }
}

void SketcherValidation::onFixConstraintClicked()
{
    if (sketch.expired()) {
        return;
    }

    sketch->validateConstraints();
    ui->fixConstraint->setEnabled(false);
}

void SketcherValidation::onFindReversedClicked()
{
    if (sketch.expired()) {
        return;
    }

    std::vector<Base::Vector3d> points;
    const std::vector<Part::Geometry*>& geom = sketch->getExternalGeometry();
    for (std::size_t i = 0; i < geom.size(); i++) {
        Part::Geometry* g = geom[i];
        // only arcs of circles need to be repaired. Arcs of ellipse were so broken there should be
        // nothing to repair from.
        if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle* segm = static_cast<const Part::GeomArcOfCircle*>(g);
            if (segm->isReversed()) {
                points.push_back(segm->getStartPoint(/*emulateCCW=*/true));
                points.push_back(segm->getEndPoint(/*emulateCCW=*/true));
            }
        }
    }
    hidePoints();
    if (!points.empty()) {
        int nc = sketch->port_reversedExternalArcs(/*justAnalyze=*/true);
        showPoints(points);
        if (nc > 0) {
            Gui::TranslatedUserWarning(
                *sketch,
                tr("Reversed external geometry"),
                tr("%1 reversed external-geometry arcs were found. Their endpoints are"
                   " encircled in 3d view.\n\n"
                   "%2 constraints are linking to the endpoints. The constraints have"
                   " been listed in Report view (menu View -> Panels -> Report view).\n\n"
                   "Click \"Swap endpoints in constraints\" button to reassign endpoints."
                   " Do this only once to sketches created in FreeCAD older than v0.15")
                    .arg(points.size() / 2)
                    .arg(nc));

            ui->swapReversed->setEnabled(true);
        }
        else {
            Gui::TranslatedUserWarning(
                *sketch,
                tr("Reversed external geometry"),
                tr("%1 reversed external-geometry arcs were found. Their endpoints are "
                   "encircled in 3d view.\n\n"
                   "However, no constraints linking to the endpoints were found.")
                    .arg(points.size() / 2));

            ui->swapReversed->setEnabled(false);
        }
    }
    else {
        Gui::TranslatedNotification(*sketch,
                                    tr("Reversed external geometry"),
                                    tr("No reversed external-geometry arcs were found."));
    }
}

void SketcherValidation::onSwapReversedClicked()
{
    if (sketch.expired()) {
        return;
    }

    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Sketch porting");

    int n = sketch->port_reversedExternalArcs(/*justAnalyze=*/false);
    Gui::TranslatedNotification(
        *sketch,
        tr("Reversed external geometry"),
        tr("%1 changes were made to constraints linking to endpoints of reversed arcs.").arg(n));

    hidePoints();
    ui->swapReversed->setEnabled(false);

    doc->commitTransaction();
}

void SketcherValidation::onOrientLockEnableClicked()
{
    if (sketch.expired()) {
        return;
    }

    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Constraint orientation lock");

    int n = sketch->changeConstraintsLocking(/*bLock=*/true);
    Gui::TranslatedNotification(
        *sketch,
        tr("Constraint orientation locking"),
        tr("Orientation locking was enabled and recomputed for %1 constraints. The"
           " constraints have been listed in Report view (menu View -> Panels ->"
           " Report view).")
            .arg(n));

    doc->commitTransaction();
}

void SketcherValidation::onOrientLockDisableClicked()
{
    if (sketch.expired()) {
        return;
    }

    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Constraint orientation unlock");

    int n = sketch->changeConstraintsLocking(/*bLock=*/false);
    Gui::TranslatedNotification(
        *sketch,
        tr("Constraint orientation locking"),
        tr("Orientation locking was disabled for %1 constraints. The"
           " constraints have been listed in Report view (menu View -> Panels ->"
           " Report view). Note that for all future constraints, the locking still"
           " defaults to ON.")
            .arg(n));

    doc->commitTransaction();
}

void SketcherValidation::onDelConstrExtrClicked()
{
    if (sketch.expired()) {
        return;
    }

    int reply;
    reply = QMessageBox::question(
        this,
        tr("Delete constraints to external geom."),
        tr("You are about to delete ALL constraints that deal with external geometry. This is "
           "useful to rescue a sketch with broken/changed links to external geometry. Are you sure "
           "you want to delete the constraints?"),
        QMessageBox::No | QMessageBox::Yes,
        QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Delete constraints");

    sketch->delConstraintsToExternal();

    doc->commitTransaction();

    Gui::TranslatedNotification(
        *sketch,
        tr("Delete constraints to external geom."),
        tr("All constraints that deal with external geometry were deleted."));
}

void SketcherValidation::showPoints(const std::vector<Base::Vector3d>& pts)
{
    SoCoordinate3* coords = new SoCoordinate3();
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->pointSize = 6;
    SoPointSet* pcPoints = new SoPointSet();

    coincidenceRoot = new SoGroup();

    coincidenceRoot->addChild(drawStyle);
    SoSeparator* pointsep = new SoSeparator();
    SoBaseColor* basecol = new SoBaseColor();
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    pointsep->addChild(basecol);
    pointsep->addChild(coords);
    pointsep->addChild(pcPoints);
    coincidenceRoot->addChild(pointsep);

    // Draw markers
    SoBaseColor* markcol = new SoBaseColor();
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet();
    marker->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex(
        "PLUS",
        App::GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
            ->GetInt("MarkerSize", 9));
    pointsep->addChild(markcol);
    pointsep->addChild(marker);

    int pts_size = (int)pts.size();
    coords->point.setNum(pts_size);
    SbVec3f* c = coords->point.startEditing();
    for (int i = 0; i < pts_size; i++) {
        const Base::Vector3d& v = pts[i];
        c[i].setValue((float)v.x, (float)v.y, (float)v.z);
    }
    coords->point.finishEditing();

    if (!sketch.expired()) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(sketch.get());
        vp->getRoot()->addChild(coincidenceRoot);
    }
}

void SketcherValidation::hidePoints()
{
    if (coincidenceRoot) {
        if (!sketch.expired()) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(sketch.get());
            vp->getRoot()->removeChild(coincidenceRoot);
        }
        coincidenceRoot = nullptr;
    }
}

void SketcherValidation::onFindDegeneratedClicked()
{
    if (sketch.expired()) {
        return;
    }

    double prec = Precision::Confusion();
    int count = sketchAnalyser.detectDegeneratedGeometries(prec);

    if (count == 0) {
        Gui::TranslatedNotification(*sketch,
                                    tr("No degenerated geometry"),
                                    tr("No degenerated geometry found"));

        ui->fixDegenerated->setEnabled(false);
    }
    else {
        Gui::TranslatedUserWarning(*sketch,
                                   tr("Degenerated geometry"),
                                   tr("%1 degenerated geometry found").arg(count));

        ui->fixDegenerated->setEnabled(true);
    }
}

void SketcherValidation::onFixDegeneratedClicked()
{
    if (sketch.expired()) {
        return;
    }

    // undo command open
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Remove degenerated geometry");

    double prec = Precision::Confusion();
    sketchAnalyser.removeDegeneratedGeometries(prec);

    ui->fixButton->setEnabled(false);
    hidePoints();

    // finish the transaction and update
    Gui::WaitCursor wc;
    doc->commitTransaction();
    doc->recompute();
}

// -----------------------------------------------

TaskSketcherValidation::TaskSketcherValidation(Sketcher::SketchObject* Obj)
{
    QWidget* widget = new SketcherValidation(Obj);
    Gui::TaskView::TaskBox* taskbox =
        new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSketcherValidation::~TaskSketcherValidation()
{}

#include "moc_TaskSketcherValidation.cpp"
