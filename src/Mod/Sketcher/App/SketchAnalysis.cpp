/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Console.h>
#include <App/Document.h>

#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/Geometry.h>

#include <cmath>

#include "SketchAnalysis.h"

using namespace Sketcher;

SketchAnalysis::SketchAnalysis(Sketcher::SketchObject* Obj)
  : sketch(Obj)
{

}

SketchAnalysis::~SketchAnalysis()
{

}

struct SketchAnalysis::VertexIds {
    Base::Vector3d v;
    int GeoId;
    Sketcher::PointPos PosId;
};

struct SketchAnalysis::Vertex_Less : public std::binary_function<const VertexIds&,
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

struct SketchAnalysis::Vertex_EqualTo : public std::binary_function<const VertexIds&,
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

struct SketchAnalysis::EdgeIds {
    double l;
    int GeoId;
};

struct SketchAnalysis::Edge_Less : public std::binary_function<const EdgeIds&,
const EdgeIds&, bool>
{
    Edge_Less(double tolerance) : tolerance(tolerance){}
    bool operator()(const EdgeIds& x,
                    const EdgeIds& y) const
                    {
                        if (fabs (x.l - y.l) > tolerance)
                            return x.l < y.l;
                        return false; // points are considered to be equal
                    }
private:
    double tolerance;
};

struct SketchAnalysis::Edge_EqualTo : public std::binary_function<const EdgeIds&,
const EdgeIds&, bool>
{
    Edge_EqualTo(double tolerance) : tolerance(tolerance){}
    bool operator()(const EdgeIds& x,
                    const EdgeIds& y) const
                    {
                        if (fabs (x.l - y.l) <= tolerance) {
                            return true;
                        }
                        return false;
                    }
private:
    double tolerance;
};

