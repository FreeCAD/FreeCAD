/***************************************************************************
 *   Copyright (c) 2011 Konstantinos Poulios <logari81@gmail.com>          *
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

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif

#include <algorithm>
#define DEBUG_DERIVS 0
#if DEBUG_DERIVS
#include <cassert>
#endif
#include <cmath>

#include <boost/graph/graph_concepts.hpp>

#include "Constraints.h"


namespace GCS
{

///////////////////////////////////////
// Constraints
///////////////////////////////////////

Constraint::Constraint()
    : origpvec(0)
    , pvec(0)
    , scale(1.)
    , tag(0)
    , pvecChangedFlag(true)
    , driving(true)
    , internalAlignment(Alignment::NoInternalAlignment)
{}

void Constraint::redirectParams(const MAP_pD_pD& redirectionmap)
{
    int i = 0;
    for (VEC_pD::iterator param = origpvec.begin(); param != origpvec.end(); ++param, i++) {
        MAP_pD_pD::const_iterator it = redirectionmap.find(*param);
        if (it != redirectionmap.end()) {
            pvec[i] = it->second;
        }
    }
    pvecChangedFlag = true;
}

void Constraint::revertParams()
{
    pvec = origpvec;
    pvecChangedFlag = true;
}

ConstraintType Constraint::getTypeId()
{
    return None;
}

void Constraint::rescale(double coef)
{
    scale = coef * 1.0;
}

double Constraint::error()
{
    return 0.0;
}

double Constraint::grad(double* /*param*/)
{
    return 0.0;
}

double Constraint::maxStep(MAP_pD_D& /*dir*/, double lim)
{
    return lim;
}

int Constraint::findParamInPvec(double* param)
{
    int ret = -1;
    for (std::size_t i = 0; i < pvec.size(); i++) {
        if (param == pvec[i]) {
            ret = static_cast<int>(i);
            break;
        }
    }
    return ret;
}


// --------------------------------------------------------
// Equal
ConstraintEqual::ConstraintEqual(double* p1, double* p2, double p1p2ratio)
{
    ratio = p1p2ratio;
    pvec.push_back(p1);
    pvec.push_back(p2);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintEqual::getTypeId()
{
    return Equal;
}

void ConstraintEqual::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintEqual::error()
{
    return scale * (*param1() - ratio * (*param2()));
}

double ConstraintEqual::grad(double* param)
{
    double deriv = 0.;
    if (param == param1()) {
        deriv += 1;
    }
    if (param == param2()) {
        deriv += -1;
    }
    return scale * deriv;
}


// --------------------------------------------------------
// Weighted Linear Combination
ConstraintWeightedLinearCombination::ConstraintWeightedLinearCombination(
    size_t givennumpoles,
    const std::vector<double*>& givenpvec,
    const std::vector<double>& givenfactors)
    : factors(givenfactors)
    , numpoles(givennumpoles)
{
    pvec = givenpvec;
    assert(pvec.size() == 2 * numpoles + 1);
    assert(factors.size() == numpoles);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintWeightedLinearCombination::getTypeId()
{
    return WeightedLinearCombination;
}

void ConstraintWeightedLinearCombination::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintWeightedLinearCombination::error()
{
    // Explanation of the math here:
    // https://forum.freecad.org/viewtopic.php?f=9&t=71130&start=120#p635538

    double sum = 0;
    double wsum = 0;

    for (size_t i = 0; i < numpoles; ++i) {
        double wcontrib = *weightat(i) * factors[i];
        wsum += wcontrib;
        sum += *poleat(i) * wcontrib;
    }

    return scale * ((*thepoint()) * wsum - sum);
}

double ConstraintWeightedLinearCombination::grad(double* param)
{
    // Equations are from here:
    // https://forum.freecad.org/viewtopic.php?f=9&t=71130&start=120#p635538

    double deriv = 0.;

    if (param == thepoint()) {
        // Eq. (11)
        double wsum = 0;
        for (size_t i = 0; i < numpoles; ++i) {
            wsum += *weightat(i) * factors[i];
        }
        deriv = wsum;
        return scale * deriv;
    }

    for (size_t i = 0; i < numpoles; ++i) {
        if (param == poleat(i)) {
            // Eq. (12)
            deriv = -(*weightat(i) * factors[i]);
            return scale * deriv;
        }
        if (param == weightat(i)) {
            // Eq. (13)
            deriv = (*thepoint() - *poleat(i)) * factors[i];
            return scale * deriv;
        }
    }

    return scale * deriv;
}


// --------------------------------------------------------
// Center of Gravity
ConstraintCenterOfGravity::ConstraintCenterOfGravity(const std::vector<double*>& givenpvec,
                                                     const std::vector<double>& givenweights)
    : weights(givenweights)
{
    pvec = givenpvec;
    numpoints = pvec.size() - 1;
    assert(pvec.size() > 1);
    assert(weights.size() == numpoints);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintCenterOfGravity::getTypeId()
{
    return CenterOfGravity;
}

void ConstraintCenterOfGravity::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintCenterOfGravity::error()
{
    double sum = 0;
    for (size_t i = 0; i < numpoints; ++i) {
        sum += *pointat(i) * weights[i];
    }

    return scale * (*thecenter() - sum);
}

double ConstraintCenterOfGravity::grad(double* param)
{
    double deriv = 0.;
    if (param == thecenter()) {
        deriv = 1;
    }

    for (size_t i = 0; i < numpoints; ++i) {
        if (param == pointat(i)) {
            deriv = -weights[i];
        }
    }

    return scale * deriv;
}


// --------------------------------------------------------
// Slope at B-spline knot
ConstraintSlopeAtBSplineKnot::ConstraintSlopeAtBSplineKnot(BSpline& b, Line& l, size_t knotindex)
{
    // set up pvec: pole x-coords, pole y-coords, pole weights,
    // line point 1 coords, line point 2 coords

    numpoles = b.degree - b.mult[knotindex] + 1;
    // slope at knot doesn't make sense if there's only C0 continuity
    assert(numpoles >= 2);

    pvec.reserve(3 * numpoles + 4);

    // `startpole` is the first pole affecting the knot with `knotindex`
    size_t startpole = 0;
    // See `System::addConstraintInternalAlignmentKnotPoint()` for some elaboration
    for (size_t j = 1; j <= knotindex; ++j) {
        startpole += b.mult[j];
    }
    if (!b.periodic && startpole >= b.poles.size()) {
        startpole = b.poles.size() - 1;
    }

    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.poles[(startpole + i) % b.poles.size()].x);
    }
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.poles[(startpole + i) % b.poles.size()].y);
    }
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.weights[(startpole + i) % b.weights.size()]);
    }
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);

    // Set up factors to get slope at knot point
    std::vector<double> tempfactors((numpoles + 1), 1.0 / (numpoles + 1));
    factors.resize(numpoles);
    slopefactors.resize(numpoles);
    for (size_t i = 0; i < numpoles + 1; ++i) {
        tempfactors[i] = b.getLinCombFactor(*(b.knots[knotindex]),
                                            startpole + b.degree,
                                            startpole + i,
                                            b.degree - 1)
            / (b.flattenedknots[startpole + b.degree + i] - b.flattenedknots[startpole + i]);
    }
    for (size_t i = 0; i < numpoles; ++i) {
        factors[i] = b.getLinCombFactor(*(b.knots[knotindex]), startpole + b.degree, startpole + i);
        slopefactors[i] = b.degree * (tempfactors[i] - tempfactors[i + 1]);
    }

    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintSlopeAtBSplineKnot::getTypeId()
{
    return SlopeAtBSplineKnot;
}

void ConstraintSlopeAtBSplineKnot::rescale(double coef)
{
    double slopex = 0., slopey = 0.;

    for (size_t i = 0; i < numpoles; ++i) {
        slopex += *polexat(i) * slopefactors[i];
        slopey += *poleyat(i) * slopefactors[i];
    }

    scale = coef / sqrt((slopex * slopex + slopey * slopey));
}

double ConstraintSlopeAtBSplineKnot::error()
{
    double xsum = 0., xslopesum = 0.;
    double ysum = 0., yslopesum = 0.;
    double wsum = 0., wslopesum = 0.;

    for (size_t i = 0; i < numpoles; ++i) {
        double wcontrib = *weightat(i) * factors[i];
        double wslopecontrib = *weightat(i) * slopefactors[i];
        wsum += wcontrib;
        xsum += *polexat(i) * wcontrib;
        ysum += *poleyat(i) * wcontrib;
        wslopesum += wslopecontrib;
        xslopesum += *polexat(i) * wslopecontrib;
        yslopesum += *poleyat(i) * wslopecontrib;
    }

    // This is actually wsum^2 * the respective slopes
    // See Eq (19) from:
    // https://forum.freecad.org/viewtopic.php?f=9&t=71130&start=120#p635538
    double slopex = wsum * xslopesum - wslopesum * xsum;
    double slopey = wsum * yslopesum - wslopesum * ysum;

    // Normalizing it ensures that the cross product is not zero just because
    // one vector is zero.
    double linex = *linep2x() - *linep1x();
    double liney = *linep2y() - *linep1y();
    double dirx = linex / sqrt(linex * linex + liney * liney);
    double diry = liney / sqrt(linex * linex + liney * liney);

    // error is the cross product
    return scale * (slopex * diry - slopey * dirx);
}

