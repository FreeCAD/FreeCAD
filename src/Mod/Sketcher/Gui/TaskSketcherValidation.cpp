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
# include <Standard_math.hxx>
# include <QDoubleValidator>
# include <QLocale>
# include <QMessageBox>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
#endif

#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <algorithm>

#include "ui_TaskSketcherValidation.h"
#include "TaskSketcherValidation.h"
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/Geometry.h>
#include <App/Document.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Inventor/MarkerBitmaps.h>

using namespace SketcherGui;
using namespace Gui::TaskView;

/* TRANSLATOR SketcherGui::SketcherValidation */

SketcherValidation::SketcherValidation(Sketcher::SketchObject* Obj, QWidget* parent)
: QWidget(parent), ui(new Ui_TaskSketcherValidation()), sketch(Obj), sketchAnalyser(Obj), coincidenceRoot(0)
{
    ui->setupUi(this);
    ui->fixButton->setEnabled(false);
    ui->fixConstraint->setEnabled(false);
    ui->swapReversed->setEnabled(false);
    ui->checkBoxIgnoreConstruction->setEnabled(true);
    double tolerances[8] = {
        Precision::Confusion() / 100,
        Precision::Confusion() / 10,
        Precision::Confusion(),
        Precision::Confusion() * 10,
        Precision::Confusion() * 100,
        Precision::Confusion() * 1000,
        Precision::Confusion() * 10000,
        Precision::Confusion() * 100000
    };

    for (int i=0; i<8; i++) {
        ui->comboBoxTolerance->addItem(QLocale::system().toString(tolerances[i]), QVariant(tolerances[i]));
    }
    ui->comboBoxTolerance->setCurrentIndex(5);
    ui->comboBoxTolerance->setEditable(true);
    ui->comboBoxTolerance->setValidator(new QDoubleValidator(0,10,10,this));

}

SketcherValidation::~SketcherValidation()
{
    hidePoints();
}

void SketcherValidation::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(e);
}

void SketcherValidation::on_findButton_clicked()
{
    double prec = Precision::Confusion();
    bool ok;
    double conv;

    conv = QLocale::system().toDouble(ui->comboBoxTolerance->currentText(),&ok);

    if(ok) {
        prec = conv;
    }
    else {
        QVariant v = ui->comboBoxTolerance->itemData(ui->comboBoxTolerance->currentIndex());
        if (v.isValid()) {
            prec = v.toDouble();
        }
    }

    sketchAnalyser.detectMissingPointOnPointConstraints(prec,!ui->checkBoxIgnoreConstruction->isChecked());

    std::vector<Sketcher::ConstraintIds> & vertexConstraints = sketchAnalyser.getMissingPointOnPointConstraints();

    std::vector<Base::Vector3d> points;
    points.reserve(vertexConstraints.size());

    for (auto vc : vertexConstraints) {
        points.push_back(vc.v);
    }

    hidePoints();
    if (vertexConstraints.empty()) {
        QMessageBox::information(this, tr("No missing coincidences"),
            tr("No missing coincidences found"));
        ui->fixButton->setEnabled(false);
    }
    else {
        showPoints(points);
        QMessageBox::warning(this, tr("Missing coincidences"),
            tr("%1 missing coincidences found").arg(vertexConstraints.size()));
        ui->fixButton->setEnabled(true);
    }
}

void SketcherValidation::on_fixButton_clicked()
{
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

void SketcherValidation::on_highlightButton_clicked()
{
    std::vector<Base::Vector3d> points;

    points = sketchAnalyser.getOpenVertices();

    hidePoints();
    if (!points.empty())
        showPoints(points);
}

void SketcherValidation::on_findConstraint_clicked()
{
    if (sketch->evaluateConstraints()) {
        QMessageBox::information(this, tr("No invalid constraints"),
            tr("No invalid constraints found"));
        ui->fixConstraint->setEnabled(false);
    }
    else {
        QMessageBox::warning(this, tr("Invalid constraints"),
            tr("Invalid constraints found"));
        ui->fixConstraint->setEnabled(true);
    }
}

void SketcherValidation::on_fixConstraint_clicked()
{
    sketch->validateConstraints();
    ui->fixConstraint->setEnabled(false);
}

void SketcherValidation::on_findReversed_clicked()
{
    std::vector<Base::Vector3d> points;
    const std::vector<Part::Geometry *>& geom = sketch->getExternalGeometry();
    for (std::size_t i=0; i<geom.size(); i++) {
        Part::Geometry* g = geom[i];
        //only arcs of circles need to be repaired. Arcs of ellipse were so broken there should be nothing to repair from.
        if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *segm = static_cast<const Part::GeomArcOfCircle*>(g);
            if (segm->isReversed()) {
                points.push_back(segm->getStartPoint(/*emulateCCW=*/true));
                points.push_back(segm->getEndPoint(/*emulateCCW=*/true));
            }
        }
    }
    hidePoints();
    if(points.size()>0){
        int nc = sketch->port_reversedExternalArcs(/*justAnalyze=*/true);
        showPoints(points);
        if(nc>0){
            QMessageBox::warning(this, tr("Reversed external geometry"),
                tr("%1 reversed external-geometry arcs were found. Their endpoints are"
                   " encircled in 3d view.\n\n"
                   "%2 constraints are linking to the endpoints. The constraints have"
                   " been listed in Report view (menu View -> Panels -> Report view).\n\n"
                   "Click \"Swap endpoints in constraints\" button to reassign endpoints."
                   " Do this only once to sketches created in FreeCAD older than v0.15"
                   ).arg(points.size()/2).arg(nc)
                                 );
            ui->swapReversed->setEnabled(true);
        } else {
            QMessageBox::warning(this, tr("Reversed external geometry"),
                tr("%1 reversed external-geometry arcs were found. Their endpoints are "
                   "encircled in 3d view.\n\n"
                   "However, no constraints linking to the endpoints were found.").arg(points.size()/2));
            ui->swapReversed->setEnabled(false);
        }
    } else {
        QMessageBox::warning(this, tr("Reversed external geometry"),
            tr("No reversed external-geometry arcs were found."));
    }
}