int SketchAnalysis::detectMissingPointOnPointConstraints(double precision, bool includeconstruction /*=true*/)
{
    std::vector<VertexIds> vertexIds;
    const std::vector<Part::Geometry *>& geom = sketch->getInternalGeometry();
    for (std::size_t i=0; i<geom.size(); i++) {
        Part::Geometry* g = geom[i];

        if(g->Construction && !includeconstruction)
            continue;

        if (g->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *segm = static_cast<const Part::GeomLineSegment*>(g);
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
            const Part::GeomArcOfCircle *segm = static_cast<const Part::GeomArcOfCircle*>(g);
            VertexIds id;
            id.GeoId = (int)i;
            id.PosId = Sketcher::start;
            id.v = segm->getStartPoint(/*emulateCCW=*/true);
            vertexIds.push_back(id);
            id.GeoId = (int)i;
            id.PosId = Sketcher::end;
            id.v = segm->getEndPoint(/*emulateCCW=*/true);
            vertexIds.push_back(id);
        }
        else if (g->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            const Part::GeomArcOfEllipse *segm = static_cast<const Part::GeomArcOfEllipse*>(g);
            VertexIds id;
            id.GeoId = (int)i;
            id.PosId = Sketcher::start;
            id.v = segm->getStartPoint(/*emulateCCW=*/true);
            vertexIds.push_back(id);
            id.GeoId = (int)i;
            id.PosId = Sketcher::end;
            id.v = segm->getEndPoint(/*emulateCCW=*/true);
            vertexIds.push_back(id);
        }
        else if (g->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            const Part::GeomArcOfHyperbola *segm = static_cast<const Part::GeomArcOfHyperbola*>(g);
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
        else if (g->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            const Part::GeomArcOfParabola *segm = static_cast<const Part::GeomArcOfParabola*>(g);
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
        else if (g->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve *segm = static_cast<const Part::GeomBSplineCurve*>(g);
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

    std::sort(vertexIds.begin(), vertexIds.end(), Vertex_Less(precision));
    std::vector<VertexIds>::iterator vt = vertexIds.begin();
    Vertex_EqualTo pred(precision);

    std::list<ConstraintIds> coincidences;
    // Make a list of constraint we expect for coincident vertexes
    while (vt < vertexIds.end()) {
        // get first item whose adjacent element has the same vertex coordinates
        vt = std::adjacent_find(vt, vertexIds.end(), pred);
        if (vt < vertexIds.end()) {
            std::vector<VertexIds>::iterator vn;
            for (vn = vt+1; vn != vertexIds.end(); ++vn) {
                if (pred(*vt,*vn)) {
                    ConstraintIds id;
                    id.Type = Coincident; // default point on point restriction
                    id.v = vt->v;
                    id.First = vt->GeoId;
                    id.FirstPos = vt->PosId;
                    id.Second = vn->GeoId;
                    id.SecondPos = vn->PosId;
                    coincidences.push_back(id);
                }
                else {
                    break;
                }
            }

            vt = vn;
        }
    }

    // Go through the available 'Coincident', 'Tangent' or 'Perpendicular' constraints
    // and check which of them is forcing two vertexes to be coincident.
    // If there is none but two vertexes can be considered equal a coincident constraint is missing.
    std::vector<Sketcher::Constraint*> constraint = sketch->Constraints.getValues();
    for (std::vector<Sketcher::Constraint*>::iterator it = constraint.begin(); it != constraint.end(); ++it) {
        if ((*it)->Type == Sketcher::Coincident ||
            (*it)->Type == Sketcher::Tangent ||
            (*it)->Type == Sketcher::Perpendicular) {
            ConstraintIds id;
            id.First = (*it)->First;
            id.FirstPos = (*it)->FirstPos;
            id.Second = (*it)->Second;
            id.SecondPos = (*it)->SecondPos;
            std::list<ConstraintIds>::iterator pos = std::find_if
                    (coincidences.begin(), coincidences.end(), Constraint_Equal(id));
            if (pos != coincidences.end()) {
                coincidences.erase(pos);
            }
        }
    }

    this->vertexConstraints.clear();
    this->vertexConstraints.reserve(coincidences.size());

    for (std::list<ConstraintIds>::iterator it = coincidences.begin(); it != coincidences.end(); ++it) {
        this->vertexConstraints.push_back(*it);
    }

    return this->vertexConstraints.size();
}

void SketchAnalysis::analyseMissingPointOnPointCoincident(double angleprecision)
{
    for(auto & vc : vertexConstraints) {

        auto geo1 = sketch->getGeometry(vc.First);
        auto geo2 = sketch->getGeometry(vc.Second);

        // tangency point-on-point
        const Part::GeomCurve * curve1 = dynamic_cast<const Part::GeomCurve *>(geo1);
        const Part::GeomCurve * curve2 = dynamic_cast<const Part::GeomCurve *>(geo2);

        if(curve1 && curve2) {

            if( geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {

                const Part::GeomLineSegment *segm1 = static_cast<const Part::GeomLineSegment*>(geo1);
                const Part::GeomLineSegment *segm2 = static_cast<const Part::GeomLineSegment*>(geo2);

                Base::Vector3d dir1 = segm1->getEndPoint() - segm1->getStartPoint();
                Base::Vector3d dir2 = segm2->getEndPoint() - segm2->getStartPoint();

                if( (checkVertical(dir1,angleprecision)  || checkHorizontal(dir1,angleprecision)) &&
                    (checkVertical(dir2,angleprecision)  || checkHorizontal(dir2,angleprecision)) ) {
                    // this is a job for horizontal/vertical constraints alone
                    continue;
                }
            }

            try {
                double u1, u2;

                curve1->closestParameter(vc.v,u1);
                curve2->closestParameter(vc.v,u2);

                Base::Vector3d tgv1 = curve1->firstDerivativeAtParameter(u1).Normalize();
                Base::Vector3d tgv2 = curve2->firstDerivativeAtParameter(u2).Normalize();

                if(fabs(tgv1*tgv2)>fabs(cos(angleprecision))) {
                    vc.Type = Sketcher::Tangent;
                }
                else if(fabs(tgv1*tgv2)<fabs(cos(M_PI/2 - angleprecision))) {
                    vc.Type = Sketcher::Perpendicular;
                }

            }
            catch(Base::Exception &) {
                Base::Console().Warning("Point-On-Point Coincidence analysis: unable to obtain derivative. Detection ignored.\n");
                continue;
            }


        }
    }
}


void SketchAnalysis::makeMissingPointOnPointCoincident(bool onebyone)
{
    int status, dofs;
    std::vector<Sketcher::Constraint*> constr;

    for (std::vector<Sketcher::ConstraintIds>::iterator it = vertexConstraints.begin(); it != vertexConstraints.end(); ++it) {
        Sketcher::Constraint* c = new Sketcher::Constraint();
        c->Type = it->Type;
        c->First = it->First;
        c->Second = it->Second;
        c->FirstPos = it->FirstPos;
        c->SecondPos = it->SecondPos;

        if(onebyone) {
            sketch->addConstraint(c);

            solvesketch(status,dofs,true);

            if(status == -2) { //redundant constraints
                sketch->autoRemoveRedundants(false);

                solvesketch(status,dofs,false);
            }

            if(status) {
                THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch while applying coincident constraints."));
            }
        }
        else {
            constr.push_back(c);
        }
    }

    if(!onebyone)
        sketch->addConstraints(constr);

    vertexConstraints.clear();

    for (std::vector<Sketcher::Constraint*>::iterator it = constr.begin(); it != constr.end(); ++it) {
        delete *it;
    }
}

int SketchAnalysis::detectMissingVerticalHorizontalConstraints(double angleprecision)
{
    const std::vector<Part::Geometry *>& geom = sketch->getInternalGeometry();

    verthorizConstraints.clear();

    for (std::size_t i=0; i<geom.size(); i++) {
        Part::Geometry* g = geom[i];

        if (g->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *segm = static_cast<const Part::GeomLineSegment*>(g);

            Base::Vector3d dir = segm->getEndPoint() - segm->getStartPoint();

            ConstraintIds id;

            id.v = dir;
            id.First = (int)i;
            id.FirstPos = Sketcher::none;
            id.Second = Constraint::GeoUndef;
            id.SecondPos = Sketcher::none;

            if( checkVertical(dir, angleprecision) ) {
                id.Type = Sketcher::Vertical;
                verthorizConstraints.push_back(id);
            }
            else if (checkHorizontal(dir, angleprecision)  ) {
                id.Type = Sketcher::Horizontal;
                verthorizConstraints.push_back(id);
            }
        }
    }

    return verthorizConstraints.size();
}

void SketchAnalysis::makeMissingVerticalHorizontal(bool onebyone)
{
    int status, dofs;
    std::vector<Sketcher::Constraint*> constr;

    for (std::vector<Sketcher::ConstraintIds>::iterator it = verthorizConstraints.begin(); it != verthorizConstraints.end(); ++it) {
        Sketcher::Constraint* c = new Sketcher::Constraint();
        c->Type = it->Type;
        c->First = it->First;
        c->Second = it->Second;
        c->FirstPos = it->FirstPos;
        c->SecondPos = it->SecondPos;

        if(onebyone) {
            sketch->addConstraint(c);

            solvesketch(status,dofs,true);

            if(status == -2) { //redundant constraints
                sketch->autoRemoveRedundants(false);

                solvesketch(status,dofs,false);
            }

            if(status) {
                THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch while applying vertical/horizontal constraints."));
            }
        }
        else {
            constr.push_back(c);
        }
    }

    if(!onebyone)
        sketch->addConstraints(constr);

    verthorizConstraints.clear();

    for (std::vector<Sketcher::Constraint*>::iterator it = constr.begin(); it != constr.end(); ++it) {
        delete *it;
    }
}

bool SketchAnalysis::checkVertical(Base::Vector3d dir, double angleprecision)
{
    return (dir.x == 0. && dir.y != 0.) || ( fabs(dir.y/dir.x) > tan(M_PI/2 - angleprecision));
}

bool SketchAnalysis::checkHorizontal(Base::Vector3d dir, double angleprecision)
{
    return (dir.y == 0. && dir.x != 0.) || ( fabs(dir.x/dir.y) > (1/tan(angleprecision)));
}

int SketchAnalysis::detectMissingEqualityConstraints(double precision)
{
    std::vector<EdgeIds> lineedgeIds;
    std::vector<EdgeIds> radiusedgeIds;

    const std::vector<Part::Geometry *>& geom = sketch->getInternalGeometry();
    for (std::size_t i=0; i<geom.size(); i++) {
        Part::Geometry* g = geom[i];

        if (g->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *segm = static_cast<const Part::GeomLineSegment*>(g);
            EdgeIds id;
            id.GeoId = (int)i;
            id.l = (segm->getEndPoint()-segm->getStartPoint()).Length();
            lineedgeIds.push_back(id);
        }
        else if (g->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *segm = static_cast<const Part::GeomArcOfCircle*>(g);
            EdgeIds id;
            id.GeoId = (int)i;
            id.l = segm->getRadius();
            radiusedgeIds.push_back(id);
        }
        else if (g->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *segm = static_cast<const Part::GeomCircle*>(g);
            EdgeIds id;
            id.GeoId = (int)i;
            id.l = segm->getRadius();
            radiusedgeIds.push_back(id);
        }
    }

    std::sort(lineedgeIds.begin(), lineedgeIds.end(), Edge_Less(precision));
    std::vector<EdgeIds>::iterator vt = lineedgeIds.begin();
    Edge_EqualTo pred(precision);

    std::list<ConstraintIds> equallines;
    // Make a list of constraint we expect for coincident vertexes
    while (vt < lineedgeIds.end()) {
        // get first item whose adjacent element has the same vertex coordinates
        vt = std::adjacent_find(vt, lineedgeIds.end(), pred);
        if (vt < lineedgeIds.end()) {
            std::vector<EdgeIds>::iterator vn;
            for (vn = vt+1; vn != lineedgeIds.end(); ++vn) {
                if (pred(*vt,*vn)) {
                    ConstraintIds id;
                    id.Type = Equal;
                    id.v.x = vt->l;
                    id.First = vt->GeoId;
                    id.FirstPos = Sketcher::none;
                    id.Second = vn->GeoId;
                    id.SecondPos = Sketcher::none;
                    equallines.push_back(id);
                }
                else {
                    break;
                }
            }

            vt = vn;
        }
    }

    std::sort(radiusedgeIds.begin(), radiusedgeIds.end(), Edge_Less(precision));
    vt = radiusedgeIds.begin();

    std::list<ConstraintIds> equalradius;
    // Make a list of constraint we expect for coincident vertexes
    while (vt < radiusedgeIds.end()) {
        // get first item whose adjacent element has the same vertex coordinates
        vt = std::adjacent_find(vt, radiusedgeIds.end(), pred);
        if (vt < radiusedgeIds.end()) {
            std::vector<EdgeIds>::iterator vn;
            for (vn = vt+1; vn != radiusedgeIds.end(); ++vn) {
                if (pred(*vt,*vn)) {
                    ConstraintIds id;
                    id.Type = Equal;
                    id.v.x = vt->l;
                    id.First = vt->GeoId;
                    id.FirstPos = Sketcher::none;
                    id.Second = vn->GeoId;
                    id.SecondPos = Sketcher::none;
                    equalradius.push_back(id);
                }
                else {
                    break;
                }
            }

            vt = vn;
        }
    }


    // Go through the available 'Coincident', 'Tangent' or 'Perpendicular' constraints
    // and check which of them is forcing two vertexes to be coincident.
    // If there is none but two vertexes can be considered equal a coincident constraint is missing.
    std::vector<Sketcher::Constraint*> constraint = sketch->Constraints.getValues();
    for (std::vector<Sketcher::Constraint*>::iterator it = constraint.begin(); it != constraint.end(); ++it) {
        if ((*it)->Type == Sketcher::Equal) {
            ConstraintIds id;
            id.First = (*it)->First;
            id.FirstPos = (*it)->FirstPos;
            id.Second = (*it)->Second;
            id.SecondPos = (*it)->SecondPos;

            std::list<ConstraintIds>::iterator pos = std::find_if
                (equallines.begin(), equallines.end(), Constraint_Equal(id));

            if (pos != equallines.end()) {
                equallines.erase(pos);
            }

            pos = std::find_if
                (equalradius.begin(), equalradius.end(), Constraint_Equal(id));

            if (pos != equalradius.end()) {
                equalradius.erase(pos);
            }
        }
    }

    this->lineequalityConstraints.clear();
    this->lineequalityConstraints.reserve(equallines.size());

    for (std::list<ConstraintIds>::iterator it = equallines.begin(); it != equallines.end(); ++it) {
        this->lineequalityConstraints.push_back(*it);
    }

    this->radiusequalityConstraints.clear();
    this->radiusequalityConstraints.reserve(equalradius.size());

    for (std::list<ConstraintIds>::iterator it = equalradius.begin(); it != equalradius.end(); ++it) {
        this->radiusequalityConstraints.push_back(*it);
    }

    return this->lineequalityConstraints.size() + this->radiusequalityConstraints.size();
}

void SketchAnalysis::makeMissingEquality(bool onebyone)
{
    int status, dofs;
    std::vector<Sketcher::Constraint*> constr;

    std::vector<Sketcher::ConstraintIds> equalities(lineequalityConstraints);
    equalities.insert(equalities.end(),radiusequalityConstraints.begin(), radiusequalityConstraints.end());

    for (std::vector<Sketcher::ConstraintIds>::iterator it = equalities.begin(); it != equalities.end(); ++it) {
        Sketcher::Constraint* c = new Sketcher::Constraint();
        c->Type = it->Type;
        c->First = it->First;
        c->Second = it->Second;
        c->FirstPos = it->FirstPos;
        c->SecondPos = it->SecondPos;

        if(onebyone) {
            sketch->addConstraint(c);

            solvesketch(status,dofs,true);

            if(status == -2) { //redundant constraints
                sketch->autoRemoveRedundants(false);

                solvesketch(status,dofs,false);
            }

            if(status) {
                THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch while applying equality constraints."));
            }
        }
        else {
            constr.push_back(c);
        }
    }

    if(!onebyone)
        sketch->addConstraints(constr);

    lineequalityConstraints.clear();
    radiusequalityConstraints.clear();

    for (std::vector<Sketcher::Constraint*>::iterator it = constr.begin(); it != constr.end(); ++it) {
        delete *it;
    }
}

void SketchAnalysis::solvesketch(int &status, int &dofs, bool updategeo)
{
    status = sketch->solve(updategeo);

    if(updategeo)
        dofs = sketch->setUpSketch();
    else
        dofs = sketch->getLastDoF();

    if (sketch->getLastHasRedundancies()) { // redundant constraints
        status = -2;
    }

    if (dofs < 0) { // over-constrained sketch
        status = -4;
    }
    else if (sketch->getLastHasConflicts()) { // conflicting constraints
        status = -3;
    }
}

int SketchAnalysis::autoconstraint(double precision, double angleprecision, bool includeconstruction)
{
    App::Document* doc = sketch->getDocument();
    doc->openTransaction("delete all constraints");
    // We start from zero
    sketch->deleteAllConstraints();

    doc->commitTransaction();

    int status, dofs;

    solvesketch(status,dofs,true);

    if(status) {// it should not be possible at this moment as we start from a clean situation
        THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch without constraints."));
    }

    // STAGE 1: Vertical/Horizontal Line Segments
    int nhv = detectMissingVerticalHorizontalConstraints(angleprecision);

    // STAGE 2: Point-on-Point constraint (Coincidents, endpoint perp, endpoint tangency)
    // Note: We do not apply the vertical/horizontal constraints before calculating the pointonpoint constraints
    //       as the solver may move the geometry in the meantime and prevent correct detection
    int nc = detectMissingPointOnPointConstraints(precision, includeconstruction);

    if (nc > 0) // STAGE 2a: Classify point-on-point into coincidents, endpoint perp, endpoint tangency
        analyseMissingPointOnPointCoincident(angleprecision);

    // STAGE 3: Equality constraint detection
    int ne = detectMissingEqualityConstraints(precision);

    Base::Console().Log("Constraints: Vertical/Horizontal: %d found. Point-on-point: %d. Equality: %d\n", nhv, nc, ne);

    // Applying STAGE 1, if any
    if (nhv >0 ) {
        App::Document* doc = sketch->getDocument();
        doc->openTransaction("add vertical/horizontal constraints");

        makeMissingVerticalHorizontal();

        // finish the transaction and update
        doc->commitTransaction();

        solvesketch(status,dofs,true);

        if(status == -2) { // redundants
            sketch->autoRemoveRedundants(false);
            solvesketch(status,dofs,false);
        }

        if(status) {
            THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch after applying horizontal and vertical constraints."));
        }
    }

    // Applying STAGE 2
    if(nc > 0) {
        App::Document* doc = sketch->getDocument();
        doc->openTransaction("add coincident constraint");

        makeMissingPointOnPointCoincident();

        // finish the transaction and update
        doc->commitTransaction();

        solvesketch(status,dofs,true);

        if(status == -2) { // redundants
            sketch->autoRemoveRedundants(false);
            solvesketch(status,dofs,false);
        }

        if(status) {
            THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch after applying point-on-point constraints."));
        }
    }

    // Applying STAGE 3
    if(ne > 0) {
        App::Document* doc = sketch->getDocument();
        doc->openTransaction("add equality constraints");

        try {
            makeMissingEquality();
        }
        catch(Base::RuntimeError &) {
            doc->abortTransaction();
            throw;
        }

        // finish the transaction and update
        doc->commitTransaction();

        solvesketch(status,dofs,true);

        if(status == -2) { // redundants
            sketch->autoRemoveRedundants(false);
            solvesketch(status,dofs,false);
        }

        if(status) {
            THROWMT(Base::RuntimeError, QT_TRANSLATE_NOOP("Exceptions", "Autoconstrain error: Unsolvable sketch after applying equality constraints."));
        }
    }


    return 0;
}


std::vector<Base::Vector3d> SketchAnalysis::getOpenVertices(void) const
{
    std::vector<Base::Vector3d> points;
    TopoDS_Shape shape = sketch->Shape.getValue();

    Base::Placement Plm = sketch->Placement.getValue();

    Base::Placement invPlm = Plm.inverse();

    // build up map vertex->edge
    TopTools_IndexedDataMapOfShapeListOfShape vertex2Edge;
    TopExp::MapShapesAndAncestors(shape, TopAbs_VERTEX, TopAbs_EDGE, vertex2Edge);
    for (int i=1; i<= vertex2Edge.Extent(); ++i) {
        const TopTools_ListOfShape& los = vertex2Edge.FindFromIndex(i);
        if (los.Extent() != 2) {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(vertex2Edge.FindKey(i));
            gp_Pnt pnt = BRep_Tool::Pnt(vertex);
            Base::Vector3d pos;
            invPlm.multVec(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()),pos);
            points.push_back(pos);
        }
    }

    return points;
}