double ConstraintSlopeAtBSplineKnot::grad(double* param)
{
    // Equations are from here:
    // https://forum.freecad.org/viewtopic.php?f=9&t=71130&start=120#p635538
    double result = 0.0;
    double linex = *linep2x() - *linep1x();
    double liney = *linep2y() - *linep1y();
    double dirx = linex / sqrt(linex * linex + liney * liney);
    double diry = liney / sqrt(linex * linex + liney * liney);

    for (size_t i = 0; i < numpoles; ++i) {
        if (param == polexat(i)) {
            // Eq. (21)
            double wsum = 0., wslopesum = 0.;
            for (size_t j = 0; j < numpoles; ++j) {
                double wcontrib = *weightat(j) * factors[j];
                double wslopecontrib = *weightat(j) * slopefactors[j];
                wsum += wcontrib;
                wslopesum += wslopecontrib;
            }
            result = (wsum * slopefactors[i] - wslopesum * factors[i]) * diry;
            return scale * result;
        }
        if (param == poleyat(i)) {
            // Eq. (21)
            double wsum = 0., wslopesum = 0.;
            for (size_t i = 0; i < numpoles; ++i) {
                double wcontrib = *weightat(i) * factors[i];
                double wslopecontrib = *weightat(i) * slopefactors[i];
                wsum += wcontrib;
                wslopesum += wslopecontrib;
            }
            result = -(wsum * slopefactors[i] - wslopesum * factors[i]) * dirx;
            return scale * result;
        }
        if (param == weightat(i)) {
            // Eq. (22)
            double xsum = 0., xslopesum = 0.;
            double ysum = 0., yslopesum = 0.;
            for (size_t j = 0; j < numpoles; ++j) {
                double wcontrib = *weightat(j) * factors[j];
                double wslopecontrib = *weightat(j) * slopefactors[j];
                xsum += wcontrib * (*polexat(j) - *polexat(i));
                xslopesum += wslopecontrib * (*polexat(j) - *polexat(i));
                ysum += wcontrib * (*poleyat(j) - *poleyat(i));
                yslopesum += wslopecontrib * (*poleyat(j) - *poleyat(i));
            }
            result = (factors[i] * xslopesum - slopefactors[i] * xsum) * diry
                - (factors[i] * yslopesum - slopefactors[i] * ysum) * dirx;
            return scale * result;
        }
    }

    double slopex = 0., slopey = 0.;

    auto getSlopes = [&]() {
        double xsum = 0., xslopesum = 0.;
        double ysum = 0., yslopesum = 0.;
        double wsum = 0., wslopesum = 0.;

        for (size_t i = 0; i < numpoles; ++i) {
            double wcontrib = *weightat(i) * factors[i];
            double wslopecontrib = *weightat(i) * slopefactors[i];
            wsum += wcontrib;
            xsum += *polexat(i) * wcontrib;
            ysum += *poleyat(i) * wcontrib;
            wslopesum += wslopecontrib;
            xslopesum += *polexat(i) * wslopecontrib;
            yslopesum += *poleyat(i) * wslopecontrib;
        }

        // This is actually wsum^2 * the respective slopes
        slopex = wsum * xslopesum - wslopesum * xsum;
        slopey = wsum * yslopesum - wslopesum * ysum;
    };

    if (param == linep1x()) {
        getSlopes();
        double dDirxDLinex = (liney * liney) / pow(linex * linex + liney * liney, 1.5);
        double dDiryDLinex = -(linex * liney) / pow(linex * linex + liney * liney, 1.5);
        // NOTE: d(linex)/d(x1) = -1
        result = slopex * (-dDiryDLinex) - slopey * (-dDirxDLinex);
        return scale * result;
    }
    if (param == linep2x()) {
        getSlopes();
        double dDirxDLinex = (liney * liney) / pow(linex * linex + liney * liney, 1.5);
        double dDiryDLinex = -(linex * liney) / pow(linex * linex + liney * liney, 1.5);
        // NOTE: d(linex)/d(x2) = 1
        result = slopex * dDiryDLinex - slopey * dDirxDLinex;
        return scale * result;
    }
    if (param == linep1y()) {
        getSlopes();
        double dDirxDLiney = -(linex * liney) / pow(linex * linex + liney * liney, 1.5);
        double dDiryDLiney = (linex * linex) / pow(linex * linex + liney * liney, 1.5);
        // NOTE: d(liney)/d(y1) = -1
        result = slopex * (-dDiryDLiney) - slopey * (-dDirxDLiney);
        return scale * result;
    }
    if (param == linep2y()) {
        getSlopes();
        double dDirxDLiney = -(linex * liney) / pow(linex * linex + liney * liney, 1.5);
        double dDiryDLiney = (linex * linex) / pow(linex * linex + liney * liney, 1.5);
        // NOTE: d(liney)/d(y2) = 1
        result = slopex * dDiryDLiney - slopey * dDirxDLiney;
        return scale * result;
    }

    return scale * result;
}


// --------------------------------------------------------
// Point On BSpline
ConstraintPointOnBSpline::ConstraintPointOnBSpline(double* point,
                                                   double* initparam,
                                                   int coordidx,
                                                   BSpline& b)
    : bsp(b)
{
    // This is always going to be true
    numpoints = bsp.degree + 1;

    pvec.reserve(2 + 2 * b.poles.size());
    pvec.push_back(point);
    pvec.push_back(initparam);

    setStartPole(*initparam);

    for (size_t i = 0; i < b.poles.size(); ++i) {
        if (coordidx == 0) {
            pvec.push_back(b.poles[i].x);
        }
        else {
            pvec.push_back(b.poles[i].y);
        }
    }
    for (size_t i = 0; i < b.weights.size(); ++i) {
        pvec.push_back(b.weights[i]);
    }

    if (bsp.flattenedknots.empty()) {
        bsp.setupFlattenedKnots();
    }

    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnBSpline::getTypeId()
{
    return PointOnBSpline;
}

void ConstraintPointOnBSpline::setStartPole(double u)
{
    // The startpole logic is repeated in a lot of places,
    // for example in GCS and slope at knot
    // find relevant poles
    startpole = 0;
    for (size_t j = 1; j < bsp.mult.size() && *(bsp.knots[j]) <= u; ++j) {
        startpole += bsp.mult[j];
    }
    if (!bsp.periodic && startpole >= bsp.poles.size()) {
        startpole = bsp.poles.size() - bsp.degree - 1;
    }
}

void ConstraintPointOnBSpline::rescale(double coef)
{
    scale = coef * 1.0;
}

double ConstraintPointOnBSpline::error()
{
    if (*theparam() < bsp.flattenedknots[startpole + bsp.degree]
        || *theparam() > bsp.flattenedknots[startpole + bsp.degree + 1]) {
        setStartPole(*theparam());
    }

    double sum = 0;
    double wsum = 0;

    // TODO: maybe make it global so it doesn't have to be created every time
    VEC_D d(numpoints);
    for (size_t i = 0; i < numpoints; ++i) {
        d[i] = *poleat(i) * *weightat(i);
    }
    sum = BSpline::splineValue(*theparam(),
                               startpole + bsp.degree,
                               bsp.degree,
                               d,
                               bsp.flattenedknots);
    for (size_t i = 0; i < numpoints; ++i) {
        d[i] = *weightat(i);
    }
    wsum = BSpline::splineValue(*theparam(),
                                startpole + bsp.degree,
                                bsp.degree,
                                d,
                                bsp.flattenedknots);

    // TODO: Change the poles as the point moves between pieces

    return scale * (*thepoint() * wsum - sum);
}

double ConstraintPointOnBSpline::grad(double* gcsparam)
{
    double deriv = 0.;
    if (gcsparam == thepoint()) {
        VEC_D d(numpoints);
        for (size_t i = 0; i < numpoints; ++i) {
            d[i] = *weightat(i);
        }
        double wsum = BSpline::splineValue(*theparam(),
                                           startpole + bsp.degree,
                                           bsp.degree,
                                           d,
                                           bsp.flattenedknots);
        deriv += wsum;
    }

    if (gcsparam == theparam()) {
        VEC_D d(numpoints - 1);
        for (size_t i = 1; i < numpoints; ++i) {
            d[i - 1] = (*poleat(i) * *weightat(i) - *poleat(i - 1) * *weightat(i - 1))
                / (bsp.flattenedknots[startpole + i + bsp.degree]
                   - bsp.flattenedknots[startpole + i]);
        }
        double slopevalue = BSpline::splineValue(*theparam(),
                                                 startpole + bsp.degree,
                                                 bsp.degree - 1,
                                                 d,
                                                 bsp.flattenedknots);
        for (size_t i = 1; i < numpoints; ++i) {
            d[i - 1] = (*weightat(i) - *weightat(i - 1))
                / (bsp.flattenedknots[startpole + i + bsp.degree]
                   - bsp.flattenedknots[startpole + i]);
        }
        double wslopevalue = BSpline::splineValue(*theparam(),
                                                  startpole + bsp.degree,
                                                  bsp.degree - 1,
                                                  d,
                                                  bsp.flattenedknots);
        deriv += (*thepoint() * wslopevalue - slopevalue) * bsp.degree;
    }

    for (size_t i = 0; i < numpoints; ++i) {
        if (gcsparam == poleat(i)) {
            auto factorsI =
                bsp.getLinCombFactor(*theparam(), startpole + bsp.degree, startpole + i);
            deriv += -(*weightat(i) * factorsI);
        }
        if (gcsparam == weightat(i)) {
            auto factorsI =
                bsp.getLinCombFactor(*theparam(), startpole + bsp.degree, startpole + i);
            deriv += (*thepoint() - *poleat(i)) * factorsI;
        }
    }

    return scale * deriv;
}

// Difference
ConstraintDifference::ConstraintDifference(double* p1, double* p2, double* d)
{
    pvec.push_back(p1);
    pvec.push_back(p2);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintDifference::getTypeId()
{
    return Difference;
}

void ConstraintDifference::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintDifference::error()
{
    return scale * (*param2() - *param1() - *difference());
}

double ConstraintDifference::grad(double* param)
{
    double deriv = 0.;
    if (param == param1()) {
        deriv += -1;
    }
    if (param == param2()) {
        deriv += 1;
    }
    if (param == difference()) {
        deriv += -1;
    }
    return scale * deriv;
}


// --------------------------------------------------------
// P2PDistance
ConstraintP2PDistance::ConstraintP2PDistance(Point& p1, Point& p2, double* d)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2PDistance::getTypeId()
{
    return P2PDistance;
}

void ConstraintP2PDistance::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintP2PDistance::error()
{
    double dx = (*p1x() - *p2x());
    double dy = (*p1y() - *p2y());
    double d = sqrt(dx * dx + dy * dy);
    double dist = *distance();
    return scale * (d - dist);
}

double ConstraintP2PDistance::grad(double* param)
{
    double deriv = 0.;
    if (param == p1x() || param == p1y() || param == p2x() || param == p2y()) {
        double dx = (*p1x() - *p2x());
        double dy = (*p1y() - *p2y());
        double d = sqrt(dx * dx + dy * dy);
        if (param == p1x()) {
            deriv += dx / d;
        }
        if (param == p1y()) {
            deriv += dy / d;
        }
        if (param == p2x()) {
            deriv += -dx / d;
        }
        if (param == p2y()) {
            deriv += -dy / d;
        }
    }
    if (param == distance()) {
        deriv += -1.;
    }

    return scale * deriv;
}

double ConstraintP2PDistance::maxStep(MAP_pD_D& dir, double lim)
{
    MAP_pD_D::iterator it;
    // distance() >= 0
    it = dir.find(distance());
    if (it != dir.end()) {
        if (it->second < 0.) {
            lim = std::min(lim, -(*distance()) / it->second);
        }
    }
    // restrict actual distance change
    double ddx = 0., ddy = 0.;
    it = dir.find(p1x());
    if (it != dir.end()) {
        ddx += it->second;
    }
    it = dir.find(p1y());
    if (it != dir.end()) {
        ddy += it->second;
    }
    it = dir.find(p2x());
    if (it != dir.end()) {
        ddx -= it->second;
    }
    it = dir.find(p2y());
    if (it != dir.end()) {
        ddy -= it->second;
    }
    double dd = sqrt(ddx * ddx + ddy * ddy);
    double dist = *distance();
    if (dd > dist) {
        double dx = (*p1x() - *p2x());
        double dy = (*p1y() - *p2y());
        double d = sqrt(dx * dx + dy * dy);
        if (dd > d) {
            lim = std::min(lim, std::max(d, dist) / dd);
        }
    }
    return lim;
}


// --------------------------------------------------------
// P2PAngle
ConstraintP2PAngle::ConstraintP2PAngle(Point& p1, Point& p2, double* a, double da_)
    : da(da_)
{
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2PAngle::getTypeId()
{
    return P2PAngle;
}

void ConstraintP2PAngle::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintP2PAngle::error()
{
    double dx = (*p2x() - *p1x());
    double dy = (*p2y() - *p1y());
    double a = *angle() + da;
    double ca = cos(a);
    double sa = sin(a);
    double x = dx * ca + dy * sa;
    double y = -dx * sa + dy * ca;
    return scale * atan2(y, x);
}

double ConstraintP2PAngle::grad(double* param)
{
    double deriv = 0.;
    if (param == p1x() || param == p1y() || param == p2x() || param == p2y()) {
        double dx = (*p2x() - *p1x());
        double dy = (*p2y() - *p1y());
        double a = *angle() + da;
        double ca = cos(a);
        double sa = sin(a);
        double x = dx * ca + dy * sa;
        double y = -dx * sa + dy * ca;
        double r2 = dx * dx + dy * dy;
        dx = -y / r2;
        dy = x / r2;
        if (param == p1x()) {
            deriv += (-ca * dx + sa * dy);
        }
        if (param == p1y()) {
            deriv += (-sa * dx - ca * dy);
        }
        if (param == p2x()) {
            deriv += (ca * dx - sa * dy);
        }
        if (param == p2y()) {
            deriv += (sa * dx + ca * dy);
        }
    }
    if (param == angle()) {
        deriv += -1;
    }

    return scale * deriv;
}

double ConstraintP2PAngle::maxStep(MAP_pD_D& dir, double lim)
{
    // step(angle()) <= pi/18 = 10°
    MAP_pD_D::iterator it = dir.find(angle());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI / 18.0) {
            lim = std::min(lim, (M_PI / 18.0) / step);
        }
    }
    return lim;
}


// --------------------------------------------------------
// P2LDistance
ConstraintP2LDistance::ConstraintP2LDistance(Point& p, Line& l, double* d)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    pvec.push_back(d);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintP2LDistance::getTypeId()
{
    return P2LDistance;
}

void ConstraintP2LDistance::rescale(double coef)
{
    scale = coef;
}

double ConstraintP2LDistance::error()
{
    double x0 = *p0x(), x1 = *p1x(), x2 = *p2x();
    double y0 = *p0y(), y1 = *p1y(), y2 = *p2y();
    double dist = *distance();
    double dx = x2 - x1;
    double dy = y2 - y1;
    double d = sqrt(dx * dx + dy * dy);  // line length
    double area =
        std::abs(-x0 * dy + y0 * dx + x1 * y2
                 - x2 * y1);  // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * (area / d - dist);
}

double ConstraintP2LDistance::grad(double* param)
{
    double deriv = 0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == p0x() || param == p0y() || param == p1x() || param == p1y() || param == p2x()
        || param == p2y()) {
        double x0 = *p0x(), x1 = *p1x(), x2 = *p2x();
        double y0 = *p0y(), y1 = *p1y(), y2 = *p2y();
        double dx = x2 - x1;
        double dy = y2 - y1;
        double d2 = dx * dx + dy * dy;
        double d = sqrt(d2);
        double area = -x0 * dy + y0 * dx + x1 * y2 - x2 * y1;
        if (param == p0x()) {
            deriv += (y1 - y2) / d;
        }
        if (param == p0y()) {
            deriv += (x2 - x1) / d;
        }
        if (param == p1x()) {
            deriv += ((y2 - y0) * d + (dx / d) * area) / d2;
        }
        if (param == p1y()) {
            deriv += ((x0 - x2) * d + (dy / d) * area) / d2;
        }
        if (param == p2x()) {
            deriv += ((y0 - y1) * d - (dx / d) * area) / d2;
        }
        if (param == p2y()) {
            deriv += ((x1 - x0) * d - (dy / d) * area) / d2;
        }
        if (area < 0) {
            deriv *= -1;
        }
    }
    if (param == distance()) {
        deriv += -1;
    }

    return scale * deriv;
}