void SketcherValidation::on_swapReversed_clicked()
{
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Sketch porting");

    int n = sketch->port_reversedExternalArcs(/*justAnalyze=*/false);
    QMessageBox::warning(this, tr("Reversed external geometry"),
        tr("%1 changes were made to constraints linking to endpoints of reversed arcs.").arg(n));
    hidePoints();
    ui->swapReversed->setEnabled(false);

    doc->commitTransaction();
}

void SketcherValidation::on_orientLockEnable_clicked()
{
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Constraint orientation lock");

    int n = sketch->changeConstraintsLocking(/*bLock=*/true);
    QMessageBox::warning(this, tr("Constraint orientation locking"),
        tr("Orientation locking was enabled and recomputed for %1 constraints. The"
           " constraints have been listed in Report view (menu View -> Panels ->"
           " Report view).").arg(n));

    doc->commitTransaction();
}

void SketcherValidation::on_orientLockDisable_clicked()
{
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Constraint orientation unlock");

    int n = sketch->changeConstraintsLocking(/*bLock=*/false);
    QMessageBox::warning(this, tr("Constraint orientation locking"),
        tr("Orientation locking was disabled for %1 constraints. The"
           " constraints have been listed in Report view (menu View -> Panels ->"
           " Report view). Note that for all future constraints, the locking still"
           " defaults to ON.").arg(n));

    doc->commitTransaction();
}

void SketcherValidation::on_delConstrExtr_clicked()
{
    int reply;
    reply =  QMessageBox::question(this,
                        tr("Delete constraints to external geom."),
                        tr("You are about to delete ALL constraints that deal with external geometry. This is useful to rescue a sketch with broken/changed links to external geometry. Are you sure you want to delete the constraints?"),
                        QMessageBox::No|QMessageBox::Yes,QMessageBox::No);
    if(reply!=QMessageBox::Yes) return;

    App::Document* doc = sketch->getDocument();
    doc->openTransaction("Delete constraints");

    sketch->delConstraintsToExternal();

    doc->commitTransaction();

    QMessageBox::warning(this, tr("Delete constraints to external geom."),
                         tr("All constraints that deal with external geometry were deleted."));
}

void SketcherValidation::showPoints(const std::vector<Base::Vector3d>& pts)
{
    SoCoordinate3 * coords = new SoCoordinate3();
    SoDrawStyle   * drawStyle = new SoDrawStyle();
    drawStyle->pointSize = 6;
    SoPointSet* pcPoints = new SoPointSet();

    coincidenceRoot = new SoGroup();

    coincidenceRoot->addChild(drawStyle);
    SoSeparator* pointsep = new SoSeparator();
    SoBaseColor * basecol = new SoBaseColor();
    basecol->rgb.setValue(1.0f, 0.5f, 0.0f);
    pointsep->addChild(basecol);
    pointsep->addChild(coords);
    pointsep->addChild(pcPoints);
    coincidenceRoot->addChild(pointsep);

    // Draw markers
    SoBaseColor * markcol = new SoBaseColor();
    markcol->rgb.setValue(1.0f, 1.0f, 0.0f);
    SoMarkerSet* marker = new SoMarkerSet();
    marker->markerIndex=Gui::Inventor::MarkerBitmaps::getMarkerIndex("PLUS", App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")->GetInt("MarkerSize", 9));
    pointsep->addChild(markcol);
    pointsep->addChild(marker);

    int pts_size = (int)pts.size();
    coords->point.setNum(pts_size);
    SbVec3f* c = coords->point.startEditing();
    for (int i = 0; i < pts_size; i++) {
        const Base::Vector3d& v = pts[i];
        c[i].setValue((float)v.x,(float)v.y,(float)v.z);
    }
    coords->point.finishEditing();

    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(sketch);
    vp->getRoot()->addChild(coincidenceRoot);
}

void SketcherValidation::hidePoints()
{
    if (coincidenceRoot) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(sketch);
        vp->getRoot()->removeChild(coincidenceRoot);
        coincidenceRoot = 0;
    }
}

// -----------------------------------------------

TaskSketcherValidation::TaskSketcherValidation(Sketcher::SketchObject* Obj)
{
    QWidget* widget = new SketcherValidation(Obj);
    Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
        QPixmap(), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskSketcherValidation::~TaskSketcherValidation()
{
}

#include "moc_TaskSketcherValidation.cpp"
