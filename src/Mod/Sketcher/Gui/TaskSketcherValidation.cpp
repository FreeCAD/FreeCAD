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

#include <Precision.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp_Explorer.hxx>
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

using namespace SketcherGui;
using namespace Gui::TaskView;

/* TRANSLATOR SketcherGui::SketcherValidation */

SketcherValidation::SketcherValidation(Sketcher::SketchObject* Obj, QWidget* parent)
  : QWidget(parent), ui(new Ui_TaskSketcherValidation()), sketch(Obj), coincidenceRoot(0)
{
    ui->setupUi(this);
    ui->fixButton->setEnabled(false);
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

struct SketcherValidation::VertexIds {
    Base::Vector3d v;
    int GeoId;
    Sketcher::PointPos PosId;
};

struct SketcherValidation::Vertex_Less : public std::binary_function<const VertexIds&,
                                                                     const VertexIds&, bool>
{
    Vertex_Less(double tolerance) : tolerance(tolerance){}
    bool operator()(const VertexIds& x,
                    const VertexIds& y) const
    {
        if (fabs (x.v.x - y.v.x) > tolerance)
            return x.v.x < y.v.x;
        if (fabs (x.v.y - y.v.y) > tolerance)
            return x.v.y < y.v.y;
        if (fabs (x.v.z - y.v.z) > tolerance)
            return x.v.z < y.v.z;
        return false; // points are considered to be equal
    }
private:
    double tolerance;
};

struct SketcherValidation::Vertex_EqualTo : public std::binary_function<const VertexIds&,
                                                                        const VertexIds&, bool>
{
    Vertex_EqualTo(double tolerance) : tolerance(tolerance){}
    bool operator()(const VertexIds& x,
                    const VertexIds& y) const
    {
        if (fabs (x.v.x - y.v.x) <= tolerance) {
            if (fabs (x.v.y - y.v.y) <= tolerance) {
                if (fabs (x.v.z - y.v.z) <= tolerance) {
                    return true;
                }
            }
        }
        return false;
    }
private:
    double tolerance;
};

struct SketcherValidation::ConstraintIds {
    Base::Vector3d v;
    int First;
    int Second;
    Sketcher::PointPos FirstPos;
    Sketcher::PointPos SecondPos;
};

struct SketcherValidation::Constraint_Less  : public std::binary_function<const ConstraintIds&,
                                                                          const ConstraintIds&, bool>
{
    bool operator()(const ConstraintIds& x,
                    const ConstraintIds& y) const
    {
        int x1 = x.First;
        int x2 = x.Second;
        int y1 = y.First;
        int y2 = y.Second;

        if (x1 > x2)
        { std::swap(x1, x2); }
        if (y1 > y2)
        { std::swap(y1, y2); }

        if      (x1 < y1) return true;
        else if (x1 > y1) return false;
        else if (x2 < y2) return true;
        else if (x2 > y2) return false;
        return false;
    }
};

void SketcherValidation::on_findButton_clicked()
{
    std::vector<VertexIds> vertexIds;
    const std::vector<Part::Geometry *>& geom = sketch->getInternalGeometry();
    for (std::size_t i=0; i<geom.size(); i++) {
        Part::Geometry* g = geom[i];
        if (g->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *segm = dynamic_cast<const Part::GeomLineSegment*>(g);
            VertexIds id;
            id.GeoId = (int)i;
            id.PosId = Sketcher::start;
            id.v = segm->getStartPoint();
            vertexIds.push_back(id);
            id.GeoId = (int)i;
            id.PosId = Sketcher::end;
            id.v = segm->getEndPoint();
            vertexIds.push_back(id);
        }
        else if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *segm = dynamic_cast<const Part::GeomArcOfCircle*>(g);
            VertexIds id;
            id.GeoId = (int)i;
            id.PosId = Sketcher::start;
            id.v = segm->getStartPoint();
            vertexIds.push_back(id);
            id.GeoId = (int)i;
            id.PosId = Sketcher::end;
            id.v = segm->getEndPoint();
            vertexIds.push_back(id);
        }
    }

    std::set<ConstraintIds, Constraint_Less> coincidences;
    double prec = Precision::Confusion();
    QVariant v = ui->comboBoxTolerance->itemData(ui->comboBoxTolerance->currentIndex());
    if (v.isValid())
        prec = v.toDouble();
    else
        prec = QLocale::system().toDouble(ui->comboBoxTolerance->currentText());
    std::sort(vertexIds.begin(), vertexIds.end(), Vertex_Less(prec));
    std::vector<VertexIds>::iterator vt = vertexIds.begin();
    Vertex_EqualTo pred(prec);
    while (vt < vertexIds.end()) {
        // get first item whose adjacent element has the same vertex coordinates
        vt = std::adjacent_find(vt, vertexIds.end(), pred);
        if (vt < vertexIds.end()) {
            std::vector<VertexIds>::iterator vn;
            for (vn = vt+1; vn != vertexIds.end(); ++vn) {
                if (pred(*vt,*vn)) {
                    ConstraintIds id;
                    id.v = vt->v;
                    id.First = vt->GeoId;
                    id.FirstPos = vt->PosId;
                    id.Second = vn->GeoId;
                    id.SecondPos = vn->PosId;
                    coincidences.insert(id);
                }
                else {
                    break;
                }
            }

            vt = vn;
        }
    }

    std::vector<Sketcher::Constraint*> constraint = sketch->Constraints.getValues();
    for (std::vector<Sketcher::Constraint*>::iterator it = constraint.begin(); it != constraint.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident) {
            ConstraintIds id;
            id.First = (*it)->First;
            id.FirstPos = (*it)->FirstPos;
            id.Second = (*it)->Second;
            id.SecondPos = (*it)->SecondPos;
            std::set<ConstraintIds, Constraint_Less>::iterator pos = coincidences.find(id);
            if (pos != coincidences.end()) {
                coincidences.erase(pos);
            }
        }
    }

    this->vertexConstraints.clear();
    this->vertexConstraints.reserve(coincidences.size());
    std::vector<Base::Vector3d> points;
    points.reserve(coincidences.size());
    for (std::set<ConstraintIds, Constraint_Less>::iterator it = coincidences.begin(); it != coincidences.end(); ++it) {
        this->vertexConstraints.push_back(*it);
        points.push_back(it->v);
    }

    hidePoints();
    if (this->vertexConstraints.empty()) {
        QMessageBox::information(this, tr("No missing coincidences"),
            tr("No missing coincidences found"));
        ui->fixButton->setEnabled(false);
    }
    else {
        showPoints(points);
        QMessageBox::warning(this, tr("Missing coincidences"),
            tr("%1 missing coincidences found").arg(this->vertexConstraints.size()));
        ui->fixButton->setEnabled(true);
    }
}

void SketcherValidation::on_fixButton_clicked()
{
    // undo command open
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("add coincident constraint");
    std::vector<Sketcher::Constraint*> constr;
    for (std::vector<ConstraintIds>::iterator it = this->vertexConstraints.begin(); it != this->vertexConstraints.end(); ++it) {
        Sketcher::Constraint* c = new Sketcher::Constraint();
        c->Type = Sketcher::Coincident;
        c->First = it->First;
        c->Second = it->Second;
        c->FirstPos = it->FirstPos;
        c->SecondPos = it->SecondPos;
        constr.push_back(c);
    }

    sketch->addConstraints(constr);
    this->vertexConstraints.clear();
    ui->fixButton->setEnabled(false);
    hidePoints();
    for (std::vector<Sketcher::Constraint*>::iterator it = constr.begin(); it != constr.end(); ++it) {
        delete *it;
    }

    // finish the transaction and update
    Gui::WaitCursor wc;
    doc->commitTransaction();
    doc->recompute();
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
    marker->markerIndex=SoMarkerSet::PLUS_9_9;
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