double ConstraintP2LDistance::maxStep(MAP_pD_D& dir, double lim)
{
    MAP_pD_D::iterator it;
    // distance() >= 0
    it = dir.find(distance());
    if (it != dir.end()) {
        if (it->second < 0.) {
            lim = std::min(lim, -(*distance()) / it->second);
        }
    }
    // restrict actual area change
    double darea = 0.;
    double x0 = *p0x(), x1 = *p1x(), x2 = *p2x();
    double y0 = *p0y(), y1 = *p1y(), y2 = *p2y();
    it = dir.find(p0x());
    if (it != dir.end()) {
        darea += (y1 - y2) * it->second;
    }
    it = dir.find(p0y());
    if (it != dir.end()) {
        darea += (x2 - x1) * it->second;
    }
    it = dir.find(p1x());
    if (it != dir.end()) {
        darea += (y2 - y0) * it->second;
    }
    it = dir.find(p1y());
    if (it != dir.end()) {
        darea += (x0 - x2) * it->second;
    }
    it = dir.find(p2x());
    if (it != dir.end()) {
        darea += (y0 - y1) * it->second;
    }
    it = dir.find(p2y());
    if (it != dir.end()) {
        darea += (x1 - x0) * it->second;
    }

    darea = std::abs(darea);
    if (darea > 0.) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        double area = 0.3 * (*distance()) * sqrt(dx * dx + dy * dy);
        if (darea > area) {
            area = std::max(area, 0.3 * std::abs(-x0 * dy + y0 * dx + x1 * y2 - x2 * y1));
            if (darea > area) {
                lim = std::min(lim, area / darea);
            }
        }
    }
    return lim;
}


// --------------------------------------------------------
// PointOnLine
ConstraintPointOnLine::ConstraintPointOnLine(Point& p, Line& l)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnLine::ConstraintPointOnLine(Point& p, Point& lp1, Point& lp2)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(lp1.x);
    pvec.push_back(lp1.y);
    pvec.push_back(lp2.x);
    pvec.push_back(lp2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnLine::getTypeId()
{
    return PointOnLine;
}

void ConstraintPointOnLine::rescale(double coef)
{
    scale = coef;
}

double ConstraintPointOnLine::error()
{
    double x0 = *p0x(), x1 = *p1x(), x2 = *p2x();
    double y0 = *p0y(), y1 = *p1y(), y2 = *p2y();
    double dx = x2 - x1;
    double dy = y2 - y1;
    double d = sqrt(dx * dx + dy * dy);
    double area = -x0 * dy + y0 * dx + x1 * y2
        - x2 * y1;  // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * area / d;
}

double ConstraintPointOnLine::grad(double* param)
{
    double deriv = 0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == p0x() || param == p0y() || param == p1x() || param == p1y() || param == p2x()
        || param == p2y()) {
        double x0 = *p0x(), x1 = *p1x(), x2 = *p2x();
        double y0 = *p0y(), y1 = *p1y(), y2 = *p2y();
        double dx = x2 - x1;
        double dy = y2 - y1;
        double d2 = dx * dx + dy * dy;
        double d = sqrt(d2);
        double area = -x0 * dy + y0 * dx + x1 * y2 - x2 * y1;
        if (param == p0x()) {
            deriv += (y1 - y2) / d;
        }
        if (param == p0y()) {
            deriv += (x2 - x1) / d;
        }
        if (param == p1x()) {
            deriv += ((y2 - y0) * d + (dx / d) * area) / d2;
        }
        if (param == p1y()) {
            deriv += ((x0 - x2) * d + (dy / d) * area) / d2;
        }
        if (param == p2x()) {
            deriv += ((y0 - y1) * d - (dx / d) * area) / d2;
        }
        if (param == p2y()) {
            deriv += ((x1 - x0) * d - (dy / d) * area) / d2;
        }
    }
    return scale * deriv;
}


// --------------------------------------------------------
// PointOnPerpBisector
ConstraintPointOnPerpBisector::ConstraintPointOnPerpBisector(Point& p, Line& l)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(l.p1.x);
    pvec.push_back(l.p1.y);
    pvec.push_back(l.p2.x);
    pvec.push_back(l.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnPerpBisector::ConstraintPointOnPerpBisector(Point& p, Point& lp1, Point& lp2)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(lp1.x);
    pvec.push_back(lp1.y);
    pvec.push_back(lp2.x);
    pvec.push_back(lp2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnPerpBisector::getTypeId()
{
    return PointOnPerpBisector;
}

void ConstraintPointOnPerpBisector::rescale(double coef)
{
    scale = coef;
}

void ConstraintPointOnPerpBisector::errorgrad(double* err, double* grad, double* param)
{
    DeriVector2 p0(Point(p0x(), p0y()), param);
    DeriVector2 p1(Point(p1x(), p1y()), param);
    DeriVector2 p2(Point(p2x(), p2y()), param);

    DeriVector2 d1 = p0.subtr(p1);
    DeriVector2 d2 = p0.subtr(p2);
    DeriVector2 D = p2.subtr(p1).getNormalized();

    double projd1, dprojd1;
    projd1 = d1.scalarProd(D, &dprojd1);

    double projd2, dprojd2;
    projd2 = d2.scalarProd(D, &dprojd2);

    if (err) {
        *err = projd1 + projd2;
    }
    if (grad) {
        *grad = dprojd1 + dprojd2;
    }
}

double ConstraintPointOnPerpBisector::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintPointOnPerpBisector::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}


// --------------------------------------------------------
// Parallel
ConstraintParallel::ConstraintParallel(Line& l1, Line& l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintParallel::getTypeId()
{
    return Parallel;
}

void ConstraintParallel::rescale(double coef)
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    scale = coef / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2));
}

double ConstraintParallel::error()
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    return scale * (dx1 * dy2 - dy1 * dx2);
}

double ConstraintParallel::grad(double* param)
{
    double deriv = 0.;
    if (param == l1p1x()) {
        deriv += (*l2p1y() - *l2p2y());  // = dy2
    }
    if (param == l1p2x()) {
        deriv += -(*l2p1y() - *l2p2y());  // = -dy2
    }
    if (param == l1p1y()) {
        deriv += -(*l2p1x() - *l2p2x());  // = -dx2
    }
    if (param == l1p2y()) {
        deriv += (*l2p1x() - *l2p2x());  // = dx2
    }

    if (param == l2p1x()) {
        deriv += -(*l1p1y() - *l1p2y());  // = -dy1
    }
    if (param == l2p2x()) {
        deriv += (*l1p1y() - *l1p2y());  // = dy1
    }
    if (param == l2p1y()) {
        deriv += (*l1p1x() - *l1p2x());  // = dx1
    }
    if (param == l2p2y()) {
        deriv += -(*l1p1x() - *l1p2x());  // = -dx1
    }

    return scale * deriv;
}


// --------------------------------------------------------
// Perpendicular
ConstraintPerpendicular::ConstraintPerpendicular(Line& l1, Line& l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintPerpendicular::ConstraintPerpendicular(Point& l1p1, Point& l1p2, Point& l2p1, Point& l2p2)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPerpendicular::getTypeId()
{
    return Perpendicular;
}

void ConstraintPerpendicular::rescale(double coef)
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    scale = coef / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2));
}

double ConstraintPerpendicular::error()
{
    double dx1 = (*l1p1x() - *l1p2x());
    double dy1 = (*l1p1y() - *l1p2y());
    double dx2 = (*l2p1x() - *l2p2x());
    double dy2 = (*l2p1y() - *l2p2y());
    return scale * (dx1 * dx2 + dy1 * dy2);
}

double ConstraintPerpendicular::grad(double* param)
{
    double deriv = 0.;
    if (param == l1p1x()) {
        deriv += (*l2p1x() - *l2p2x());  // = dx2
    }
    if (param == l1p2x()) {
        deriv += -(*l2p1x() - *l2p2x());  // = -dx2
    }
    if (param == l1p1y()) {
        deriv += (*l2p1y() - *l2p2y());  // = dy2
    }
    if (param == l1p2y()) {
        deriv += -(*l2p1y() - *l2p2y());  // = -dy2
    }

    if (param == l2p1x()) {
        deriv += (*l1p1x() - *l1p2x());  // = dx1
    }
    if (param == l2p2x()) {
        deriv += -(*l1p1x() - *l1p2x());  // = -dx1
    }
    if (param == l2p1y()) {
        deriv += (*l1p1y() - *l1p2y());  // = dy1
    }
    if (param == l2p2y()) {
        deriv += -(*l1p1y() - *l1p2y());  // = -dy1
    }

    return scale * deriv;
}


// --------------------------------------------------------
// L2LAngle
ConstraintL2LAngle::ConstraintL2LAngle(Line& l1, Line& l2, double* a)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintL2LAngle::ConstraintL2LAngle(Point& l1p1,
                                       Point& l1p2,
                                       Point& l2p1,
                                       Point& l2p2,
                                       double* a)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    pvec.push_back(a);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintL2LAngle::getTypeId()
{
    return L2LAngle;
}

void ConstraintL2LAngle::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintL2LAngle::error()
{
    double dx1 = (*l1p2x() - *l1p1x());
    double dy1 = (*l1p2y() - *l1p1y());
    double dx2 = (*l2p2x() - *l2p1x());
    double dy2 = (*l2p2y() - *l2p1y());
    double a = atan2(dy1, dx1) + *angle();
    double ca = cos(a);
    double sa = sin(a);
    double x2 = dx2 * ca + dy2 * sa;
    double y2 = -dx2 * sa + dy2 * ca;
    return scale * atan2(y2, x2);
}

double ConstraintL2LAngle::grad(double* param)
{
    double deriv = 0.;
    if (param == l1p1x() || param == l1p1y() || param == l1p2x() || param == l1p2y()) {
        double dx1 = (*l1p2x() - *l1p1x());
        double dy1 = (*l1p2y() - *l1p1y());
        double r2 = dx1 * dx1 + dy1 * dy1;
        if (param == l1p1x()) {
            deriv += -dy1 / r2;
        }
        if (param == l1p1y()) {
            deriv += dx1 / r2;
        }
        if (param == l1p2x()) {
            deriv += dy1 / r2;
        }
        if (param == l1p2y()) {
            deriv += -dx1 / r2;
        }
    }
    if (param == l2p1x() || param == l2p1y() || param == l2p2x() || param == l2p2y()) {
        double dx1 = (*l1p2x() - *l1p1x());
        double dy1 = (*l1p2y() - *l1p1y());
        double dx2 = (*l2p2x() - *l2p1x());
        double dy2 = (*l2p2y() - *l2p1y());
        double a = atan2(dy1, dx1) + *angle();
        double ca = cos(a);
        double sa = sin(a);
        double x2 = dx2 * ca + dy2 * sa;
        double y2 = -dx2 * sa + dy2 * ca;
        double r2 = dx2 * dx2 + dy2 * dy2;
        dx2 = -y2 / r2;
        dy2 = x2 / r2;
        if (param == l2p1x()) {
            deriv += (-ca * dx2 + sa * dy2);
        }
        if (param == l2p1y()) {
            deriv += (-sa * dx2 - ca * dy2);
        }
        if (param == l2p2x()) {
            deriv += (ca * dx2 - sa * dy2);
        }
        if (param == l2p2y()) {
            deriv += (sa * dx2 + ca * dy2);
        }
    }
    if (param == angle()) {
        deriv += -1;
    }

    return scale * deriv;
}

double ConstraintL2LAngle::maxStep(MAP_pD_D& dir, double lim)
{
    // step(angle()) <= pi/18 = 10°
    MAP_pD_D::iterator it = dir.find(angle());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI / 18.0) {
            lim = std::min(lim, (M_PI / 18.0) / step);
        }
    }
    return lim;
}


// --------------------------------------------------------
// MidpointOnLine
ConstraintMidpointOnLine::ConstraintMidpointOnLine(Line& l1, Line& l2)
{
    pvec.push_back(l1.p1.x);
    pvec.push_back(l1.p1.y);
    pvec.push_back(l1.p2.x);
    pvec.push_back(l1.p2.y);
    pvec.push_back(l2.p1.x);
    pvec.push_back(l2.p1.y);
    pvec.push_back(l2.p2.x);
    pvec.push_back(l2.p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintMidpointOnLine::ConstraintMidpointOnLine(Point& l1p1,
                                                   Point& l1p2,
                                                   Point& l2p1,
                                                   Point& l2p2)
{
    pvec.push_back(l1p1.x);
    pvec.push_back(l1p1.y);
    pvec.push_back(l1p2.x);
    pvec.push_back(l1p2.y);
    pvec.push_back(l2p1.x);
    pvec.push_back(l2p1.y);
    pvec.push_back(l2p2.x);
    pvec.push_back(l2p2.y);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintMidpointOnLine::getTypeId()
{
    return MidpointOnLine;
}

void ConstraintMidpointOnLine::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintMidpointOnLine::error()
{
    double x0 = ((*l1p1x()) + (*l1p2x())) / 2;
    double y0 = ((*l1p1y()) + (*l1p2y())) / 2;
    double x1 = *l2p1x(), x2 = *l2p2x();
    double y1 = *l2p1y(), y2 = *l2p2y();
    double dx = x2 - x1;
    double dy = y2 - y1;
    double d = sqrt(dx * dx + dy * dy);
    double area = -x0 * dy + y0 * dx + x1 * y2
        - x2 * y1;  // = x1y2 - x2y1 - x0y2 + x2y0 + x0y1 - x1y0 = 2*(triangle area)
    return scale * area / d;
}

double ConstraintMidpointOnLine::grad(double* param)
{
    double deriv = 0.;
    // darea/dx0 = (y1-y2)      darea/dy0 = (x2-x1)
    // darea/dx1 = (y2-y0)      darea/dy1 = (x0-x2)
    // darea/dx2 = (y0-y1)      darea/dy2 = (x1-x0)
    if (param == l1p1x() || param == l1p1y() || param == l1p2x() || param == l1p2y()
        || param == l2p1x() || param == l2p1y() || param == l2p2x() || param == l2p2y()) {
        double x0 = ((*l1p1x()) + (*l1p2x())) / 2;
        double y0 = ((*l1p1y()) + (*l1p2y())) / 2;
        double x1 = *l2p1x(), x2 = *l2p2x();
        double y1 = *l2p1y(), y2 = *l2p2y();
        double dx = x2 - x1;
        double dy = y2 - y1;
        double d2 = dx * dx + dy * dy;
        double d = sqrt(d2);
        double area = -x0 * dy + y0 * dx + x1 * y2 - x2 * y1;
        if (param == l1p1x()) {
            deriv += (y1 - y2) / (2 * d);
        }
        if (param == l1p1y()) {
            deriv += (x2 - x1) / (2 * d);
        }
        if (param == l1p2x()) {
            deriv += (y1 - y2) / (2 * d);
        }
        if (param == l1p2y()) {
            deriv += (x2 - x1) / (2 * d);
        }
        if (param == l2p1x()) {
            deriv += ((y2 - y0) * d + (dx / d) * area) / d2;
        }
        if (param == l2p1y()) {
            deriv += ((x0 - x2) * d + (dy / d) * area) / d2;
        }
        if (param == l2p2x()) {
            deriv += ((y0 - y1) * d - (dx / d) * area) / d2;
        }
        if (param == l2p2y()) {
            deriv += ((x1 - x0) * d - (dy / d) * area) / d2;
        }
    }
    return scale * deriv;
}


// --------------------------------------------------------
// TangentCircumf
ConstraintTangentCircumf::ConstraintTangentCircumf(Point& p1,
                                                   Point& p2,
                                                   double* rad1,
                                                   double* rad2,
                                                   bool internal_)
{
    internal = internal_;
    pvec.push_back(p1.x);
    pvec.push_back(p1.y);
    pvec.push_back(p2.x);
    pvec.push_back(p2.y);
    pvec.push_back(rad1);
    pvec.push_back(rad2);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintTangentCircumf::getTypeId()
{
    return TangentCircumf;
}

void ConstraintTangentCircumf::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintTangentCircumf::error()
{
    double dx = (*c1x() - *c2x());
    double dy = (*c1y() - *c2y());
    if (internal) {
        return scale * (sqrt(dx * dx + dy * dy) - std::abs(*r1() - *r2()));
    }
    else {
        return scale * (sqrt(dx * dx + dy * dy) - (*r1() + *r2()));
    }
}

double ConstraintTangentCircumf::grad(double* param)
{
    double deriv = 0.;
    if (param == c1x() || param == c1y() || param == c2x() || param == c2y() || param == r1()
        || param == r2()) {
        double dx = (*c1x() - *c2x());
        double dy = (*c1y() - *c2y());
        double d = sqrt(dx * dx + dy * dy);
        if (param == c1x()) {
            deriv += dx / d;
        }
        if (param == c1y()) {
            deriv += dy / d;
        }
        if (param == c2x()) {
            deriv += -dx / d;
        }
        if (param == c2y()) {
            deriv += -dy / d;
        }
        if (internal) {
            if (param == r1()) {
                deriv += (*r1() > *r2()) ? -1 : 1;
            }
            if (param == r2()) {
                deriv += (*r1() > *r2()) ? 1 : -1;
            }
        }
        else {
            if (param == r1()) {
                deriv += -1;
            }
            if (param == r2()) {
                deriv += -1;
            }
        }
    }
    return scale * deriv;
}


// --------------------------------------------------------
// ConstraintPointOnEllipse
ConstraintPointOnEllipse::ConstraintPointOnEllipse(Point& p, Ellipse& e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1.x);
    pvec.push_back(e.focus1.y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnEllipse::getTypeId()
{
    return PointOnEllipse;
}

void ConstraintPointOnEllipse::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintPointOnEllipse::error()
{
    double X_0 = *p1x();
    double Y_0 = *p1y();
    double X_c = *cx();
    double Y_c = *cy();
    double X_F1 = *f1x();
    double Y_F1 = *f1y();
    double b = *rmin();

    double err = sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
        + sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2))
        - 2 * sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
    return scale * err;
}

double ConstraintPointOnEllipse::grad(double* param)
{
    double deriv = 0.;
    if (param == p1x() || param == p1y() || param == f1x() || param == f1y() || param == cx()
        || param == cy() || param == rmin()) {

        double X_0 = *p1x();
        double Y_0 = *p1y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y();
        double b = *rmin();

        if (param == p1x()) {
            deriv += (X_0 - X_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                + (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == p1y()) {
            deriv += (Y_0 - Y_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                + (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == f1x()) {
            deriv += -(X_0 - X_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                - 2 * (X_F1 - X_c) / sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == f1y()) {
            deriv += -(Y_0 - Y_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                - 2 * (Y_F1 - Y_c) / sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == cx()) {
            deriv += 2 * (X_F1 - X_c) / sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                - 2 * (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == cy()) {
            deriv += 2 * (Y_F1 - Y_c) / sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                - 2 * (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == rmin()) {
            deriv += -2 * b / sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
        }
    }
    return scale * deriv;
}


// --------------------------------------------------------
// ConstraintEllipseTangentLine
ConstraintEllipseTangentLine::ConstraintEllipseTangentLine(Line& l, Ellipse& e)
{
    this->l = l;
    this->l.PushOwnParams(pvec);

    this->e = e;
    this->e.PushOwnParams(pvec);  // DeepSOIC: hopefully, this won't push arc's parameters
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEllipseTangentLine::ReconstructGeomPointers()
{
    int i = 0;
    l.ReconstructOnNewPvec(pvec, i);
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEllipseTangentLine::getTypeId()
{
    return TangentEllipseLine;
}

void ConstraintEllipseTangentLine::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEllipseTangentLine::errorgrad(double* err, double* grad, double* param)
{
    // DeepSOIC equation
    // http://forum.freecad.org/viewtopic.php?f=10&t=7520&start=140

    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }
    DeriVector2 p1(l.p1, param);
    DeriVector2 p2(l.p2, param);
    DeriVector2 f1(e.focus1, param);
    DeriVector2 c(e.center, param);
    DeriVector2 f2 = c.linCombi(2.0, f1, -1.0);  // 2*cv - f1v

    // mirror F1 against the line
    DeriVector2 nl = l.CalculateNormal(l.p1, param).getNormalized();
    double distF1L = 0, ddistF1L = 0;  // distance F1 to line
    distF1L = f1.subtr(p1).scalarProd(nl, &ddistF1L);
    DeriVector2 f1m = f1.sum(nl.multD(-2 * distF1L, -2 * ddistF1L));  // f1m = f1 mirrored

    // calculate distance form f1m to f2
    double distF1mF2, ddistF1mF2;
    distF1mF2 = f2.subtr(f1m).length(ddistF1mF2);

    // calculate major radius (to compare the distance to)
    double dradmin = (param == e.radmin) ? 1.0 : 0.0;
    double radmaj, dradmaj;
    radmaj = e.getRadMaj(c, f1, *e.radmin, dradmin, dradmaj);

    if (err) {
        *err = distF1mF2 - 2 * radmaj;
    }
    if (grad) {
        *grad = ddistF1mF2 - 2 * dradmaj;
    }
}

double ConstraintEllipseTangentLine::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintEllipseTangentLine::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

// use numeric for testing
#if 0
        double const eps = 0.00001;
        double oldparam = *param;
        double v0 = this->error();
        *param += eps;
        double vr = this->error();
        *param = oldparam - eps;
        double vl = this->error();
        *param = oldparam;
        //If not nasty, real derivative should be between left one and right one
        double numretl = (v0 - vl) / eps;
        double numretr = (vr - v0) / eps;
        assert(deriv <= std::max(numretl, numretr));
        assert(deriv >= std::min(numretl, numretr));
#endif

    return deriv * scale;
}


// --------------------------------------------------------
// ConstraintInternalAlignmentPoint2Ellipse
ConstraintInternalAlignmentPoint2Ellipse::ConstraintInternalAlignmentPoint2Ellipse(
    Ellipse& e,
    Point& p1,
    InternalAlignmentType alignmentType)
{
    this->p = p1;
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    this->e = e;
    this->e.PushOwnParams(pvec);
    this->AlignmentType = alignmentType;
    origpvec = pvec;
    rescale();
}

void ConstraintInternalAlignmentPoint2Ellipse::ReconstructGeomPointers()
{
    int i = 0;
    p.x = pvec[i];
    i++;
    p.y = pvec[i];
    i++;
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintInternalAlignmentPoint2Ellipse::getTypeId()
{
    return InternalAlignmentPoint2Ellipse;
}

void ConstraintInternalAlignmentPoint2Ellipse::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintInternalAlignmentPoint2Ellipse::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    // todo: prefill only what's needed, not everything

    DeriVector2 c(e.center, param);
    DeriVector2 f1(e.focus1, param);
    DeriVector2 emaj = f1.subtr(c).getNormalized();
    DeriVector2 emin = emaj.rotate90ccw();
    DeriVector2 pv(p, param);
    double b, db;  // minor radius
    b = *e.radmin;
    db = (e.radmin == param) ? 1.0 : 0.0;

    // major radius
    double a, da;
    a = e.getRadMaj(c, f1, b, db, da);

    DeriVector2 poa;  // point to align to
    bool by_y_not_by_x =
        false;  // a flag to indicate if the alignment error function is for y (false - x, true - y)

    switch (AlignmentType) {
        case EllipsePositiveMajorX:
        case EllipsePositiveMajorY:
            poa = c.sum(emaj.multD(a, da));
            by_y_not_by_x = AlignmentType == EllipsePositiveMajorY;
            break;
        case EllipseNegativeMajorX:
        case EllipseNegativeMajorY:
            poa = c.sum(emaj.multD(-a, -da));
            by_y_not_by_x = AlignmentType == EllipseNegativeMajorY;
            break;
        case EllipsePositiveMinorX:
        case EllipsePositiveMinorY:
            poa = c.sum(emin.multD(b, db));
            by_y_not_by_x = AlignmentType == EllipsePositiveMinorY;
            break;
        case EllipseNegativeMinorX:
        case EllipseNegativeMinorY:
            poa = c.sum(emin.multD(-b, -db));
            by_y_not_by_x = AlignmentType == EllipseNegativeMinorY;
            break;
        case EllipseFocus2X:
        case EllipseFocus2Y:
            poa = c.linCombi(2.0, f1, -1.0);
            by_y_not_by_x = AlignmentType == EllipseFocus2Y;
            break;
        default:
            // shouldn't happen
            poa = pv;  // align to the point itself, doing nothing essentially
    }
    if (err) {
        *err = by_y_not_by_x ? pv.y - poa.y : pv.x - poa.x;
    }
    if (grad) {
        *grad = by_y_not_by_x ? pv.dy - poa.dy : pv.dx - poa.dx;
    }
}

double ConstraintInternalAlignmentPoint2Ellipse::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintInternalAlignmentPoint2Ellipse::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

// use numeric for testing
#if 0
        double const eps = 0.00001;
        double oldparam = *param;
        double v0 = this->error();
        *param += eps;
        double vr = this->error();
        *param = oldparam - eps;
        double vl = this->error();
        *param = oldparam;
        //If not nasty, real derivative should be between left one and right one
        double numretl = (v0 - vl) / eps;
        double numretr = (vr - v0) / eps;
        assert(deriv <= std::max(numretl, numretr));
        assert(deriv >= std::min(numretl, numretr));
#endif

    return deriv * scale;
}


// --------------------------------------------------------
// ConstraintInternalAlignmentPoint2Hyperbola
ConstraintInternalAlignmentPoint2Hyperbola::ConstraintInternalAlignmentPoint2Hyperbola(
    Hyperbola& e,
    Point& p1,
    InternalAlignmentType alignmentType)
{
    this->p = p1;
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    this->e = e;
    this->e.PushOwnParams(pvec);
    this->AlignmentType = alignmentType;
    origpvec = pvec;
    rescale();
}

void ConstraintInternalAlignmentPoint2Hyperbola::ReconstructGeomPointers()
{
    int i = 0;
    p.x = pvec[i];
    i++;
    p.y = pvec[i];
    i++;
    e.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintInternalAlignmentPoint2Hyperbola::getTypeId()
{
    return InternalAlignmentPoint2Hyperbola;
}

void ConstraintInternalAlignmentPoint2Hyperbola::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintInternalAlignmentPoint2Hyperbola::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    // todo: prefill only what's needed, not everything

    DeriVector2 c(e.center, param);
    DeriVector2 f1(e.focus1, param);
    DeriVector2 emaj = f1.subtr(c).getNormalized();
    DeriVector2 emin = emaj.rotate90ccw();
    DeriVector2 pv(p, param);

    double b, db;  // minor radius
    b = *e.radmin;
    db = (e.radmin == param) ? 1.0 : 0.0;

    // major radius
    double a, da;
    a = e.getRadMaj(c, f1, b, db, da);

    DeriVector2 poa;  // point to align to
    bool by_y_not_by_x =
        false;  // a flag to indicate if the alignment error function is for y (false - x, true - y)

    switch (AlignmentType) {
        case HyperbolaPositiveMajorX:
        case HyperbolaPositiveMajorY:
            poa = c.sum(emaj.multD(a, da));
            by_y_not_by_x = AlignmentType == HyperbolaPositiveMajorY;
            break;
        case HyperbolaNegativeMajorX:
        case HyperbolaNegativeMajorY:
            poa = c.sum(emaj.multD(-a, -da));
            by_y_not_by_x = AlignmentType == HyperbolaNegativeMajorY;
            break;
        case HyperbolaPositiveMinorX:
        case HyperbolaPositiveMinorY: {
            DeriVector2 pa = c.sum(emaj.multD(a, da));
            // DeriVector2 A(pa.x,pa.y);
            // poa = A.sum(emin.multD(b, db));
            poa = pa.sum(emin.multD(b, db));
            by_y_not_by_x = AlignmentType == HyperbolaPositiveMinorY;
            break;
        }
        case HyperbolaNegativeMinorX:
        case HyperbolaNegativeMinorY: {
            DeriVector2 pa = c.sum(emaj.multD(a, da));
            // DeriVector2 A(pa.x,pa.y);
            // poa = A.sum(emin.multD(-b, -db));
            poa = pa.sum(emin.multD(-b, -db));
            by_y_not_by_x = AlignmentType == HyperbolaNegativeMinorY;
            break;
        }
        default:
            // shouldn't happen
            poa = pv;  // align to the point itself, doing nothing essentially
    }

    if (err) {
        *err = by_y_not_by_x ? pv.y - poa.y : pv.x - poa.x;
    }
    if (grad) {
        *grad = by_y_not_by_x ? pv.dy - poa.dy : pv.dx - poa.dx;
    }
}

double ConstraintInternalAlignmentPoint2Hyperbola::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintInternalAlignmentPoint2Hyperbola::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}


// --------------------------------------------------------
//  ConstraintEqualMajorAxesEllipse
ConstraintEqualMajorAxesConic::ConstraintEqualMajorAxesConic(MajorRadiusConic* a1,
                                                             MajorRadiusConic* a2)
{
    this->e1 = a1;
    this->e1->PushOwnParams(pvec);
    this->e2 = a2;
    this->e2->PushOwnParams(pvec);
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEqualMajorAxesConic::ReconstructGeomPointers()
{
    int i = 0;
    e1->ReconstructOnNewPvec(pvec, i);
    e2->ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEqualMajorAxesConic::getTypeId()
{
    return EqualMajorAxesConic;
}

void ConstraintEqualMajorAxesConic::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEqualMajorAxesConic::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }
    double a1, da1;
    a1 = e1->getRadMaj(param, da1);
    double a2, da2;
    a2 = e2->getRadMaj(param, da2);
    if (err) {
        *err = a2 - a1;
    }
    if (grad) {
        *grad = da2 - da1;
    }
}

double ConstraintEqualMajorAxesConic::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintEqualMajorAxesConic::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}

//  ConstraintEqualFocalDistance
ConstraintEqualFocalDistance::ConstraintEqualFocalDistance(ArcOfParabola* a1, ArcOfParabola* a2)
{
    this->e1 = a1;
    this->e1->PushOwnParams(pvec);
    this->e2 = a2;
    this->e2->PushOwnParams(pvec);
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEqualFocalDistance::ReconstructGeomPointers()
{
    int i = 0;
    e1->ReconstructOnNewPvec(pvec, i);
    e2->ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEqualFocalDistance::getTypeId()
{
    return EqualFocalDistance;
}

void ConstraintEqualFocalDistance::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEqualFocalDistance::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 focus1(this->e1->focus1, param);
    DeriVector2 vertex1(this->e1->vertex, param);

    DeriVector2 focalvect1 = vertex1.subtr(focus1);

    double focal1, dfocal1;

    focal1 = focalvect1.length(dfocal1);

    DeriVector2 focus2(this->e2->focus1, param);
    DeriVector2 vertex2(this->e2->vertex, param);

    DeriVector2 focalvect2 = vertex2.subtr(focus2);

    double focal2, dfocal2;

    focal2 = focalvect2.length(dfocal2);

    if (err) {
        *err = focal2 - focal1;
    }
    if (grad) {
        *grad = dfocal2 - dfocal1;
    }
}

double ConstraintEqualFocalDistance::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintEqualFocalDistance::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}


// --------------------------------------------------------
// ConstraintCurveValue
ConstraintCurveValue::ConstraintCurveValue(Point& p, double* pcoord, Curve& crv, double* u)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(pcoord);
    pvec.push_back(u);
    crv.PushOwnParams(pvec);
    this->crv = crv.Copy();
    pvecChangedFlag = true;
    origpvec = pvec;
    rescale();
}

ConstraintCurveValue::~ConstraintCurveValue()
{
    delete this->crv;
    this->crv = nullptr;
}

void ConstraintCurveValue::ReconstructGeomPointers()
{
    int i = 0;
    p.x = pvec[i];
    i++;
    p.y = pvec[i];
    i++;
    i++;  // we have an inline function for point coordinate
    i++;  // we have an inline function for the parameterU
    this->crv->ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintCurveValue::getTypeId()
{
    return CurveValue;
}

void ConstraintCurveValue::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintCurveValue::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    double u, du;
    u = *(this->u());
    du = (param == this->u()) ? 1.0 : 0.0;

    DeriVector2 P_to;  // point of curve at parameter value of u, in global coordinates
    P_to = this->crv->Value(u, du, param);

    DeriVector2 P_from(this->p, param);  // point to be constrained

    DeriVector2 err_vec = P_from.subtr(P_to);

    if (this->pcoord() == this->p.x) {  // this constraint is for X projection
        if (err) {
            *err = err_vec.x;
        }
        if (grad) {
            *grad = err_vec.dx;
        }
    }
    else if (this->pcoord() == this->p.y) {  // this constraint is for Y projection
        if (err) {
            *err = err_vec.y;
        }
        if (grad) {
            *grad = err_vec.dy;
        }
    }
    else {
        assert(false /*this constraint is neither X nor Y. Nothing to do..*/);
    }
}

double ConstraintCurveValue::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintCurveValue::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}

double ConstraintCurveValue::maxStep(MAP_pD_D& /*dir*/, double lim)
{
    // step(angle()) <= pi/18 = 10°
    /* TODO: curve-dependent parameter change limiting??
    MAP_pD_D::iterator it = dir.find(this->u());
    if (it != dir.end()) {
        double step = std::abs(it->second);
        if (step > M_PI/18.)
            lim = std::min(lim, (M_PI/18.) / step);
    }
    */
    return lim;
}


// --------------------------------------------------------
// ConstraintPointOnHyperbola
ConstraintPointOnHyperbola::ConstraintPointOnHyperbola(Point& p, Hyperbola& e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1.x);
    pvec.push_back(e.focus1.y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintPointOnHyperbola::ConstraintPointOnHyperbola(Point& p, ArcOfHyperbola& e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    pvec.push_back(e.center.x);
    pvec.push_back(e.center.y);
    pvec.push_back(e.focus1.x);
    pvec.push_back(e.focus1.y);
    pvec.push_back(e.radmin);
    origpvec = pvec;
    rescale();
}

ConstraintType ConstraintPointOnHyperbola::getTypeId()
{
    return PointOnHyperbola;
}

void ConstraintPointOnHyperbola::rescale(double coef)
{
    scale = coef * 1;
}

double ConstraintPointOnHyperbola::error()
{
    double X_0 = *p1x();
    double Y_0 = *p1y();
    double X_c = *cx();
    double Y_c = *cy();
    double X_F1 = *f1x();
    double Y_F1 = *f1y();
    double b = *rmin();

    // Full sage worksheet at:
    // http://forum.freecad.org/viewtopic.php?f=10&t=8038&p=110447#p110447
    //
    // Err = |PF2| - |PF1| - 2*a
    // sage code:
    // C = vector([X_c,Y_c])
    // F2 = C+(C-F1)
    // X_F2 = F2[0]
    // Y_F2 = F2[1]
    // a = sqrt((F1-C)*(F1-C)-b*b);
    // show(a)
    // DM=sqrt((P-F2)*(P-F2))-sqrt((P-F1)*(P-F1))-2*a
    // show(DM.simplify_radical())
    double err = -sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
        + sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2))
        - 2 * sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
    return scale * err;
}

double ConstraintPointOnHyperbola::grad(double* param)
{
    double deriv = 0.;
    if (param == p1x() || param == p1y() || param == f1x() || param == f1y() || param == cx()
        || param == cy() || param == rmin()) {

        double X_0 = *p1x();
        double Y_0 = *p1y();
        double X_c = *cx();
        double Y_c = *cy();
        double X_F1 = *f1x();
        double Y_F1 = *f1y();
        double b = *rmin();

        if (param == p1x()) {
            deriv += -(X_0 - X_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                + (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == p1y()) {
            deriv += -(Y_0 - Y_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                + (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == f1x()) {
            deriv += (X_0 - X_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                - 2 * (X_F1 - X_c) / sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == f1y()) {
            deriv += (Y_0 - Y_F1) / sqrt(pow(X_0 - X_F1, 2) + pow(Y_0 - Y_F1, 2))
                - 2 * (Y_F1 - Y_c) / sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                + (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == cx()) {
            deriv += 2 * (X_F1 - X_c) / sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                - 2 * (X_0 + X_F1 - 2 * X_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == cy()) {
            deriv += 2 * (Y_F1 - Y_c) / sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                - 2 * (Y_0 + Y_F1 - 2 * Y_c)
                    / sqrt(pow(X_0 + X_F1 - 2 * X_c, 2) + pow(Y_0 + Y_F1 - 2 * Y_c, 2));
        }
        if (param == rmin()) {
            deriv += 2 * b / sqrt(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2));
        }
    }
    return scale * deriv;
}


// --------------------------------------------------------
// ConstraintPointOnParabola
ConstraintPointOnParabola::ConstraintPointOnParabola(Point& p, Parabola& e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    e.PushOwnParams(pvec);
    this->parab = e.Copy();
    pvecChangedFlag = true;
    origpvec = pvec;
    rescale();
}

ConstraintPointOnParabola::ConstraintPointOnParabola(Point& p, ArcOfParabola& e)
{
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    e.PushOwnParams(pvec);
    this->parab = e.Copy();
    pvecChangedFlag = true;
    origpvec = pvec;
    rescale();
}

ConstraintPointOnParabola::~ConstraintPointOnParabola()
{
    delete this->parab;
    this->parab = nullptr;
}

void ConstraintPointOnParabola::ReconstructGeomPointers()
{
    int i = 0;
    p.x = pvec[i];
    i++;
    p.y = pvec[i];
    i++;
    this->parab->ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintPointOnParabola::getTypeId()
{
    return PointOnParabola;
}

void ConstraintPointOnParabola::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintPointOnParabola::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 focus(this->parab->focus1, param);
    DeriVector2 vertex(this->parab->vertex, param);

    DeriVector2 point(this->p, param);  // point to be constrained to parabola

    DeriVector2 focalvect = focus.subtr(vertex);

    DeriVector2 xdir = focalvect.getNormalized();

    DeriVector2 point_to_focus = point.subtr(focus);

    double focal, dfocal;

    focal = focalvect.length(dfocal);

    double pf, dpf;

    pf = point_to_focus.length(dpf);

    double proj, dproj;

    proj = point_to_focus.scalarProd(xdir, &dproj);

    if (err) {
        *err = pf - 2 * focal - proj;
    }
    if (grad) {
        *grad = dpf - 2 * dfocal - dproj;
    }
}

double ConstraintPointOnParabola::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintPointOnParabola::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}


// --------------------------------------------------------
// ConstraintAngleViaPoint
ConstraintAngleViaPoint::ConstraintAngleViaPoint(Curve& acrv1, Curve& acrv2, Point p, double* angle)
{
    pvec.push_back(angle);
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    acrv1.PushOwnParams(pvec);
    acrv2.PushOwnParams(pvec);
    crv1 = acrv1.Copy();
    crv2 = acrv2.Copy();
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

ConstraintAngleViaPoint::~ConstraintAngleViaPoint()
{
    delete crv1;
    crv1 = nullptr;
    delete crv2;
    crv2 = nullptr;
}

void ConstraintAngleViaPoint::ReconstructGeomPointers()
{
    int cnt = 0;
    cnt++;  // skip angle - we have an inline function for that
    poa.x = pvec[cnt];
    cnt++;
    poa.y = pvec[cnt];
    cnt++;
    crv1->ReconstructOnNewPvec(pvec, cnt);
    crv2->ReconstructOnNewPvec(pvec, cnt);
    pvecChangedFlag = false;
}

ConstraintType ConstraintAngleViaPoint::getTypeId()
{
    return AngleViaPoint;
}

void ConstraintAngleViaPoint::rescale(double coef)
{
    scale = coef * 1.;
}

double ConstraintAngleViaPoint::error()
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }
    double ang = *angle();
    DeriVector2 n1 = crv1->CalculateNormal(poa);
    DeriVector2 n2 = crv2->CalculateNormal(poa);

    // rotate n1 by angle
    DeriVector2 n1r(n1.x * cos(ang) - n1.y * sin(ang), n1.x * sin(ang) + n1.y * cos(ang));

    // calculate angle between n1r and n2. Since we have rotated the n1, the angle is the error
    // function. for our atan2, y is a dot product (n2) * (n1r rotated ccw by 90 degrees).
    //                x is a dot product (n2) * (n1r)
    double err = atan2(-n2.x * n1r.y + n2.y * n1r.x, n2.x * n1r.x + n2.y * n1r.y);
    // essentially, the function is equivalent to atan2(n2)-(atan2(n1)+angle). The only difference
    // is behavior when normals are zero (the intended result is also zero in this case).
    return scale * err;
}

double ConstraintAngleViaPoint::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv = 0.;

    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    if (param == angle()) {
        deriv += -1.0;
    }
    DeriVector2 n1 = crv1->CalculateNormal(poa, param);
    DeriVector2 n2 = crv2->CalculateNormal(poa, param);
    deriv -= ((-n1.dx) * n1.y / pow(n1.length(), 2) + n1.dy * n1.x / pow(n1.length(), 2));
    deriv += ((-n2.dx) * n2.y / pow(n2.length(), 2) + n2.dy * n2.x / pow(n2.length(), 2));


// use numeric for testing
#if 0
    double const eps = 0.00001;
    double oldparam = *param;
    double v0 = this->error();
    *param += eps;
    double vr = this->error();
    *param = oldparam - eps;
    double vl = this->error();
    *param = oldparam;
    //If not nasty, real derivative should be between left one and right one
    double numretl = (v0-vl)/eps;
    double numretr = (vr-v0)/eps;
    assert(deriv <= std::max(numretl,numretr) );
    assert(deriv >= std::min(numretl,numretr) );
#endif

    return scale * deriv;
}


// --------------------------------------------------------
// ConstraintSnell
ConstraintSnell::ConstraintSnell(Curve& ray1,
                                 Curve& ray2,
                                 Curve& boundary,
                                 Point p,
                                 double* n1,
                                 double* n2,
                                 bool flipn1,
                                 bool flipn2)
{
    pvec.push_back(n1);
    pvec.push_back(n2);
    pvec.push_back(p.x);
    pvec.push_back(p.y);
    ray1.PushOwnParams(pvec);
    ray2.PushOwnParams(pvec);
    boundary.PushOwnParams(pvec);
    this->ray1 = ray1.Copy();
    this->ray2 = ray2.Copy();
    this->boundary = boundary.Copy();
    origpvec = pvec;
    pvecChangedFlag = true;

    this->flipn1 = flipn1;
    this->flipn2 = flipn2;

    rescale();
}

ConstraintSnell::~ConstraintSnell()
{
    delete ray1;
    ray1 = nullptr;
    delete ray2;
    ray2 = nullptr;
    delete boundary;
    boundary = nullptr;
}

void ConstraintSnell::ReconstructGeomPointers()
{
    int cnt = 0;
    cnt++;
    cnt++;  // skip n1, n2 - we have an inline function for that
    poa.x = pvec[cnt];
    cnt++;
    poa.y = pvec[cnt];
    cnt++;
    ray1->ReconstructOnNewPvec(pvec, cnt);
    ray2->ReconstructOnNewPvec(pvec, cnt);
    boundary->ReconstructOnNewPvec(pvec, cnt);
    pvecChangedFlag = false;
}

ConstraintType ConstraintSnell::getTypeId()
{
    return Snell;
}

void ConstraintSnell::rescale(double coef)
{
    scale = coef * 1.;
}

// error and gradient combined. Values are returned through pointers.
void ConstraintSnell::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }
    DeriVector2 tang1 = ray1->CalculateNormal(poa, param).rotate90cw().getNormalized();
    DeriVector2 tang2 = ray2->CalculateNormal(poa, param).rotate90cw().getNormalized();
    DeriVector2 tangB = boundary->CalculateNormal(poa, param).rotate90cw().getNormalized();
    double sin1, dsin1, sin2, dsin2;
    sin1 = tang1.scalarProd(tangB, &dsin1);  // sinus of angle of incidence
    sin2 = tang2.scalarProd(tangB, &dsin2);
    if (flipn1) {
        sin1 = -sin1;
        dsin1 = -dsin1;
    }
    if (flipn2) {
        sin2 = -sin2;
        dsin2 = -dsin2;
    }

    double dn1 = (param == n1()) ? 1.0 : 0.0;
    double dn2 = (param == n2()) ? 1.0 : 0.0;
    if (err) {
        *err = *n1() * sin1 - *n2() * sin2;
    }
    if (grad) {
        *grad = dn1 * sin1 + *n1() * dsin1 - dn2 * sin2 - *n2() * dsin2;
    }
}

double ConstraintSnell::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintSnell::grad(double* param)
{
    // first of all, check that we need to compute anything.
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

// use numeric for testing
#if 0
    double const eps = 0.00001;
    double oldparam = *param;
    double v0 = this->error();
    *param += eps;
    double vr = this->error();
    *param = oldparam - eps;
    double vl = this->error();
    *param = oldparam;
    //If not nasty, real derivative should be between left one and right one
    double numretl = (v0 - vl) / eps;
    double numretr = (vr - v0) / eps;
    assert(deriv <= std::max(numretl, numretr));
    assert(deriv >= std::min(numretl, numretr));
#endif

    return scale * deriv;
}


// --------------------------------------------------------
// ConstraintEqualLineLength
ConstraintEqualLineLength::ConstraintEqualLineLength(Line& l1, Line& l2)
{
    this->l1 = l1;
    this->l1.PushOwnParams(pvec);

    this->l2 = l2;
    this->l2.PushOwnParams(pvec);
    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintEqualLineLength::ReconstructGeomPointers()
{
    int i = 0;
    l1.ReconstructOnNewPvec(pvec, i);
    l2.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintEqualLineLength::getTypeId()
{
    return EqualLineLength;
}

void ConstraintEqualLineLength::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintEqualLineLength::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 p1(l1.p1, param);
    DeriVector2 p2(l1.p2, param);
    DeriVector2 p3(l2.p1, param);
    DeriVector2 p4(l2.p2, param);

    DeriVector2 v1 = p1.subtr(p2);
    DeriVector2 v2 = p3.subtr(p4);

    double length1, dlength1;
    length1 = v1.length(dlength1);

    double length2, dlength2;
    length2 = v2.length(dlength2);

    if (err) {
        *err = length2 - length1;
    }

    if (grad) {
        *grad = dlength2 - dlength1;
        // if the one of the lines gets vertical or horizontal, the gradients will become zero. this
        // will affect the diagnose function and the detection of dependent/independent parameters.
        //
        // So here we maintain the very small derivative of 1e-10 when the gradient is under such
        // value, such that the diagnose function with pivot threshold of 1e-13 treats the value as
        // non-zero and correctly detects and can tell apart when a parameter is fully constrained
        // or just locked into a maximum/minimum
        if (fabs(*grad) < 1e-10) {
            double surrogate = 1e-10;
            if (param == l1.p1.x) {
                *grad = v1.x > 0 ? surrogate : -surrogate;
            }
            if (param == l1.p1.y) {
                *grad = v1.y > 0 ? surrogate : -surrogate;
            }
            if (param == l1.p2.x) {
                *grad = v1.x > 0 ? -surrogate : surrogate;
            }
            if (param == l1.p2.y) {
                *grad = v1.y > 0 ? -surrogate : surrogate;
            }
            if (param == l2.p1.x) {
                *grad = v2.x > 0 ? surrogate : -surrogate;
            }
            if (param == l2.p1.y) {
                *grad = v2.y > 0 ? surrogate : -surrogate;
            }
            if (param == l2.p2.x) {
                *grad = v2.x > 0 ? -surrogate : surrogate;
            }
            if (param == l2.p2.y) {
                *grad = v2.y > 0 ? -surrogate : surrogate;
            }
        }
    }
}

double ConstraintEqualLineLength::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintEqualLineLength::grad(double* param)
{
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}


// --------------------------------------------------------
// ConstraintC2CDistance
ConstraintC2CDistance::ConstraintC2CDistance(Circle& c1, Circle& c2, double* d)
{
    this->d = d;
    pvec.push_back(d);

    this->c1 = c1;
    this->c1.PushOwnParams(pvec);

    this->c2 = c2;
    this->c2.PushOwnParams(pvec);

    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

void ConstraintC2CDistance::ReconstructGeomPointers()
{
    int i = 0;
    i++;  // skip the first parameter as there is the inline function distance for it
    c1.ReconstructOnNewPvec(pvec, i);
    c2.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

ConstraintType ConstraintC2CDistance::getTypeId()
{
    return C2CDistance;
}

void ConstraintC2CDistance::rescale(double coef)
{
    scale = coef * 1;
}

void ConstraintC2CDistance::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 ct1(c1.center, param);
    DeriVector2 ct2(c2.center, param);

    DeriVector2 vector_ct12 = ct1.subtr(ct2);

    double length_ct12, dlength_ct12;
    length_ct12 = vector_ct12.length(dlength_ct12);

    // outer case (defined as the centers of the circles are outside the center of the other
    // circles) it may well be that the circles intersect.
    if (length_ct12 >= *c1.rad && length_ct12 >= *c2.rad) {
        if (err) {
            *err = length_ct12 - (*c2.rad + *c1.rad + *distance());
        }
        else if (grad) {
            double drad = (param == c2.rad || param == c1.rad || param == distance()) ? -1.0 : 0.0;
            *grad = dlength_ct12 + drad;
        }
    }
    else {
        double* bigradius = (*c1.rad >= *c2.rad) ? c1.rad : c2.rad;
        double* smallradius = (*c1.rad >= *c2.rad) ? c2.rad : c1.rad;

        double smallspan = *smallradius + length_ct12 + *distance();

        if (err) {
            *err = *bigradius - smallspan;
        }
        else if (grad) {
            double drad = 0.0;

            if (param == bigradius) {
                drad = 1.0;
            }
            else if (param == smallradius) {
                drad = -1.0;
            }
            else if (param == distance()) {
                drad = (*distance() < 0.) ? 1.0 : -1.0;
            }
            if (length_ct12 > 1e-13) {
                *grad = -dlength_ct12 + drad;
            }
            else {  // concentric case
                *grad = drad;
            }
        }
    }
}

double ConstraintC2CDistance::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintC2CDistance::grad(double* param)
{
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}

// --------------------------------------------------------
// ConstraintC2LDistance
ConstraintC2LDistance::ConstraintC2LDistance(Circle& c, Line& l, double* d)
{
    this->d = d;
    pvec.push_back(d);

    this->circle = c;
    this->circle.PushOwnParams(pvec);

    this->line = l;
    this->line.PushOwnParams(pvec);

    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

ConstraintType ConstraintC2LDistance::getTypeId()
{
    return C2LDistance;
}

void ConstraintC2LDistance::rescale(double coef)
{
    scale = coef;
}

void ConstraintC2LDistance::ReconstructGeomPointers()
{
    int i = 0;
    i++;  // skip the first parameter as there is the inline function distance for it
    circle.ReconstructOnNewPvec(pvec, i);
    line.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

void ConstraintC2LDistance::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 ct(circle.center, param);
    DeriVector2 p1(line.p1, param);
    DeriVector2 p2(line.p2, param);
    DeriVector2 v_line = p2.subtr(p1);
    DeriVector2 v_p1ct = ct.subtr(p1);

    // center to line distance (=h) and its derivative (=dh)
    double darea = 0.0;
    double area = v_line.crossProdNorm(v_p1ct, darea);  // parallelogram oriented area

    double dlength;
    double length = v_line.length(dlength);

    // vector product (cross vector) has a magnitude corresponding to the area of
    // the parallelogram defined by the vectors above. The area of the triangle is
    // half the parallelogram area. The height of the triangle is the area divided by
    // the base, which is the distance from the center of the circle to the line.
    //
    // However, the vector (which points in z direction), can be positive or negative.
    // the area is the absolute value
    double h = std::abs(area) / length;

    // darea is the magnitude of a vector in the z direction, which makes the area vector
    // increase or decrease. If area vector is negative a negative value makes the area increase
    // and a positive value makes it decrease.
    darea = std::signbit(area) ? -darea : darea;

    double dh = (darea - h * dlength) / length;

    if (err) {
        *err = *distance() + *circle.rad - h;
    }
    else if (grad) {
        if (param == distance() || param == circle.rad) {
            *grad = 1.0;
        }
        else {
            *grad = -dh;
        }
    }
}

double ConstraintC2LDistance::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintC2LDistance::grad(double* param)
{
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}

// --------------------------------------------------------
// ConstraintP2CDistance
ConstraintP2CDistance::ConstraintP2CDistance(Point& p, Circle& c, double* d)
{
    this->d = d;
    pvec.push_back(d);

    this->circle = c;
    this->circle.PushOwnParams(pvec);

    this->pt = p;
    this->pt.PushOwnParams(pvec);

    origpvec = pvec;
    pvecChangedFlag = true;
    rescale();
}

ConstraintType ConstraintP2CDistance::getTypeId()
{
    return P2CDistance;
}

void ConstraintP2CDistance::rescale(double coef)
{
    scale = coef;
}

void ConstraintP2CDistance::ReconstructGeomPointers()
{
    int i = 0;
    i++;  // skip the first parameter as there is the inline function distance for it
    circle.ReconstructOnNewPvec(pvec, i);
    pt.ReconstructOnNewPvec(pvec, i);
    pvecChangedFlag = false;
}

void ConstraintP2CDistance::errorgrad(double* err, double* grad, double* param)
{
    if (pvecChangedFlag) {
        ReconstructGeomPointers();
    }

    DeriVector2 ct(circle.center, param);
    DeriVector2 p(pt, param);
    DeriVector2 v_length = ct.subtr(p);

    double dlength;
    double length = v_length.length(dlength);

    if (err) {
        *err = *circle.rad + *distance() - length;
        if (length < *circle.rad) {
            *err = *circle.rad - *distance() - length;
        }
    }
    else if (grad) {
        if (param == distance()) {
            *grad = 1.0;
            if (length < *circle.rad) {
                *grad = -1.0;
            }
        }
        else if (param == circle.rad) {
            *grad = 1.0;
        }
        else {
            *grad = -dlength;
        }
    }
}

double ConstraintP2CDistance::error()
{
    double err;
    errorgrad(&err, nullptr, nullptr);
    return scale * err;
}

double ConstraintP2CDistance::grad(double* param)
{
    if (findParamInPvec(param) == -1) {
        return 0.0;
    }

    double deriv;
    errorgrad(nullptr, &deriv, param);

    return deriv * scale;
}

}  // namespace GCS
