/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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
#pragma warning(disable : 4244)
#pragma warning(disable : 4996)
#endif

//#define _GCS_DEBUG
//#define _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
//#define _DEBUG_TO_FILE // Many matrices surpass the report view string size.
#undef _GCS_DEBUG
#undef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
#undef _DEBUG_TO_FILE

// This has to be included BEFORE any EIGEN include
// This format is Sage compatible, so you can just copy/paste the matrix into Sage
#ifdef _GCS_DEBUG
#define     EIGEN_DEFAULT_IO_FORMAT   Eigen::IOFormat(3,0,",",",\n","[","]","[","]")
/* Parameters:
 *
 * StreamPrecision,
 * int _flags = 0,
 * const std::string &     _coeffSeparator = " ",
 * const std::string &     _rowSeparator = "\n",
 * const std::string &     _rowPrefix = "",
 * const std::string &     _rowSuffix = "",
 * const std::string &     _matPrefix = "",
 * const std::string &     _matSuffix = "" )*/
#endif

#include <iostream>
#include <algorithm>
#include <cfloat>
#include <limits>

#include "GCS.h"
#include "qp_eq.h"

// NOTE: In CMakeList.txt -DEIGEN_NO_DEBUG is set (it does not work with a define here), to solve this:
// this is needed to fix this SparseQR crash http://forum.freecadweb.org/viewtopic.php?f=10&t=11341&p=92146#p92146,
// until Eigen library fixes its own problem with the assertion (definitely not solved in 3.2.0 branch)
// NOTE2: solved in eigen3.3

#define EIGEN_VERSION (EIGEN_WORLD_VERSION * 10000 \
+ EIGEN_MAJOR_VERSION * 100 \
+ EIGEN_MINOR_VERSION)

// Extraction of Q matrix for Debugging used to crash
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
#if EIGEN_VERSION >= 30304
#define SPARSE_Q_MATRIX
#endif
#endif


#if EIGEN_VERSION >= 30202
#define EIGEN_SPARSEQR_COMPATIBLE
#if EIGEN_VERSION > 30290   // This regulates that only starting in Eigen 3.3, the problem with
                            // http://forum.freecadweb.org/viewtopic.php?f=3&t=4651&start=40
                            // was solved in Eigen:
                            // http://forum.freecadweb.org/viewtopic.php?f=10&t=12769&start=60#p106492
                            // https://forum.kde.org/viewtopic.php?f=74&t=129439
#define EIGEN_STOCK_FULLPIVLU_COMPUTE
#endif
#endif

//#undef EIGEN_SPARSEQR_COMPATIBLE

#include <Eigen/QR>

#ifdef EIGEN_SPARSEQR_COMPATIBLE
#include <Eigen/Sparse>
#include <Eigen/OrderingMethods>
#endif

// _GCS_EXTRACT_SOLVER_SUBSYSTEM_ to be enabled in Constraints.h when needed.
#if defined(_GCS_EXTRACT_SOLVER_SUBSYSTEM_) || defined(_DEBUG_TO_FILE)
#include <fstream>

#define CASE_NOT_IMP(X) case X: { subsystemfile << "//" #X "not yet implemented" << std::endl; break; }
#endif

#include <FCConfig.h>
#include <Base/Console.h>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

typedef Eigen::FullPivHouseholderQR<Eigen::MatrixXd>::IntDiagSizeVectorType MatrixIndexType;

#ifdef _GCS_DEBUG
void LogMatrix(std::string str, Eigen::MatrixXd matrix )
{
#ifdef _DEBUG_TO_FILE
    std::ofstream stream;
    stream.open("GCS_debug.txt", std::ofstream::out | std::ofstream::app);
#else
    // Debug code starts
    std::stringstream stream;
#endif

    stream << '\n' << " " << str << " =" << '\n';
    stream << "[" << '\n';
    stream << matrix << '\n' ;
    stream << "]" << '\n';

#ifdef _DEBUG_TO_FILE
    stream.flush();
    stream.close();
#else
    const std::string tmp = stream.str();

    Base::Console().Log(tmp.c_str());
#endif
}

void LogMatrix(std::string str, MatrixIndexType matrix )
{
    #ifdef _DEBUG_TO_FILE
    std::ofstream stream;
    stream.open("GCS_debug.txt", std::ofstream::out | std::ofstream::app);
    #else
    // Debug code starts
    std::stringstream stream;
    #endif

    stream << '\n' << " " << str << " =" << '\n';
    stream << "[" << '\n';
    stream << matrix << '\n' ;
    stream << "]" << '\n';

    #ifdef _DEBUG_TO_FILE
    stream.flush();
    stream.close();
    #else
    const std::string tmp = stream.str();

    Base::Console().Log(tmp.c_str());
    #endif
}
#endif

void LogString(std::string str)
{
    #ifdef _DEBUG_TO_FILE
    std::ofstream stream;
    stream.open("GCS_debug.txt", std::ofstream::out | std::ofstream::app);
    #else
    // Debug code starts
    std::stringstream stream;
    #endif

    stream << str << std::endl;

    #ifdef _DEBUG_TO_FILE
    stream.flush();
    stream.close();
    #else
    const std::string tmp = stream.str();

    Base::Console().Log(tmp.c_str());
    #endif
}

#ifndef EIGEN_STOCK_FULLPIVLU_COMPUTE
namespace Eigen {

typedef Matrix<double,-1,-1,0,-1,-1> MatrixdType;

template<>
FullPivLU<MatrixdType>& FullPivLU<MatrixdType>::compute(const MatrixdType& matrix)
{
  m_isInitialized = true;
  m_lu = matrix;

  const Index size = matrix.diagonalSize();
  const Index rows = matrix.rows();
  const Index cols = matrix.cols();

  // will store the transpositions, before we accumulate them at the end.
  // can't accumulate on-the-fly because that will be done in reverse order for the rows.
  m_rowsTranspositions.resize(matrix.rows());
  m_colsTranspositions.resize(matrix.cols());
  Index number_of_transpositions = 0; // number of NONTRIVIAL transpositions, i.e. m_rowsTranspositions[i]!=i

  m_nonzero_pivots = size; // the generic case is that in which all pivots are nonzero (invertible case)
  m_maxpivot = RealScalar(0);
  RealScalar cutoff(0);

  for(Index k = 0; k < size; ++k)
  {
    // First, we need to find the pivot.

    // biggest coefficient in the remaining bottom-right corner (starting at row k, col k)
    Index row_of_biggest_in_corner, col_of_biggest_in_corner;
    RealScalar biggest_in_corner;
    biggest_in_corner = m_lu.bottomRightCorner(rows-k, cols-k)
                        .cwiseAbs()
                        .maxCoeff(&row_of_biggest_in_corner, &col_of_biggest_in_corner);
    row_of_biggest_in_corner += k; // correct the values! since they were computed in the corner,
    col_of_biggest_in_corner += k; // need to add k to them.

    // when k==0, biggest_in_corner is the biggest coeff absolute value in the original matrix
    if(k == 0) cutoff = biggest_in_corner * NumTraits<Scalar>::epsilon();

    // if the pivot (hence the corner) is "zero", terminate to avoid generating nan/inf values.
    // Notice that using an exact comparison (biggest_in_corner==0) here, as Golub-van Loan do in
    // their pseudo-code, results in numerical instability! The cutoff here has been validated
    // by running the unit test 'lu' with many repetitions.
    if(biggest_in_corner < cutoff)
    {
      // before exiting, make sure to initialize the still uninitialized transpositions
      // in a sane state without destroying what we already have.
      m_nonzero_pivots = k;
      for(Index i = k; i < size; ++i)
      {
        m_rowsTranspositions.coeffRef(i) = i;
        m_colsTranspositions.coeffRef(i) = i;
      }
      break;
    }

    if(biggest_in_corner > m_maxpivot) m_maxpivot = biggest_in_corner;

    // Now that we've found the pivot, we need to apply the row/col swaps to
    // bring it to the location (k,k).

    m_rowsTranspositions.coeffRef(k) = row_of_biggest_in_corner;
    m_colsTranspositions.coeffRef(k) = col_of_biggest_in_corner;
    if(k != row_of_biggest_in_corner) {
      m_lu.row(k).swap(m_lu.row(row_of_biggest_in_corner));
      ++number_of_transpositions;
    }
    if(k != col_of_biggest_in_corner) {
      m_lu.col(k).swap(m_lu.col(col_of_biggest_in_corner));
      ++number_of_transpositions;
    }

    // Now that the pivot is at the right location, we update the remaining
    // bottom-right corner by Gaussian elimination.

    if(k<rows-1)
      m_lu.col(k).tail(rows-k-1) /= m_lu.coeff(k,k);
    if(k<size-1)
      m_lu.block(k+1,k+1,rows-k-1,cols-k-1).noalias() -= m_lu.col(k).tail(rows-k-1) * m_lu.row(k).tail(cols-k-1);
  }

  // the main loop is over, we still have to accumulate the transpositions to find the
  // permutations P and Q

  m_p.setIdentity(rows);
  for(Index k = size-1; k >= 0; --k)
    m_p.applyTranspositionOnTheRight(k, m_rowsTranspositions.coeff(k));

  m_q.setIdentity(cols);
  for(Index k = 0; k < size; ++k)
    m_q.applyTranspositionOnTheRight(k, m_colsTranspositions.coeff(k));

  m_det_pq = (number_of_transpositions%2) ? -1 : 1;
  return *this;
}

} // Eigen
#endif

namespace GCS
{

typedef boost::adjacency_list <boost::vecS, boost::vecS, boost::undirectedS> Graph;

///////////////////////////////////////
// Solver
///////////////////////////////////////

// System
System::System()
  : plist(0)
  , pdrivenlist(0)
  , pdependentparameters(0)
  , clist(0)
  , c2p()
  , p2c()
  , subSystems(0)
  , subSystemsAux(0)
  , reference(0)
  , dofs(0)
  , hasUnknowns(false)
  , hasDiagnosis(false)
  , isInit(false)
  , maxIter(100)
  , maxIterRedundant(100)
  , sketchSizeMultiplier(false)
  , sketchSizeMultiplierRedundant(false)
  , convergence(1e-10)
  , convergenceRedundant(1e-10)
  , qrAlgorithm(EigenSparseQR)
  , dogLegGaussStep(FullPivLU)
  , qrpivotThreshold(1E-13)
  , debugMode(Minimal)
  , LM_eps(1E-10)
  , LM_eps1(1E-80)
  , LM_tau(1E-3)
  , DL_tolg(1E-80)
  , DL_tolx(1E-80)
  , DL_tolf(1E-10)
  , LM_epsRedundant(1E-10)
  , LM_eps1Redundant(1E-80)
  , LM_tauRedundant(1E-3)
  , DL_tolgRedundant(1E-80)
  , DL_tolxRedundant(1E-80)
  , DL_tolfRedundant(1E-10)
{
    // currently Eigen only supports multithreading for multiplications
    // There is no appreciable gain from using more threads
#ifdef EIGEN_SPARSEQR_COMPATIBLE
    Eigen::setNbThreads(1);
#endif
}

/*DeepSOIC: seriously outdated, needs redesign
System::System(std::vector<Constraint *> clist_)
: plist(0),
  c2p(), p2c(),
  subSystems(0), subSystemsAux(0),
  reference(0),
  hasUnknowns(false), hasDiagnosis(false), isInit(false)
{
    // create own (shallow) copy of constraints
    for (std::vector<Constraint *>::iterator constr=clist_.begin();
         constr != clist_.end(); ++constr) {
        Constraint *newconstr = 0;
        switch ((*constr)->getTypeId()) {
            case Equal: {
                ConstraintEqual *oldconstr = static_cast<ConstraintEqual *>(*constr);
                newconstr = new ConstraintEqual(*oldconstr);
                break;
            }
            case Difference: {
                ConstraintDifference *oldconstr = static_cast<ConstraintDifference *>(*constr);
                newconstr = new ConstraintDifference(*oldconstr);
                break;
            }
            case P2PDistance: {
                ConstraintP2PDistance *oldconstr = static_cast<ConstraintP2PDistance *>(*constr);
                newconstr = new ConstraintP2PDistance(*oldconstr);
                break;
            }
            case P2PAngle: {
                ConstraintP2PAngle *oldconstr = static_cast<ConstraintP2PAngle *>(*constr);
                newconstr = new ConstraintP2PAngle(*oldconstr);
                break;
            }
            case P2LDistance: {
                ConstraintP2LDistance *oldconstr = static_cast<ConstraintP2LDistance *>(*constr);
                newconstr = new ConstraintP2LDistance(*oldconstr);
                break;
            }
            case PointOnLine: {
                ConstraintPointOnLine *oldconstr = static_cast<ConstraintPointOnLine *>(*constr);
                newconstr = new ConstraintPointOnLine(*oldconstr);
                break;
            }
            case Parallel: {
                ConstraintParallel *oldconstr = static_cast<ConstraintParallel *>(*constr);
                newconstr = new ConstraintParallel(*oldconstr);
                break;
            }
            case Perpendicular: {
                ConstraintPerpendicular *oldconstr = static_cast<ConstraintPerpendicular *>(*constr);
                newconstr = new ConstraintPerpendicular(*oldconstr);
                break;
            }
            case L2LAngle: {
                ConstraintL2LAngle *oldconstr = static_cast<ConstraintL2LAngle *>(*constr);
                newconstr = new ConstraintL2LAngle(*oldconstr);
                break;
            }
            case MidpointOnLine: {
                ConstraintMidpointOnLine *oldconstr = static_cast<ConstraintMidpointOnLine *>(*constr);
                newconstr = new ConstraintMidpointOnLine(*oldconstr);
                break;
            }
            case None:
                break;
        }
        if (newconstr)
            addConstraint(newconstr);
    }
}
*/

System::~System()
{
    clear();
}

void System::clear()
{
    plist.clear();
    pdrivenlist.clear();
    pIndex.clear();
    pdependentparameters.clear();
    hasUnknowns = false;
    hasDiagnosis = false;

    redundant.clear();
    conflictingTags.clear();
    redundantTags.clear();

    reference.clear();
    clearSubSystems();
    free(clist);
    c2p.clear();
    p2c.clear();
}

void System::clearByTag(int tagId)
{
    std::vector<Constraint *> constrvec;
    for (std::vector<Constraint *>::const_iterator
         constr=clist.begin(); constr != clist.end(); ++constr) {
        if ((*constr)->getTag() == tagId)
            constrvec.push_back(*constr);
    }
    for (std::vector<Constraint *>::const_iterator
         constr=constrvec.begin(); constr != constrvec.end(); ++constr) {
        removeConstraint(*constr);
    }
}

int System::addConstraint(Constraint *constr)
{
    isInit = false;
    if (constr->getTag() >= 0) // negatively tagged constraints have no impact
        hasDiagnosis = false;  // on the diagnosis

    clist.push_back(constr);
    VEC_pD constr_params = constr->params();
    for (VEC_pD::const_iterator param=constr_params.begin();
         param != constr_params.end(); ++param) {
//        jacobi.set(constr, *param, 0.);
        c2p[constr].push_back(*param);
        p2c[*param].push_back(constr);
    }
    return clist.size()-1;
}

void System::removeConstraint(Constraint *constr)
{
    std::vector<Constraint *>::iterator it;
    it = std::find(clist.begin(), clist.end(), constr);
    if (it == clist.end())
        return;

    clist.erase(it);
    if (constr->getTag() >= 0)
        hasDiagnosis = false;
    clearSubSystems();

    VEC_pD constr_params = c2p[constr];
    for (VEC_pD::const_iterator param=constr_params.begin();
         param != constr_params.end(); ++param) {
        std::vector<Constraint *> &constraints = p2c[*param];
        it = std::find(constraints.begin(), constraints.end(), constr);
        constraints.erase(it);
    }
    c2p.erase(constr);

    std::vector<Constraint *> constrvec;
    constrvec.push_back(constr);
    free(constrvec);
}

// basic constraints

int System::addConstraintEqual(double *param1, double *param2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintEqual(param1, param2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintProportional(double *param1, double *param2, double ratio, int tagId, bool driving)
{
    Constraint *constr = new ConstraintEqual(param1, param2, ratio);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintDifference(double *param1, double *param2,
                                    double *difference, int tagId, bool driving)
{
    Constraint *constr = new ConstraintDifference(param1, param2, difference);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PDistance(Point &p1, Point &p2, double *distance, int tagId, bool driving)
{
    Constraint *constr = new ConstraintP2PDistance(p1, p2, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PAngle(Point &p1, Point &p2, double *angle,
                                  double incrAngle, int tagId, bool driving)
{
    Constraint *constr = new ConstraintP2PAngle(p1, p2, angle, incrAngle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PAngle(Point &p1, Point &p2, double *angle, int /*tagId*/, bool driving)
{
    return addConstraintP2PAngle(p1, p2, angle, 0., 0, driving);
}

int System::addConstraintP2LDistance(Point &p, Line &l, double *distance, int tagId, bool driving)
{
    Constraint *constr = new ConstraintP2LDistance(p, l, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnLine(Point &p, Line &l, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnLine(p, l);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnLine(Point &p, Point &lp1, Point &lp2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnLine(p, lp1, lp2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnPerpBisector(Point &p, Line &l, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnPerpBisector(p, l);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnPerpBisector(Point &p, Point &lp1, Point &lp2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnPerpBisector(p, lp1, lp2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintParallel(Line &l1, Line &l2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintParallel(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPerpendicular(Line &l1, Line &l2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPerpendicular(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPerpendicular(Point &l1p1, Point &l1p2,
                                       Point &l2p1, Point &l2p2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPerpendicular(l1p1, l1p2, l2p1, l2p2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintL2LAngle(Line &l1, Line &l2, double *angle, int tagId, bool driving)
{
    Constraint *constr = new ConstraintL2LAngle(l1, l2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintL2LAngle(Point &l1p1, Point &l1p2,
                                  Point &l2p1, Point &l2p2, double *angle, int tagId, bool driving)
{
    Constraint *constr = new ConstraintL2LAngle(l1p1, l1p2, l2p1, l2p2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintAngleViaPoint(Curve &crv1, Curve &crv2, Point &p, double *angle, int tagId, bool driving)
{
    Constraint *constr = new ConstraintAngleViaPoint(crv1, crv2, p, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintMidpointOnLine(Line &l1, Line &l2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintMidpointOnLine(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintMidpointOnLine(Point &l1p1, Point &l1p2,
                                        Point &l2p1, Point &l2p2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintMidpointOnLine(l1p1, l1p2, l2p1, l2p2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintTangentCircumf(Point &p1, Point &p2, double *rad1, double *rad2,
                                        bool internal, int tagId, bool driving)
{
    Constraint *constr = new ConstraintTangentCircumf(p1, p2, rad1, rad2, internal);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

// derived constraints

int System::addConstraintP2PCoincident(Point &p1, Point &p2, int tagId, bool driving)
{
    addConstraintEqual(p1.x, p2.x, tagId, driving);
    return addConstraintEqual(p1.y, p2.y, tagId, driving);
}

int System::addConstraintHorizontal(Line &l, int tagId, bool driving)
{
    return addConstraintEqual(l.p1.y, l.p2.y, tagId, driving);
}

int System::addConstraintHorizontal(Point &p1, Point &p2, int tagId, bool driving)
{
    return addConstraintEqual(p1.y, p2.y, tagId, driving);
}

int System::addConstraintVertical(Line &l, int tagId, bool driving)
{
    return addConstraintEqual(l.p1.x, l.p2.x, tagId, driving);
}

int System::addConstraintVertical(Point &p1, Point &p2, int tagId, bool driving)
{
    return addConstraintEqual(p1.x, p2.x, tagId, driving);
}

int System::addConstraintCoordinateX(Point &p, double *x, int tagId, bool driving)
{
    return addConstraintEqual(p.x, x, tagId, driving);
}

int System::addConstraintCoordinateY(Point &p, double *y, int tagId, bool driving)
{
    return addConstraintEqual(p.y, y, tagId, driving);
}

int System::addConstraintArcRules(Arc &a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start, a, a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end, a, a.endAngle, tagId, driving);
}

int System::addConstraintPointOnCircle(Point &p, Circle &c, int tagId, bool driving)
{
    return addConstraintP2PDistance(p, c.center, c.rad, tagId, driving);
}

int System::addConstraintPointOnEllipse(Point &p, Ellipse &e, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnEllipse(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnHyperbolicArc(Point &p, ArcOfHyperbola &e, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnHyperbola(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnParabolicArc(Point &p, ArcOfParabola &e, int tagId, bool driving)
{
    Constraint *constr = new ConstraintPointOnParabola(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintArcOfEllipseRules(ArcOfEllipse &a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start,a,a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end,a,a.endAngle, tagId, driving);
}

int System::addConstraintCurveValue(Point &p, Curve &a, double *u, int tagId, bool driving)
{
    Constraint *constr = new ConstraintCurveValue(p,p.x,a,u);
    constr->setTag(tagId);
    constr->setDriving(driving);
    addConstraint(constr);
    constr = new ConstraintCurveValue(p,p.y,a,u);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintArcOfHyperbolaRules(ArcOfHyperbola &a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start,a,a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end,a,a.endAngle, tagId, driving);
}

int System::addConstraintArcOfParabolaRules(ArcOfParabola &a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start,a,a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end,a,a.endAngle, tagId, driving);
}

int System::addConstraintPointOnArc(Point &p, Arc &a, int tagId, bool driving)
{
    return addConstraintP2PDistance(p, a.center, a.rad, tagId, driving);
}

int System::addConstraintPerpendicularLine2Arc(Point &p1, Point &p2, Arc &a,
                                               int tagId, bool driving)
{
    addConstraintP2PCoincident(p2, a.start, tagId, driving);
    double dx = *(p2.x) - *(p1.x);
    double dy = *(p2.y) - *(p1.y);
    if (dx * cos(*(a.startAngle)) + dy * sin(*(a.startAngle)) > 0)
        return addConstraintP2PAngle(p1, p2, a.startAngle, 0, tagId, driving);
    else
        return addConstraintP2PAngle(p1, p2, a.startAngle, M_PI, tagId, driving);
}

int System::addConstraintPerpendicularArc2Line(Arc &a, Point &p1, Point &p2,
                                               int tagId, bool driving)
{
    addConstraintP2PCoincident(p1, a.end, tagId, driving);
    double dx = *(p2.x) - *(p1.x);
    double dy = *(p2.y) - *(p1.y);
    if (dx * cos(*(a.endAngle)) + dy * sin(*(a.endAngle)) > 0)
        return addConstraintP2PAngle(p1, p2, a.endAngle, 0, tagId, driving);
    else
        return addConstraintP2PAngle(p1, p2, a.endAngle, M_PI, tagId, driving);
}

int System::addConstraintPerpendicularCircle2Arc(Point &center, double *radius,
                                                 Arc &a, int tagId, bool driving)
{
    addConstraintP2PDistance(a.start, center, radius, tagId, driving);
    double incrAngle = *(a.startAngle) < *(a.endAngle) ? M_PI/2 : -M_PI/2;
    double tangAngle = *a.startAngle + incrAngle;
    double dx = *(a.start.x) - *(center.x);
    double dy = *(a.start.y) - *(center.y);
    if (dx * cos(tangAngle) + dy * sin(tangAngle) > 0)
        return addConstraintP2PAngle(center, a.start, a.startAngle, incrAngle, tagId, driving);
    else
        return addConstraintP2PAngle(center, a.start, a.startAngle, -incrAngle, tagId, driving);
}

int System::addConstraintPerpendicularArc2Circle(Arc &a, Point &center,
                                                 double *radius, int tagId, bool driving)
{
    addConstraintP2PDistance(a.end, center, radius, tagId, driving);
    double incrAngle = *(a.startAngle) < *(a.endAngle) ? -M_PI/2 : M_PI/2;
    double tangAngle = *a.endAngle + incrAngle;
    double dx = *(a.end.x) - *(center.x);
    double dy = *(a.end.y) - *(center.y);
    if (dx * cos(tangAngle) + dy * sin(tangAngle) > 0)
        return addConstraintP2PAngle(center, a.end, a.endAngle, incrAngle, tagId, driving);
    else
        return addConstraintP2PAngle(center, a.end, a.endAngle, -incrAngle, tagId, driving);
}

int System::addConstraintPerpendicularArc2Arc(Arc &a1, bool reverse1,
                                              Arc &a2, bool reverse2, int tagId, bool driving)
{
    Point &p1 = reverse1 ? a1.start : a1.end;
    Point &p2 = reverse2 ? a2.end : a2.start;
    addConstraintP2PCoincident(p1, p2, tagId, driving);
    return addConstraintPerpendicular(a1.center, p1, a2.center, p2, tagId, driving);
}

int System::addConstraintTangent(Line &l, Circle &c, int tagId, bool driving)
{
    return addConstraintP2LDistance(c.center, l, c.rad, tagId, driving);
}

int System::addConstraintTangent(Line &l, Ellipse &e, int tagId, bool driving)
{
    Constraint *constr = new ConstraintEllipseTangentLine(l, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintTangent(Line &l, Arc &a, int tagId, bool driving)
{
    return addConstraintP2LDistance(a.center, l, a.rad, tagId, driving);
}

int System::addConstraintTangent(Circle &c1, Circle &c2, int tagId, bool driving)
{
    double dx = *(c2.center.x) - *(c1.center.x);
    double dy = *(c2.center.y) - *(c1.center.y);
    double d = sqrt(dx*dx + dy*dy);
    return addConstraintTangentCircumf(c1.center, c2.center, c1.rad, c2.rad,
                                       (d < *c1.rad || d < *c2.rad), tagId, driving);
}

int System::addConstraintTangent(Arc &a1, Arc &a2, int tagId, bool driving)
{
    double dx = *(a2.center.x) - *(a1.center.x);
    double dy = *(a2.center.y) - *(a1.center.y);
    double d = sqrt(dx*dx + dy*dy);
    return addConstraintTangentCircumf(a1.center, a2.center, a1.rad, a2.rad,
                                       (d < *a1.rad || d < *a2.rad), tagId, driving);
}

int System::addConstraintTangent(Circle &c, Arc &a, int tagId, bool driving)
{
    double dx = *(a.center.x) - *(c.center.x);
    double dy = *(a.center.y) - *(c.center.y);
    double d = sqrt(dx*dx + dy*dy);
    return addConstraintTangentCircumf(c.center, a.center, c.rad, a.rad,
                                       (d < *c.rad || d < *a.rad), tagId, driving);
}

int System::addConstraintCircleRadius(Circle &c, double *radius, int tagId, bool driving)
{
    return addConstraintEqual(c.rad, radius, tagId, driving);
}

int System::addConstraintArcRadius(Arc &a, double *radius, int tagId, bool driving)
{
    return addConstraintEqual(a.rad, radius, tagId, driving);
}

int System::addConstraintCircleDiameter(Circle &c, double *radius, int tagId, bool driving)
{
    return addConstraintProportional(c.rad, radius, 0.5, tagId, driving);
}

int System::addConstraintArcDiameter(Arc &a, double *radius, int tagId, bool driving)
{
    return addConstraintProportional(a.rad, radius, 0.5, tagId, driving);
}

int System::addConstraintEqualLength(Line &l1, Line &l2, double *length, int tagId, bool driving)
{
    addConstraintP2PDistance(l1.p1, l1.p2, length, tagId, driving);
    return addConstraintP2PDistance(l2.p1, l2.p2, length, tagId, driving);
}

int System::addConstraintEqualRadius(Circle &c1, Circle &c2, int tagId, bool driving)
{
    return addConstraintEqual(c1.rad, c2.rad, tagId, driving);
}

int System::addConstraintEqualRadii(Ellipse &e1, Ellipse &e2, int tagId, bool driving)
{
    addConstraintEqual(e1.radmin, e2.radmin, tagId, driving);

    Constraint *constr = new ConstraintEqualMajorAxesConic(&e1,&e2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualRadii(ArcOfHyperbola &a1, ArcOfHyperbola &a2, int tagId, bool driving)
{
    addConstraintEqual(a1.radmin, a2.radmin, tagId, driving);

    Constraint *constr = new ConstraintEqualMajorAxesConic(&a1,&a2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualFocus(ArcOfParabola &a1, ArcOfParabola &a2, int tagId, bool driving)
{
    Constraint *constr = new ConstraintEqualFocalDistance(&a1,&a2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualRadius(Circle &c1, Arc &a2, int tagId, bool driving)
{
    return addConstraintEqual(c1.rad, a2.rad, tagId, driving);
}

int System::addConstraintEqualRadius(Arc &a1, Arc &a2, int tagId, bool driving)
{
    return addConstraintEqual(a1.rad, a2.rad, tagId, driving);
}

int System::addConstraintP2PSymmetric(Point &p1, Point &p2, Line &l, int tagId, bool driving)
{
    addConstraintPerpendicular(p1, p2, l.p1, l.p2, tagId, driving);
    return addConstraintMidpointOnLine(p1, p2, l.p1, l.p2, tagId, driving);
}

int System::addConstraintP2PSymmetric(Point &p1, Point &p2, Point &p, int tagId, bool driving)
{
    addConstraintPointOnPerpBisector(p, p1, p2, tagId, driving);
    return addConstraintPointOnLine(p, p1, p2, tagId, driving);
}

int System::addConstraintSnellsLaw(Curve &ray1, Curve &ray2,
                                   Curve &boundary, Point p,
                                   double *n1, double *n2,
                                   bool flipn1, bool flipn2,
                                   int tagId, bool driving)
{
    Constraint *constr = new ConstraintSnell(ray1,ray2,boundary,p,n1,n2,flipn1,flipn2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentPoint2Ellipse(Ellipse &e, Point &p1, InternalAlignmentType alignmentType, int tagId, bool driving)
{
    Constraint *constr = new ConstraintInternalAlignmentPoint2Ellipse(e, p1, alignmentType);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentPoint2Hyperbola(Hyperbola &e, Point &p1, InternalAlignmentType alignmentType, int tagId, bool driving)
{
    Constraint *constr = new ConstraintInternalAlignmentPoint2Hyperbola(e, p1, alignmentType);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentEllipseMajorDiameter(Ellipse &e, Point &p1, Point &p2, int tagId, bool driving)
{
    double X_1=*p1.x;
    double Y_1=*p1.y;
    double X_2=*p2.x;
    double Y_2=*p2.y;
    double X_c=*e.center.x;
    double Y_c=*e.center.y;
    double X_F1=*e.focus1.x;
    double Y_F1=*e.focus1.y;
    double b=*e.radmin;

    // P1=vector([X_1,Y_1])
    // P2=vector([X_2,Y_2])
    // dF1= (F1-C)/sqrt((F1-C)*(F1-C))
    // print "these are the extreme points of the major axis"
    // PA = C + a * dF1
    // PN = C - a * dF1
    // print "this is a simple function to know which point is closer to the positive edge of the ellipse"
    // DMC=(P1-PA)*(P1-PA)-(P2-PA)*(P2-PA)
    double closertopositivemajor=pow(X_1 - X_c - (X_F1 - X_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c,
        2) + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
        2) - pow(X_2 - X_c - (X_F1 - X_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) +
        pow(Y_1 - Y_c - (Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) -
        pow(Y_2 - Y_c - (Y_F1 - Y_c)*sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2);

    if(closertopositivemajor>0){
        //p2 is closer to  positivemajor. Assign constraints back-to-front.
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipsePositiveMajorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipsePositiveMajorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseNegativeMajorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseNegativeMajorY,tagId, driving);
    }
    else{
        //p1 is closer to  positivemajor
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipsePositiveMajorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipsePositiveMajorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipseNegativeMajorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipseNegativeMajorY,tagId, driving);
    }
}

int System::addConstraintInternalAlignmentEllipseMinorDiameter(Ellipse &e, Point &p1, Point &p2, int tagId, bool driving)
{
    double X_1=*p1.x;
    double Y_1=*p1.y;
    double X_2=*p2.x;
    double Y_2=*p2.y;
    double X_c=*e.center.x;
    double Y_c=*e.center.y;
    double X_F1=*e.focus1.x;
    double Y_F1=*e.focus1.y;
    double b=*e.radmin;

    // Same idea as for major above, but for minor
    // DMC=(P1-PA)*(P1-PA)-(P2-PA)*(P2-PA)
    double closertopositiveminor= pow(X_1 - X_c + b*(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2)), 2) - pow(X_2 - X_c + b*(Y_F1 - Y_c)/sqrt(pow(X_F1 -
        X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) + pow(-Y_1 + Y_c + b*(X_F1 -
        X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) - pow(-Y_2 + Y_c
        + b*(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2);

    if(closertopositiveminor>0){
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipsePositiveMinorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipsePositiveMinorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseNegativeMinorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseNegativeMinorY,tagId, driving);
    } else {
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipsePositiveMinorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipsePositiveMinorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipseNegativeMinorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e,p2,EllipseNegativeMinorY,tagId, driving);
    }
}

int System::addConstraintInternalAlignmentEllipseFocus1(Ellipse &e, Point &p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving);
}

int System::addConstraintInternalAlignmentEllipseFocus2(Ellipse &e, Point &p1, int tagId, bool driving)
{
    addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseFocus2X,tagId, driving);
    return addConstraintInternalAlignmentPoint2Ellipse(e,p1,EllipseFocus2Y,tagId, driving);
}

int System::addConstraintInternalAlignmentHyperbolaMajorDiameter(Hyperbola &e, Point &p1, Point &p2, int tagId, bool driving)
{
    double X_1=*p1.x;
    double Y_1=*p1.y;
    double X_2=*p2.x;
    double Y_2=*p2.y;
    double X_c=*e.center.x;
    double Y_c=*e.center.y;
    double X_F1=*e.focus1.x;
    double Y_F1=*e.focus1.y;
    double b=*e.radmin;

    // P1=vector([X_1,Y_1])
    // P2=vector([X_2,Y_2])
    // dF1= (F1-C)/sqrt((F1-C)*(F1-C))
    // print "these are the extreme points of the major axis"
    // PA = C + a * dF1
    // PN = C - a * dF1
    // print "this is a simple function to know which point is closer to the positive edge of the ellipse"
    // DMC=(P1-PA)*(P1-PA)-(P2-PA)*(P2-PA)
    double closertopositivemajor= pow(-X_1 + X_c + (X_F1 - X_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2)
        + pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2)
        - pow(-X_2 + X_c + (X_F1 - X_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) +
        pow(-Y_1 + Y_c + (Y_F1 - Y_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) -
        pow(-Y_2 + Y_c + (Y_F1 - Y_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2);

    if(closertopositivemajor>0){
        //p2 is closer to  positivemajor. Assign constraints back-to-front.
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaPositiveMajorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaPositiveMajorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaNegativeMajorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaNegativeMajorY,tagId, driving);
    }
    else{
        //p1 is closer to  positivemajor
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaPositiveMajorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaPositiveMajorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaNegativeMajorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaNegativeMajorY,tagId, driving);
    }
}

int System::addConstraintInternalAlignmentHyperbolaMinorDiameter(Hyperbola &e, Point &p1, Point &p2, int tagId, bool driving)
{
    double X_1=*p1.x;
    double Y_1=*p1.y;
    double X_2=*p2.x;
    double Y_2=*p2.y;
    double X_c=*e.center.x;
    double Y_c=*e.center.y;
    double X_F1=*e.focus1.x;
    double Y_F1=*e.focus1.y;
    double b=*e.radmin;

    // Same idea as for major above, but for minor
    // DMC=(P1-PA)*(P1-PA)-(P2-PA)*(P2-PA)
    double closertopositiveminor= pow(-X_1 + X_c + b*(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2)) + (X_F1 - X_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) +
        pow(Y_F1 - Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) -
        pow(-X_2 + X_c + b*(Y_F1 - Y_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 -
        Y_c, 2)) + (X_F1 - X_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 -
        Y_c, 2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) + pow(-Y_1 +
        Y_c - b*(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) +
        (Y_F1 - Y_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
        2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2) - pow(-Y_2 + Y_c -
        b*(X_F1 - X_c)/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)) + (Y_F1 -
        Y_c)*(-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c,
        2))/sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2);

    if(closertopositiveminor<0){
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaPositiveMinorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaPositiveMinorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaNegativeMinorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaNegativeMinorY,tagId, driving);
    } else {
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaPositiveMinorX,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p1,HyperbolaPositiveMinorY,tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaNegativeMinorX,tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e,p2,HyperbolaNegativeMinorY,tagId, driving);
    }
}

int System::addConstraintInternalAlignmentHyperbolaFocus(Hyperbola &e, Point &p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving);
}

int System::addConstraintInternalAlignmentParabolaFocus(Parabola &e, Point &p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving);
}

int System::addConstraintInternalAlignmentBSplineControlPoint(BSpline &b, Circle &c, int poleindex, int tagId, bool driving)
{
    addConstraintEqual(b.poles[poleindex].x, c.center.x, tagId, driving);
    addConstraintEqual(b.poles[poleindex].y, c.center.y, tagId, driving);
    return addConstraintEqual(b.weights[poleindex], c.rad, tagId, driving);
}

//calculates angle between two curves at point of their intersection p. If two
//points are supplied, p is used for first curve and p2 for second, yielding a
//remote angle computation (this is useful when the endpoints haven't) been
//made coincident yet
double System::calculateAngleViaPoint(Curve &crv1, Curve &crv2, Point &p)
{
    return calculateAngleViaPoint(crv1, crv2, p, p);
}

double System::calculateAngleViaPoint(Curve &crv1, Curve &crv2, Point &p1, Point &p2)
{
    GCS::DeriVector2 n1 = crv1.CalculateNormal(p1);
    GCS::DeriVector2 n2 = crv2.CalculateNormal(p2);
    return atan2(-n2.x*n1.y+n2.y*n1.x, n2.x*n1.x + n2.y*n1.y);
}

void System::calculateNormalAtPoint(Curve &crv, Point &p, double &rtnX, double &rtnY)
{
    GCS::DeriVector2 n1 = crv.CalculateNormal(p);
    rtnX = n1.x;
    rtnY = n1.y;
}

double System::calculateConstraintErrorByTag(int tagId)
{
    int cnt = 0; //how many constraints have been accumulated
    double sqErr = 0.0; //accumulator of squared errors
    double err = 0.0;//last computed signed error value

    for (std::vector<Constraint *>::const_iterator
         constr=clist.begin(); constr != clist.end(); ++constr) {
        if ((*constr)->getTag() == tagId){
            err = (*constr)->error();
            sqErr += err*err;
            cnt++;
        };
    }
    switch (cnt) {
        case 0: //constraint not found!
            return std::numeric_limits<double>::quiet_NaN();
        break;
        case 1:
            return err;
        break;
        default:
            return sqrt(sqErr/(double)cnt);
    }

}

void System::rescaleConstraint(int id, double coeff)
{
    if (id >= static_cast<int>(clist.size()) || id < 0)
        return;
    if (clist[id])
        clist[id]->rescale(coeff);
}

void System::declareUnknowns(VEC_pD &params)
{
    plist = params;
    pIndex.clear();
    for (int i=0; i < int(plist.size()); ++i)
        pIndex[plist[i]] = i;
    hasUnknowns = true;
}

void System::declareDrivenParams(VEC_pD &params)
{
    pdrivenlist = params;
}


void System::initSolution(Algorithm alg)
{
    // - Stores the current parameters values in the vector "reference"
    // - identifies any decoupled subsystems and partitions the original
    //   system into corresponding components
    // - Stores the current parameters in the vector "reference"
    // - Identifies the equality constraints tagged with ids >= 0
    //   and prepares a corresponding system reduction
    // - Organizes the rest of constraints into two subsystems for
    //   tag ids >=0 and < 0 respectively and applies the
    //   system reduction specified in the previous step

    isInit = false;
    if (!hasUnknowns)
        return;

    // storing reference configuration
    setReference();

    // diagnose conflicting or redundant constraints
    if (!hasDiagnosis) {
        diagnose(alg);
        if (!hasDiagnosis)
            return;
    }
    std::vector<Constraint *> clistR;
    if (redundant.size()) {
        for (std::vector<Constraint *>::const_iterator constr=clist.begin(); constr != clist.end(); ++constr) {
            if (redundant.count(*constr) == 0)
                clistR.push_back(*constr);
        }
    }
    else
        clistR = clist;

    // partitioning into decoupled components
    Graph g;
    for (int i=0; i < int(plist.size() + clistR.size()); i++)
        boost::add_vertex(g);

    int cvtid = int(plist.size());
    for (std::vector<Constraint *>::const_iterator constr=clistR.begin();
         constr != clistR.end(); ++constr, cvtid++) {
        VEC_pD &cparams = c2p[*constr];
        for (VEC_pD::const_iterator param=cparams.begin();
             param != cparams.end(); ++param) {
            MAP_pD_I::const_iterator it = pIndex.find(*param);
            if (it != pIndex.end())
                boost::add_edge(cvtid, it->second, g);
        }
    }

    VEC_I components(boost::num_vertices(g));
    int componentsSize = 0;
    if (!components.empty())
        componentsSize = boost::connected_components(g, &components[0]);

    // identification of equality constraints and parameter reduction
    std::set<Constraint *> reducedConstrs;  // constraints that will be eliminated through reduction
    reductionmaps.clear(); // destroy any maps
    reductionmaps.resize(componentsSize); // create empty maps to be filled in
    {
        VEC_pD reducedParams=plist;

        for (std::vector<Constraint *>::const_iterator constr=clistR.begin();
            constr != clistR.end(); ++constr) {
            if ((*constr)->getTag() >= 0 && (*constr)->getTypeId() == Equal) {
                MAP_pD_I::const_iterator it1,it2;
                it1 = pIndex.find((*constr)->params()[0]);
                it2 = pIndex.find((*constr)->params()[1]);
                if (it1 != pIndex.end() && it2 != pIndex.end()) {
                    reducedConstrs.insert(*constr);
                    double *p_kept = reducedParams[it1->second];
                    double *p_replaced = reducedParams[it2->second];
                    for (int i=0; i < int(plist.size()); ++i) {
                       if (reducedParams[i] == p_replaced)
                           reducedParams[i] = p_kept;
                    }
                }
            }
        }
        for (int i=0; i < int(plist.size()); ++i)
            if (plist[i] != reducedParams[i]) {
                int cid = components[i];
                reductionmaps[cid][plist[i]] = reducedParams[i];
            }
    }

    clists.clear(); // destroy any lists
    clists.resize(componentsSize); // create empty lists to be filled in
    int i = int(plist.size());
    for (std::vector<Constraint *>::const_iterator constr=clistR.begin();
         constr != clistR.end(); ++constr, i++) {
        if (reducedConstrs.count(*constr) == 0) {
            int cid = components[i];
            clists[cid].push_back(*constr);
        }
    }

    plists.clear(); // destroy any lists
    plists.resize(componentsSize); // create empty lists to be filled in
    for (int i=0; i < int(plist.size()); ++i) {
        int cid = components[i];
        plists[cid].push_back(plist[i]);
    }

    // calculates subSystems and subSystemsAux from clists, plists and reductionmaps
    clearSubSystems();
    for (std::size_t cid=0; cid < clists.size(); cid++) {
        std::vector<Constraint *> clist0, clist1;
        for (std::vector<Constraint *>::const_iterator constr=clists[cid].begin();
             constr != clists[cid].end(); ++constr) {
            if ((*constr)->getTag() >= 0)
                clist0.push_back(*constr);
            else // move or distance from reference constraints
                clist1.push_back(*constr);
        }

        subSystems.push_back(NULL);
        subSystemsAux.push_back(NULL);
        if (clist0.size() > 0)
            subSystems[cid] = new SubSystem(clist0, plists[cid], reductionmaps[cid]);
        if (clist1.size() > 0)
            subSystemsAux[cid] = new SubSystem(clist1, plists[cid], reductionmaps[cid]);
    }

    isInit = true;
}

void System::setReference()
{
    reference.clear();
    reference.reserve(plist.size());
    for (VEC_pD::const_iterator param=plist.begin();
         param != plist.end(); ++param)
        reference.push_back(**param);
}

void System::resetToReference()
{
    if (reference.size() == plist.size()) {
        VEC_D::const_iterator ref=reference.begin();
        VEC_pD::iterator param=plist.begin();
        for (; ref != reference.end(); ++ref, ++param)
            **param = *ref;
    }
}

int System::solve(VEC_pD &params, bool isFine, Algorithm alg, bool isRedundantsolving)
{
    declareUnknowns(params);
    initSolution();
    return solve(isFine, alg, isRedundantsolving);
}

int System::solve(bool isFine, Algorithm alg, bool isRedundantsolving)
{
    if (!isInit)
        return Failed;

    bool isReset = false;
    // return success by default in order to permit coincidence constraints to be applied
    // even if no other system has to be solved
    int res = Success;
    for (int cid=0; cid < int(subSystems.size()); cid++) {
        if ((subSystems[cid] || subSystemsAux[cid]) && !isReset) {
             resetToReference();
             isReset = true;
        }
        if (subSystems[cid] && subSystemsAux[cid])
            res = std::max(res, solve(subSystems[cid], subSystemsAux[cid], isFine, isRedundantsolving));
        else if (subSystems[cid])
            res = std::max(res, solve(subSystems[cid], isFine, alg, isRedundantsolving));
        else if (subSystemsAux[cid])
            res = std::max(res, solve(subSystemsAux[cid], isFine, alg, isRedundantsolving));
    }
    if (res == Success) {
        for (std::set<Constraint *>::const_iterator constr=redundant.begin();
             constr != redundant.end(); ++constr){
            //DeepSOIC: there used to be a comparison of signed error value to
            //convergence, which makes no sense. Potentially I fixed bug, and
            //chances are low I've broken anything.
            double err = (*constr)->error();
            if (err*err > (isRedundantsolving?convergenceRedundant:convergence)) {
                res = Converged;
                return res;
            }
        }
    }
    return res;
}

int System::solve(SubSystem *subsys, bool isFine, Algorithm alg, bool isRedundantsolving)
{
    if (alg == BFGS)
        return solve_BFGS(subsys, isFine, isRedundantsolving);
    else if (alg == LevenbergMarquardt)
        return solve_LM(subsys, isRedundantsolving);
    else if (alg == DogLeg)
        return solve_DL(subsys, isRedundantsolving);
    else
        return Failed;
}

int System::solve_BFGS(SubSystem *subsys, bool /*isFine*/, bool isRedundantsolving)
{
    #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
    #endif

    int xsize = subsys->pSize();
    if (xsize == 0)
        return Success;

    subsys->redirectParams();

    Eigen::MatrixXd D = Eigen::MatrixXd::Identity(xsize, xsize);
    Eigen::VectorXd x(xsize);
    Eigen::VectorXd xdir(xsize);
    Eigen::VectorXd grad(xsize);
    Eigen::VectorXd h(xsize);
    Eigen::VectorXd y(xsize);
    Eigen::VectorXd Dy(xsize);

    // Initial unknowns vector and initial gradient vector
    subsys->getParams(x);
    subsys->calcGrad(grad);

    // Initial search direction opposed to gradient (steepest-descent)
    xdir = -grad;
    lineSearch(subsys, xdir);
    double err = subsys->error();

    h = x;
    subsys->getParams(x);
    h = x - h; // = x - xold

    //double convergence = isFine ? convergence : XconvergenceRough;
    int maxIterNumber = (isRedundantsolving?
        (sketchSizeMultiplierRedundant?maxIterRedundant * xsize:maxIterRedundant):
        (sketchSizeMultiplier?maxIter * xsize:maxIter));

    if(debugMode==IterationLevel) {
        std::stringstream stream;
        stream  << "BFGS: convergence: "    << (isRedundantsolving?convergenceRedundant:convergence)
                << ", xsize: "              << xsize
                << ", maxIter: "            << maxIterNumber  << "\n";

        const std::string tmp = stream.str();
        Base::Console().Log(tmp.c_str());
    }

    double divergingLim = 1e6*err + 1e12;
    double h_norm;

    for (int iter=1; iter < maxIterNumber; iter++) {
        h_norm = h.norm();
        if (h_norm <= (isRedundantsolving?convergenceRedundant:convergence) || err <= smallF){
           if(debugMode==IterationLevel) {
                std::stringstream stream;
                stream  << "BFGS Converged!!: "
                        << ", err: "              << err
                        << ", h_norm: "           << h_norm  << "\n";

                const std::string tmp = stream.str();
                Base::Console().Log(tmp.c_str());
            }
            break;
        }
        if (err > divergingLim || err != err) { // check for diverging and NaN
            if(debugMode==IterationLevel) {
                std::stringstream stream;
                stream  << "BFGS Failed: Diverging!!: "
                        << ", err: "              << err
                        << ", divergingLim: "            << divergingLim  << "\n";

                const std::string tmp = stream.str();
                Base::Console().Log(tmp.c_str());
            }
            break;
        }

        y = grad;
        subsys->calcGrad(grad);
        y = grad - y; // = grad - gradold

        double hty = h.dot(y);
        //make sure that hty is never 0
        if (hty == 0)
            hty = .0000000001;

        Dy = D * y;

        double ytDy = y.dot(Dy);

        //Now calculate the BFGS update on D
        D += (1.+ytDy/hty)/hty * h * h.transpose();
        D -= 1./hty * (h * Dy.transpose() + Dy * h.transpose());

        xdir = -D * grad;
        lineSearch(subsys, xdir);
        err = subsys->error();

        h = x;
        subsys->getParams(x);
        h = x - h; // = x - xold

        if(debugMode==IterationLevel) {
            std::stringstream stream;
            stream  << "BFGS, Iteration: "          << iter
                    << ", err: "                    << err
                    << ", h_norm: "                 << h_norm << "\n";

            const std::string tmp = stream.str();
            Base::Console().Log(tmp.c_str());
        }
    }

    subsys->revertParams();

    if (err <= smallF)
        return Success;
    if (h.norm() <= (isRedundantsolving?convergenceRedundant:convergence))
        return Converged;
    return Failed;
}

int System::solve_LM(SubSystem* subsys, bool isRedundantsolving)
{
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
#endif

    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    if (xsize == 0)
        return Success;

    Eigen::VectorXd e(csize), e_new(csize); // vector of all function errors (every constraint is one function)
    Eigen::MatrixXd J(csize, xsize);        // Jacobi of the subsystem
    Eigen::MatrixXd A(xsize, xsize);
    Eigen::VectorXd x(xsize), h(xsize), x_new(xsize), g(xsize), diag_A(xsize);

    subsys->redirectParams();

    subsys->getParams(x);
    subsys->calcResidual(e);
    e*=-1;

    int maxIterNumber = (isRedundantsolving?
        (sketchSizeMultiplierRedundant?maxIterRedundant * xsize:maxIterRedundant):
        (sketchSizeMultiplier?maxIter * xsize:maxIter));

    double divergingLim = 1e6*e.squaredNorm() + 1e12;

    double eps=(isRedundantsolving?LM_epsRedundant:LM_eps);
    double eps1=(isRedundantsolving?LM_eps1Redundant:LM_eps1);
    double tau=(isRedundantsolving?LM_tauRedundant:LM_tau);

    if(debugMode==IterationLevel) {
        std::stringstream stream;
        stream  << "LM: eps: "          << eps
                << ", eps1: "           << eps1
                << ", tau: "            << tau
                << ", convergence: "    << (isRedundantsolving?convergenceRedundant:convergence)
                << ", xsize: "          << xsize
                << ", maxIter: "        << maxIterNumber  << "\n";

        const std::string tmp = stream.str();
        Base::Console().Log(tmp.c_str());
    }

    double nu=2, mu=0;
    int iter=0, stop=0;
    for (iter=0; iter < maxIterNumber && !stop; ++iter) {

        // check error
        double err=e.squaredNorm();
        if (err <= eps) { // error is small, Success
            stop = 1;
            break;
        }
        else if (err > divergingLim || err != err) { // check for diverging and NaN
            stop = 6;
            break;
        }

        // J^T J, J^T e
        subsys->calcJacobi(J);;

        A = J.transpose()*J;
        g = J.transpose()*e;

        // Compute ||J^T e||_inf
        double g_inf = g.lpNorm<Eigen::Infinity>();
        diag_A = A.diagonal(); // save diagonal entries so that augmentation can be later canceled

        // check for convergence
        if (g_inf <= eps1) {
            stop = 2;
            break;
        }

        // compute initial damping factor
        if (iter == 0)
            mu = tau * diag_A.lpNorm<Eigen::Infinity>();

        double h_norm;
        // determine increment using adaptive damping
        int k=0;
        while (k < 50) {
            // augment normal equations A = A+uI
            for (int i=0; i < xsize; ++i)
                A(i,i) += mu;

            //solve augmented functions A*h=-g
            h = A.fullPivLu().solve(g);
            double rel_error = (A*h - g).norm() / g.norm();

            // check if solving works
            if (rel_error < 1e-5) {

                // restrict h according to maxStep
                double scale = subsys->maxStep(h);
                if (scale < 1.)
                    h *= scale;

                // compute par's new estimate and ||d_par||^2
                x_new = x + h;
                h_norm = h.squaredNorm();

                if (h_norm <= eps1*eps1*x.norm()) { // relative change in p is small, stop
                    stop = 3;
                    break;
                }
                else if (h_norm >= (x.norm()+eps1)/(DBL_EPSILON*DBL_EPSILON)) { // almost singular
                    stop = 4;
                    break;
                }

                subsys->setParams(x_new);
                subsys->calcResidual(e_new);
                e_new *= -1;

                double dF = e.squaredNorm() - e_new.squaredNorm();
                double dL = h.dot(mu*h+g);

                if (dF>0. && dL>0.) { // reduction in error, increment is accepted
                    double tmp=2*dF/dL-1.;
                    mu *= std::max(1./3., 1.-tmp*tmp*tmp);
                    nu=2;

                    // update par's estimate
                    x = x_new;
                    e = e_new;
                    break;
                }
            }

            // if this point is reached, either the linear system could not be solved or
            // the error did not reduce; in any case, the increment must be rejected

            mu*=nu;
            nu*=2.0;
            for (int i=0; i < xsize; ++i) // restore diagonal J^T J entries
                A(i,i) = diag_A(i);

            k++;
        }
        if (k > 50) {
            stop = 7;
            break;
        }

        if(debugMode==IterationLevel) {
            std::stringstream stream;
            // Iteration: 1, residual: 1e-3, tolg: 1e-5, tolx: 1e-3
            stream  << "LM, Iteration: "            << iter
                    << ", err(eps): "               << err
                    << ", g_inf(eps1): "            << g_inf
                    << ", h_norm: "                 << h_norm << "\n";

            const std::string tmp = stream.str();
            Base::Console().Log(tmp.c_str());
        }
    }

    if (iter >= maxIterNumber)
        stop = 5;

    subsys->revertParams();

    return (stop == 1) ? Success : Failed;
}


int System::solve_DL(SubSystem* subsys, bool isRedundantsolving)
{
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
#endif

    double tolg=(isRedundantsolving?DL_tolgRedundant:DL_tolg);
    double tolx=(isRedundantsolving?DL_tolxRedundant:DL_tolx);
    double tolf=(isRedundantsolving?DL_tolfRedundant:DL_tolf);

    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    if (xsize == 0)
        return Success;

    int maxIterNumber = (isRedundantsolving?
        (sketchSizeMultiplierRedundant?maxIterRedundant * xsize:maxIterRedundant):
        (sketchSizeMultiplier?maxIter * xsize:maxIter));

    if(debugMode==IterationLevel) {
        std::stringstream stream;
        stream  << "DL: tolg: "         << tolg
                << ", tolx: "           << tolx
                << ", tolf: "           << tolf
                << ", convergence: "    << (isRedundantsolving?convergenceRedundant:convergence)
                << ", dogLegGaussStep: " << (dogLegGaussStep==FullPivLU?"FullPivLU":(dogLegGaussStep==LeastNormFullPivLU?"LeastNormFullPivLU":"LeastNormLdlt"))
                << ", xsize: "          << xsize
                << ", csize: "          << csize
                << ", maxIter: "        << maxIterNumber  << "\n";

        const std::string tmp = stream.str();
        Base::Console().Log(tmp.c_str());
    }

    Eigen::VectorXd x(xsize), x_new(xsize);
    Eigen::VectorXd fx(csize), fx_new(csize);
    Eigen::MatrixXd Jx(csize, xsize), Jx_new(csize, xsize);
    Eigen::VectorXd g(xsize), h_sd(xsize), h_gn(xsize), h_dl(xsize);

    subsys->redirectParams();

    double err;
    subsys->getParams(x);
    subsys->calcResidual(fx, err);
    subsys->calcJacobi(Jx);

    g = Jx.transpose()*(-fx);

    // get the infinity norm fx_inf and g_inf
    double g_inf = g.lpNorm<Eigen::Infinity>();
    double fx_inf = fx.lpNorm<Eigen::Infinity>();

    double divergingLim = 1e6*err + 1e12;

    double delta=0.1;
    double alpha=0.;
    double nu=2.;
    int iter=0, stop=0, reduce=0;
    while (!stop) {

        // check if finished
        if (fx_inf <= tolf) // Success
            stop = 1;
        else if (g_inf <= tolg)
            stop = 2;
        else if (delta <= tolx*(tolx + x.norm()))
            stop = 2;
        else if (iter >= maxIterNumber)
            stop = 4;
        else if (err > divergingLim || err != err) { // check for diverging and NaN
            stop = 6;
        }
        else {
            // get the steepest descent direction
            alpha = g.squaredNorm()/(Jx*g).squaredNorm();
            h_sd  = alpha*g;

            // get the gauss-newton step
            // http://forum.freecadweb.org/viewtopic.php?f=10&t=12769&start=50#p106220
            // https://forum.kde.org/viewtopic.php?f=74&t=129439#p346104
            switch (dogLegGaussStep){
                case FullPivLU:
                    h_gn = Jx.fullPivLu().solve(-fx);
                    break;
                case LeastNormFullPivLU:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).fullPivLu().solve(-fx);
                    break;
                case LeastNormLdlt:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).ldlt().solve(-fx);
                    break;
            }

            double rel_error = (Jx*h_gn + fx).norm() / fx.norm();
            if (rel_error > 1e15)
                break;

            // compute the dogleg step
            if (h_gn.norm() < delta) {
                h_dl = h_gn;
                if  (h_dl.norm() <= tolx*(tolx + x.norm())) {
                    stop = 5;
                    break;
                }
            }
            else if (alpha*g.norm() >= delta) {
                h_dl = (delta/(alpha*g.norm()))*h_sd;
            }
            else {
                //compute beta
                double beta = 0;
                Eigen::VectorXd b = h_gn - h_sd;
                double bb = (b.transpose()*b).norm();
                double gb = (h_sd.transpose()*b).norm();
                double c = (delta + h_sd.norm())*(delta - h_sd.norm());

                if (gb > 0)
                    beta = c / (gb + sqrt(gb * gb + c * bb));
                else
                    beta = (sqrt(gb * gb + c * bb) - gb)/bb;

                // and update h_dl and dL with beta
                h_dl = h_sd + beta*b;
            }
        }

        // see if we are already finished
        if (stop)
            break;

// it didn't work in some tests
//        // restrict h_dl according to maxStep
//        double scale = subsys->maxStep(h_dl);
//        if (scale < 1.)
//            h_dl *= scale;

        // get the new values
        double err_new;
        x_new = x + h_dl;
        subsys->setParams(x_new);
        subsys->calcResidual(fx_new, err_new);
        subsys->calcJacobi(Jx_new);

        // calculate the linear model and the update ratio
        double dL = err - 0.5*(fx + Jx*h_dl).squaredNorm();
        double dF = err - err_new;
        double rho = dL/dF;

        if (dF > 0 && dL > 0) {
            x  = x_new;
            Jx = Jx_new;
            fx = fx_new;
            err = err_new;

            g = Jx.transpose()*(-fx);

            // get infinity norms
            g_inf = g.lpNorm<Eigen::Infinity>();
            fx_inf = fx.lpNorm<Eigen::Infinity>();
        }
        else
            rho = -1;

        // update delta
        if (fabs(rho-1.) < 0.2 && h_dl.norm() > delta/3. && reduce <= 0) {
            delta = 3*delta;
            nu = 2;
            reduce = 0;
        }
        else if (rho < 0.25) {
            delta = delta/nu;
            nu = 2*nu;
            reduce = 2;
        }
        else
            reduce--;

        if(debugMode==IterationLevel) {
            std::stringstream stream;
            // Iteration: 1, residual: 1e-3, tolg: 1e-5, tolx: 1e-3
            stream  << "DL, Iteration: "        << iter
                    << ", fx_inf(tolf): "       << fx_inf
                    << ", g_inf(tolg): "        << g_inf
                    << ", delta(f(tolx)): "     << delta
                    << ", err(divergingLim): "  << err  << "\n";

            const std::string tmp = stream.str();
            Base::Console().Log(tmp.c_str());
        }

        // count this iteration and start again
        iter++;
    }

    subsys->revertParams();

    if(debugMode==IterationLevel) {
        std::stringstream stream;
        stream  << "DL: stopcode: "     << stop << ((stop == 1) ? ", Success" : ", Failed") << "\n";

        const std::string tmp = stream.str();
        Base::Console().Log(tmp.c_str());
    }

    return (stop == 1) ? Success : Failed;
}

#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
void System::extractSubsystem(SubSystem *subsys, bool isRedundantsolving)
{
    VEC_pD plistout; //std::vector<double *>
    std::vector<Constraint *> clist_;
    VEC_pD clist_params_;

    subsys->getParamList(plistout);
    subsys->getConstraintList(clist_);

    std::ofstream subsystemfile;
    subsystemfile.open("subsystemfile.txt", std::ofstream::out | std::ofstream::app);
    subsystemfile <<  std::endl;
    subsystemfile <<  std::endl;
    subsystemfile << "Solving: " << (isRedundantsolving?"Redundant":"Normal") << " Subsystem Dump starts here................................" << std::endl;

    int ip=0;

    subsystemfile << "GCS::VEC_pD plist_;" <<  std::endl; // all SYSTEM params
    subsystemfile << "std::vector<GCS::Constraint *> clist_;" <<  std::endl; // SUBSYSTEM constraints
    subsystemfile << "GCS::VEC_pD plistsub_;" <<  std::endl; // all SUBSYSTEM params
    subsystemfile << "GCS::VEC_pD clist_params_;" <<  std::endl; // constraint params not within SYSTEM params

    // these are all the parameters, including those not in the subsystem to
    // which constraints in the subsystem may make reference.
    for( VEC_pD::iterator it = plist.begin(); it != plist.end(); ++it, ++ip) {
        subsystemfile << "plist_.push_back(new double("<< *(*it) <<")); // "<< ip <<" address: " << (void *)(*it) << std::endl;
    }

    int ips=0;
    for( VEC_pD::iterator it = plistout.begin(); it != plistout.end(); ++it, ++ips) {
        VEC_pD::iterator p = std::find(plist.begin(), plist.end(),(*it));
        size_t p_index = std::distance(plist.begin(), p);

        if(p_index == plist.size()) {
            subsystemfile << "// Error: Subsystem parameter not in system params" << "address: " << (void *)(*it)  << std::endl;
        }

        subsystemfile << "plistsub_.push_back(plist_[" << p_index <<"]); // "<< ips << std::endl;
    }

    int ic=0; // constraint index
    int icp=0; // index of constraint params not within SYSTEM params
    for (std::vector<Constraint *>::iterator it = clist_.begin(); it != clist_.end(); ++it,++ic) {

        switch((*it)->getTypeId())
        {
            case Equal:
            { // 2
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                subsystemfile << "clist_.push_back(new ConstraintEqual(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) << "]," \
                << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) << "])); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << std::endl;
                break;
            }
            case Difference:
            { // 3
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                subsystemfile << "clist_.push_back(new ConstraintDifference(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) << "]," \
                << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) << "]," \
                << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) << "])); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << std::endl;
                break;
            }
            case P2PDistance:
            { // 5
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                subsystemfile << "ConstraintP2PDistance * c" << ic <<"=new ConstraintP2PDistance();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << std::endl;
                break;
            }
            case P2PAngle:
            { // 5
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                subsystemfile << "ConstraintP2PAngle * c" << ic <<"=new ConstraintP2PAngle();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << std::endl;
                break;
            }
            case P2LDistance:
            { // 7
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                subsystemfile << "ConstraintP2LDistance * c" << ic <<"=new ConstraintP2LDistance();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << std::endl;
                break;
            }
            case PointOnLine:
            { // 6
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                subsystemfile << "ConstraintPointOnLine * c" << ic <<"=new ConstraintPointOnLine();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case PointOnPerpBisector:
            { // 6
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                subsystemfile << "ConstraintPointOnPerpBisector * c" << ic <<"=new ConstraintPointOnPerpBisector();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case Parallel:
            { // 8
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                VEC_pD::iterator p8 = std::find(plist.begin(), plist.end(),(*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                bool npb8=false;
                VEC_pD::iterator np8 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if(i8 == plist.size()) {
                    if(ni8 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[7]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[7] << std::endl;
                        icp++;
                    }
                    npb8=true;
                }

                subsystemfile << "ConstraintParallel * c" << ic <<"=new ConstraintParallel();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb8?("clist_params_["):("plist_[")) << (npb8?ni8:i8) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case Perpendicular:
            { // 8
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                VEC_pD::iterator p8 = std::find(plist.begin(), plist.end(),(*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                bool npb8=false;
                VEC_pD::iterator np8 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if(i8 == plist.size()) {
                    if(ni8 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[7]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[7] << std::endl;
                        icp++;
                    }
                    npb8=true;
                }

                subsystemfile << "ConstraintPerpendicular * c" << ic <<"=new ConstraintPerpendicular();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb8?("clist_params_["):("plist_[")) << (npb8?ni8:i8) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case L2LAngle:
            { // 9
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                VEC_pD::iterator p8 = std::find(plist.begin(), plist.end(),(*it)->pvec[7]);
                VEC_pD::iterator p9 = std::find(plist.begin(), plist.end(),(*it)->pvec[8]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);
                size_t i9 = std::distance(plist.begin(), p9);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                bool npb8=false;
                VEC_pD::iterator np8 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if(i8 == plist.size()) {
                    if(ni8 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[7]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[7] << std::endl;
                        icp++;
                    }
                    npb8=true;
                }

                bool npb9=false;
                VEC_pD::iterator np9 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[8]);
                size_t ni9 = std::distance(clist_params_.begin(), np9);

                if(i9 == plist.size()) {
                    if(ni9 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[8]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[8]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[8] << std::endl;
                        icp++;
                    }
                    npb9=true;
                }

                subsystemfile << "ConstraintL2LAngle * c" << ic <<"=new ConstraintL2LAngle();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb8?("clist_params_["):("plist_[")) << (npb8?ni8:i8) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb9?("clist_params_["):("plist_[")) << (npb9?ni9:i9) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << "," << (*it)->pvec[8] << std::endl;
                break;
            }
            case MidpointOnLine:
            { // 8
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                VEC_pD::iterator p8 = std::find(plist.begin(), plist.end(),(*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                bool npb8=false;
                VEC_pD::iterator np8 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if(i8 == plist.size()) {
                    if(ni8 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[7]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[7] << std::endl;
                        icp++;
                    }
                    npb8=true;
                }

                subsystemfile << "ConstraintMidpointOnLine * c" << ic <<"=new ConstraintMidpointOnLine();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb8?("clist_params_["):("plist_[")) << (npb8?ni8:i8) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case TangentCircumf:
            { // 6
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                subsystemfile << "ConstraintTangentCircumf * c" << ic <<"=new ConstraintTangentCircumf(" << (static_cast<ConstraintTangentCircumf *>(*it)->getInternal()?"true":"false") <<");" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case PointOnEllipse:
            { // 7
                VEC_pD::iterator p1 = std::find(plist.begin(), plist.end(),(*it)->pvec[0]);
                VEC_pD::iterator p2 = std::find(plist.begin(), plist.end(),(*it)->pvec[1]);
                VEC_pD::iterator p3 = std::find(plist.begin(), plist.end(),(*it)->pvec[2]);
                VEC_pD::iterator p4 = std::find(plist.begin(), plist.end(),(*it)->pvec[3]);
                VEC_pD::iterator p5 = std::find(plist.begin(), plist.end(),(*it)->pvec[4]);
                VEC_pD::iterator p6 = std::find(plist.begin(), plist.end(),(*it)->pvec[5]);
                VEC_pD::iterator p7 = std::find(plist.begin(), plist.end(),(*it)->pvec[6]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);

                bool npb1=false;
                VEC_pD::iterator np1 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if(i1 == plist.size()) {
                    if(ni1 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[0]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[0] << std::endl;
                        icp++;
                    }
                    npb1=true;
                }

                bool npb2=false;
                VEC_pD::iterator np2 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if(i2 == plist.size()) {
                    if(ni2 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[1]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[1] << std::endl;
                        icp++;
                    }
                    npb2=true;
                }

                bool npb3=false;
                VEC_pD::iterator np3 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if(i3 == plist.size()) {
                    if(ni3 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[2]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[2] << std::endl;
                        icp++;
                    }
                    npb3=true;
                }

                bool npb4=false;
                VEC_pD::iterator np4 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if(i4 == plist.size()) {
                    if(ni4 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[3]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[3] << std::endl;
                        icp++;
                    }
                    npb4=true;
                }

                bool npb5=false;
                VEC_pD::iterator np5 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if(i5 == plist.size()) {
                    if(ni5 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[4]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[4] << std::endl;
                        icp++;
                    }
                    npb5=true;
                }

                bool npb6=false;
                VEC_pD::iterator np6 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if(i6 == plist.size()) {
                    if(ni6 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[5]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[5] << std::endl;
                        icp++;
                    }
                    npb6=true;
                }

                bool npb7=false;
                VEC_pD::iterator np7 = std::find(clist_params_.begin(), clist_params_.end(),(*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if(i7 == plist.size()) {
                    if(ni7 == clist_params_.size()) {
                        subsystemfile << "// Address not in System params...rebuilding into clist_params_" << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double("<< *((*it)->pvec[6]) <<")); // "<< icp <<" address: " << (void *)(*it)->pvec[6] << std::endl;
                        icp++;
                    }
                    npb7=true;
                }

                subsystemfile << "ConstraintPointOnEllipse * c" << ic <<"=new ConstraintPointOnEllipse();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb1?("clist_params_["):("plist_[")) << (npb1?ni1:i1) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb2?("clist_params_["):("plist_[")) << (npb2?ni2:i2) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb3?("clist_params_["):("plist_[")) << (npb3?ni3:i3) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb4?("clist_params_["):("plist_[")) << (npb4?ni4:i4) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb5?("clist_params_["):("plist_[")) << (npb5?ni5:i5) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb6?("clist_params_["):("plist_[")) << (npb6?ni6:i6) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back(" << (npb7?("clist_params_["):("plist_[")) << (npb7?ni7:i7) <<"]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = "<< (*it)->pvec[0] << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6] << std::endl;
                break;
            }
            CASE_NOT_IMP(TangentEllipseLine)
            CASE_NOT_IMP(InternalAlignmentPoint2Ellipse)
            CASE_NOT_IMP(EqualMajorAxesEllipse)
            CASE_NOT_IMP(EllipticalArcRangeToEndPoints)
            CASE_NOT_IMP(AngleViaPoint)
            CASE_NOT_IMP(Snell)
            CASE_NOT_IMP(None)
        }
    }

    subsystemfile.close();
}
#endif

// The following solver variant solves a system compound of two subsystems
// treating the first of them as of higher priority than the second
int System::solve(SubSystem *subsysA, SubSystem *subsysB, bool /*isFine*/, bool isRedundantsolving)
{
    int xsizeA = subsysA->pSize();
    int xsizeB = subsysB->pSize();
    int csizeA = subsysA->cSize();

    VEC_pD plistAB(xsizeA+xsizeB);
    {
        VEC_pD plistA, plistB;
        subsysA->getParamList(plistA);
        subsysB->getParamList(plistB);

        std::sort(plistA.begin(),plistA.end());
        std::sort(plistB.begin(),plistB.end());

        VEC_pD::const_iterator it;
        it = std::set_union(plistA.begin(),plistA.end(),
                            plistB.begin(),plistB.end(),plistAB.begin());
        plistAB.resize(it-plistAB.begin());
    }
    int xsize = plistAB.size();

    Eigen::MatrixXd B = Eigen::MatrixXd::Identity(xsize, xsize);
    Eigen::MatrixXd JA(csizeA, xsize);
    Eigen::MatrixXd Y,Z;

    Eigen::VectorXd resA(csizeA);
    Eigen::VectorXd lambda(csizeA), lambda0(csizeA), lambdadir(csizeA);
    Eigen::VectorXd x(xsize), x0(xsize), xdir(xsize), xdir1(xsize);
    Eigen::VectorXd grad(xsize);
    Eigen::VectorXd h(xsize);
    Eigen::VectorXd y(xsize);
    Eigen::VectorXd Bh(xsize);

    // We assume that there are no common constraints in subsysA and subsysB
    subsysA->redirectParams();
    subsysB->redirectParams();

    subsysB->getParams(plistAB,x);
    subsysA->getParams(plistAB,x);
    subsysB->setParams(plistAB,x);  // just to ensure that A and B are synchronized

    subsysB->calcGrad(plistAB,grad);
    subsysA->calcJacobi(plistAB,JA);
    subsysA->calcResidual(resA);

    //double convergence = isFine ? XconvergenceFine : XconvergenceRough;
    int maxIterNumber = (isRedundantsolving?
        (sketchSizeMultiplierRedundant?maxIterRedundant * xsize:maxIterRedundant):
        (sketchSizeMultiplier?maxIter * xsize:maxIter));

    double divergingLim = 1e6*subsysA->error() + 1e12;

    double mu = 0;
    lambda.setZero();
    for (int iter=1; iter < maxIterNumber; iter++) {
        int status = qp_eq(B, grad, JA, resA, xdir, Y, Z);
        if (status)
            break;

        x0 = x;
        lambda0 = lambda;
        lambda = Y.transpose() * (B * xdir + grad);
        lambdadir = lambda - lambda0;

        // line search
        {
            double eta=0.25;
            double tau=0.5;
            double rho=0.5;
            double alpha=1;
            alpha = std::min(alpha, subsysA->maxStep(plistAB,xdir));

            // Eq. 18.32
            // double mu = lambda.lpNorm<Eigen::Infinity>() + 0.01;
            // Eq. 18.33
            // double mu =  grad.dot(xdir) / ( (1.-rho) * resA.lpNorm<1>());
            // Eq. 18.36
            mu =  std::max(mu,
                           (grad.dot(xdir) +  std::max(0., 0.5*xdir.dot(B*xdir))) /
                           ( (1. - rho) * resA.lpNorm<1>() ) );

            // Eq. 18.27
            double f0 = subsysB->error() + mu * resA.lpNorm<1>();

            // Eq. 18.29
            double deriv = grad.dot(xdir) - mu * resA.lpNorm<1>();

            x = x0 + alpha * xdir;
            subsysA->setParams(plistAB,x);
            subsysB->setParams(plistAB,x);
            subsysA->calcResidual(resA);
            double f = subsysB->error() + mu * resA.lpNorm<1>();

            // line search, Eq. 18.28
            bool first = true;
            while (f > f0 + eta * alpha * deriv) {
                if (first) { // try a second order step
//                    xdir1 = JA.jacobiSvd(Eigen::ComputeThinU |
//                                         Eigen::ComputeThinV).solve(-resA);
                    xdir1 = -Y*resA;
                    x += xdir1; // = x0 + alpha * xdir + xdir1
                    subsysA->setParams(plistAB,x);
                    subsysB->setParams(plistAB,x);
                    subsysA->calcResidual(resA);
                    f = subsysB->error() + mu * resA.lpNorm<1>();
                    if (f < f0 + eta * alpha * deriv)
                        break;
                }
                alpha = tau * alpha;
                if (alpha < 1e-8) // let the linesearch fail
                    alpha = 0.;
                x = x0 + alpha * xdir;
                subsysA->setParams(plistAB,x);
                subsysB->setParams(plistAB,x);
                subsysA->calcResidual(resA);
                f = subsysB->error() + mu * resA.lpNorm<1>();
                if (alpha < 1e-8) // let the linesearch fail
                    break;
            }
            lambda = lambda0 + alpha * lambdadir;

        }
        h = x - x0;

        y = grad - JA.transpose() * lambda;
        {
            subsysB->calcGrad(plistAB,grad);
            subsysA->calcJacobi(plistAB,JA);
            subsysA->calcResidual(resA);
        }
        y = grad - JA.transpose() * lambda - y; // Eq. 18.13

        if (iter > 1) {
            double yTh = y.dot(h);
            if (yTh != 0) {
                Bh = B * h;
                //Now calculate the BFGS update on B
                B += 1./yTh * y * y.transpose();
                B -= 1./h.dot(Bh) * (Bh * Bh.transpose());
            }
        }

        double err = subsysA->error();
        if (h.norm() <= (isRedundantsolving?convergenceRedundant:convergence) && err <= smallF)
            break;
        if (err > divergingLim || err != err) // check for diverging and NaN
            break;
    }

    int ret;
    if (subsysA->error() <= smallF)
        ret = Success;
    else if (h.norm() <= (isRedundantsolving?convergenceRedundant:convergence))
        ret = Converged;
    else
        ret = Failed;

    subsysA->revertParams();
    subsysB->revertParams();
    return ret;

}

void System::applySolution()
{
    for (int cid=0; cid < int(subSystems.size()); cid++) {
        if (subSystemsAux[cid])
            subSystemsAux[cid]->applySolution();
        if (subSystems[cid])
            subSystems[cid]->applySolution();
        for (MAP_pD_pD::const_iterator it=reductionmaps[cid].begin();
             it != reductionmaps[cid].end(); ++it)
            *(it->first) = *(it->second);
    }
}

void System::undoSolution()
{
    resetToReference();
}

int System::diagnose(Algorithm alg)
{
    // Analyses the constrainess grad of the system and provides feedback
    // The vector "conflictingTags" will hold a group of conflicting constraints

    // Hint 1: Only constraints with tag >= 0 are taken into account
    // Hint 2: Constraints tagged with 0 are treated as high priority
    //         constraints and they are excluded from the returned
    //         list of conflicting constraints. Therefore, this function
    //         will provide no feedback about possible conflicts between
    //         two high priority constraints. For this reason, tagging
    //         constraints with 0 should be used carefully.
    hasDiagnosis = false;
    if (!hasUnknowns) {
        dofs = -1;
        return dofs;
    }
#ifdef _GCS_DEBUG
#ifdef _DEBUG_TO_FILE
    std::ofstream stream;
    stream.open("GCS_debug.txt", std::ofstream::out | std::ofstream::app);
    stream << "GCS::System::diagnose()" << std::endl;
    stream.flush();
    stream.close();
#endif
#endif

    // When adding an external geometry or a constraint on an external geometry the array 'plist' is empty.
    // So, we must abort here because otherwise we would create an invalid matrix and make the application
    // eventually crash. This fixes issues #0002372/#0002373.
    if (plist.empty() || (plist.size() - pdrivenlist.size()) == 0) {
        hasDiagnosis = true;
        dofs = 0;
        return dofs;
    }

    redundant.clear();
    conflictingTags.clear();
    redundantTags.clear();

    // construct specific parameter list for diagonose ignoring driven constraint parameters
    GCS::VEC_pD pdiagnoselist;
    for (int j=0; j < int(plist.size()); j++) {
        auto result1 = std::find(std::begin(pdrivenlist), std::end(pdrivenlist), plist[j]);

        if (result1 == std::end(pdrivenlist)) {
            pdiagnoselist.push_back(plist[j]);
        }
    }

    // map tag to a tag multiplicity (the number of solver constraints associated with the same tag)
    std::map< int , int> tagmultiplicity;

    Eigen::MatrixXd J = Eigen::MatrixXd::Zero(clist.size(), pdiagnoselist.size());

    // The jacobian has been reduced to only contain driving constraints. Identification
    // of constraint indices from this reduced jacobian requires a mapping.
    std::map<int,int> jacobianconstraintmap;

    int jacobianconstraintcount=0;
    int allcount=0;
    for (std::vector<Constraint *>::iterator constr=clist.begin(); constr != clist.end(); ++constr) {
        (*constr)->revertParams();
        ++allcount;
        if ((*constr)->getTag() >= 0 && (*constr)->isDriving()) {
            jacobianconstraintcount++;
            for (int j=0; j < int(pdiagnoselist.size()); j++) {
                    J(jacobianconstraintcount-1,j) = (*constr)->grad(pdiagnoselist[j]);
            }

            // parallel processing: create tag multiplicity map
            if(tagmultiplicity.find((*constr)->getTag()) == tagmultiplicity.end())
                tagmultiplicity[(*constr)->getTag()] = 0;
            else
                tagmultiplicity[(*constr)->getTag()]++;

            jacobianconstraintmap[jacobianconstraintcount-1] = allcount-1;
        }
    }

#ifdef EIGEN_SPARSEQR_COMPATIBLE
    Eigen::SparseMatrix<double> SJ;

    if(qrAlgorithm==EigenSparseQR){
        // this creation is not optimized (done using triplets)
        // however the time this takes is negligible compared to the
        // time the QR decomposition itself takes
        SJ = J.sparseView();
        SJ.makeCompressed();
    }

    Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int> > SqrJT;
#else
    if(qrAlgorithm==EigenSparseQR){
        Base::Console().Warning("SparseQR not supported by you current version of Eigen. It requires Eigen 3.2.2 or higher. Falling back to Dense QR\n");
        qrAlgorithm=EigenDenseQR;
    }
#endif

#ifdef _GCS_DEBUG
    LogMatrix("J",J);
#endif

    Eigen::MatrixXd R;

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    Eigen::MatrixXd Q; // Obtaining the Q matrix with Sparse QR is buggy, see comments below
    Eigen::MatrixXd R2; // Intended for a trapezoidal matrix, where R is the top triangular matrix of the R2 trapezoidal matrix
#endif

    int paramsNum = 0;
    int constrNum = 0;
    int rank = 0;
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qrJT;

    if(qrAlgorithm==EigenDenseQR){
        if (J.rows() > 0) {
            qrJT.compute(J.topRows(jacobianconstraintcount).transpose());
            //Eigen::MatrixXd Q = qrJT.matrixQ ();

            paramsNum = qrJT.rows();
            constrNum = qrJT.cols();
            qrJT.setThreshold(qrpivotThreshold);
            rank = qrJT.rank();

            if (constrNum >= paramsNum)
                R = qrJT.matrixQR().triangularView<Eigen::Upper>();
            else
                R = qrJT.matrixQR().topRows(constrNum)
                                .triangularView<Eigen::Upper>();

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
            R2 = qrJT.matrixQR();
            Q = qrJT.matrixQ();
#endif
        }
    }
#ifdef EIGEN_SPARSEQR_COMPATIBLE
    else if(qrAlgorithm==EigenSparseQR){
        if (SJ.rows() > 0) {
            auto SJT = SJ.topRows(jacobianconstraintcount).transpose();
            if (SJT.rows() > 0 && SJT.cols() > 0) {
                SqrJT.compute(SJT);
                // Do not ask for Q Matrix!!
                // At Eigen 3.2 still has a bug that this only works for square matrices
                // if enabled it will crash
                #ifdef SPARSE_Q_MATRIX
                Q = SqrJT.matrixQ();
                //Q = QS;
                #endif

                paramsNum = SqrJT.rows();
                constrNum = SqrJT.cols();
                SqrJT.setPivotThreshold(qrpivotThreshold);
                rank = SqrJT.rank();

                if (constrNum >= paramsNum)
                    R = SqrJT.matrixR().triangularView<Eigen::Upper>();
                else
                    R = SqrJT.matrixR().topRows(constrNum)
                    .triangularView<Eigen::Upper>();

                #ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
                R2 = SqrJT.matrixR();
                #endif
            }
            else {
                paramsNum = SJT.rows();
                constrNum = SJT.cols();
            }
        }
    }
#endif

    if(debugMode==IterationLevel) {
        std::stringstream stream;

        stream  << (qrAlgorithm==EigenSparseQR?"EigenSparseQR":(qrAlgorithm==EigenDenseQR?"DenseQR":""));

        if (J.rows() > 0) {
            stream
#ifdef EIGEN_SPARSEQR_COMPATIBLE
                    << ", Threads: " << Eigen::nbThreads()
#endif
#ifdef EIGEN_VECTORIZE
                    << ", Vectorization: On"
#endif
                    << ", Pivot Threshold: " << qrpivotThreshold
                    << ", Params: " << paramsNum
                    << ", Constr: " << constrNum
                    << ", Rank: "   << rank;
        }
        else {
            stream
#ifdef EIGEN_SPARSEQR_COMPATIBLE
                    << ", Threads: " << Eigen::nbThreads()
#endif
#ifdef EIGEN_VECTORIZE
                    << ", Vectorization: On"
#endif
                    << ", Empty Sketch, nothing to solve";
        }

        const std::string tmp = stream.str();

        LogString(tmp);

    }

    if (J.rows() > 0) {
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        LogMatrix("R", R);

        LogMatrix("R2", R2);

        if(qrAlgorithm == EigenDenseQR){ // There is no rowsTranspositions in SparseQR. obtaining Q is buggy in Eigen for SparseQR
            LogMatrix("Q", Q);
            LogMatrix("RowTransp", qrJT.rowsTranspositions());
        }
#ifdef SPARSE_Q_MATRIX
        else if(qrAlgorithm == EigenSparseQR) {
            LogMatrix("Q", Q);
        }
#endif
#endif

        // DETECTING CONSTRAINT SOLVER PARAMETERS
        //
        // NOTE: This is only true for dense QR with full pivoting, because solve parameters get reordered.
        // I am unable to adapt it to Sparse QR. (abdullah). See:
        //
        // https://stackoverflow.com/questions/49009771/getting-rows-transpositions-with-sparse-qr
        // https://forum.kde.org/viewtopic.php?f=74&t=151239
        //
        // R (original version, not R here which is trimmed to not have empty rows)
        // has paramsNum rows, the first "rank" rows correspond to parameters that are constraint

        // Calculate the Permutation matrix from the Transposition matrix
        Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> rowPermutations;

        rowPermutations.setIdentity(paramsNum);

        if(qrAlgorithm==EigenDenseQR){ // P.J.P' = Q.R see https://eigen.tuxfamily.org/dox/classEigen_1_1FullPivHouseholderQR.html
            const MatrixIndexType rowTranspositions = qrJT.rowsTranspositions();

            for(int k = 0; k < rank; ++k)
                rowPermutations.applyTranspositionOnTheRight(k, rowTranspositions.coeff(k));
        }
#ifdef EIGEN_SPARSEQR_COMPATIBLE
        else if(qrAlgorithm==EigenSparseQR){
            // J.P = Q.R, see https://eigen.tuxfamily.org/dox/classEigen_1_1SparseQR.html
            // There is no rowsTransposition in this QR decomposition.
            // TODO: This detection method won't work for SparseQR
        }

#endif

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        std::stringstream stream;
#endif

// params (in the order of J) shown as independent from QR
        std::set<int> indepParamCols;
        std::set<int> depParamCols;

        for (int j=0; j < rank; j++) {

             int origRow = rowPermutations.indices()[j];

             indepParamCols.insert(origRow);

             // NOTE: Q*R = transpose(J), so the row of R corresponds to the col of J (the rows of transpose(J)).
             // The cols of J are the parameters, the rows are the constraints.
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
             stream << "R row " << j << " = J col " << origRow << std::endl;
#endif
        }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        std::string tmp = stream.str();

        LogString(tmp);
#endif

        // If not independent, must be dependent
        for(int j=0; j < paramsNum; j++) {
            auto result = std::find(indepParamCols.begin(), indepParamCols.end(), j);
            if(result == indepParamCols.end()) {
                depParamCols.insert(j);
            }
        }

        // Last (NumParams-rank) rows of Q construct the dependent part of J
        // in conjuntion with the R matrix
        // Last (NumParams-rank) cols of Q never contribute as R is zero after the rank
        /*std::set<int> associatedParamCols;
        for(int i = rank; i < paramsNum; i++) {
            for(int j = 0; j < rank; j++) {
                if(fabs(Q(i,j))>1e-10) {
                    for(int k = j; k < constrNum; k++) {
                        if(fabs(R(j,k))>1e-10) {
                            associatedParamCols.insert(k);
                        }
                    }
                }
            }
        }

        for( auto param : associatedParamCols) {
            auto pos = indepParamCols.find(rowPermutations.indices()[param]);

            if(pos!=indepParamCols.end()) {
                indepParamCols.erase(pos);
            }
        }*/
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        stream.flush();

        stream << "Indep params: [";
        for(auto indep :indepParamCols)
            stream << indep;
        stream << "]" << std::endl;

        stream << "Dep params: [";
        for(auto dep :depParamCols)
            stream << dep;
        stream << "]" << std::endl;

        tmp = stream.str();
        LogString(tmp);
#endif
        for( auto param : depParamCols) {
            pdependentparameters.push_back(pdiagnoselist[param]);
        }

        // Detecting conflicting or redundant constraints
        if (constrNum > rank) { // conflicting or redundant constraints
            for (int i=1; i < rank; i++) {
                // eliminate non zeros above pivot
                assert(R(i,i) != 0);
                for (int row=0; row < i; row++) {
                    if (R(row,i) != 0) {
                        double coef=R(row,i)/R(i,i);
                        R.block(row,i+1,1,constrNum-i-1) -= coef * R.block(i,i+1,1,constrNum-i-1);
                        R(row,i) = 0;
                    }
                }
            }
            std::vector< std::vector<Constraint *> > conflictGroups(constrNum-rank);
            for (int j=rank; j < constrNum; j++) {
                for (int row=0; row < rank; row++) {
                    if (fabs(R(row,j)) > 1e-10) {
                        int origCol = 0;

                        if(qrAlgorithm==EigenDenseQR)
                            origCol=qrJT.colsPermutation().indices()[row];
#ifdef EIGEN_SPARSEQR_COMPATIBLE
                        else if(qrAlgorithm==EigenSparseQR)
                            origCol=SqrJT.colsPermutation().indices()[row];
#endif
                        //conflictGroups[j-rank].push_back(clist[origCol]);
                        conflictGroups[j-rank].push_back(clist[jacobianconstraintmap.at(origCol)]);
                    }
                }
                int origCol = 0;

                if(qrAlgorithm==EigenDenseQR)
                    origCol=qrJT.colsPermutation().indices()[j];

#ifdef EIGEN_SPARSEQR_COMPATIBLE
                else if(qrAlgorithm==EigenSparseQR)
                    origCol=SqrJT.colsPermutation().indices()[j];
#endif
                //conflictGroups[j-rank].push_back(clist[origCol]);
                conflictGroups[j-rank].push_back(clist[jacobianconstraintmap.at(origCol)]);
            }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX

        stream.flush();

        stream << "ConflictGroups: [";
        for(auto group :conflictGroups)  {
            stream << "[";

            for(auto c :group)
                stream << c->getTag();

            stream << "]";
        }
        stream << "]" << std::endl;

        tmp = stream.str();
        LogString(tmp);
#endif

            // try to remove the conflicting constraints and solve the
            // system in order to check if the removed constraints were
            // just redundant but not really conflicting
            std::set<Constraint *> skipped;
            SET_I satisfiedGroups;
            while (1) {
                std::map< Constraint *, SET_I > conflictingMap;
                for (std::size_t i=0; i < conflictGroups.size(); i++) {
                    if (satisfiedGroups.count(i) == 0) {
                        for (std::size_t j=0; j < conflictGroups[i].size(); j++) {
                            Constraint *constr = conflictGroups[i][j];
                            if (constr->getTag() != 0) // exclude constraints tagged with zero
                                conflictingMap[constr].insert(i);
                        }
                    }
                }
                if (conflictingMap.empty())
                    break;

                int maxPopularity = 0;
                Constraint *mostPopular = NULL;
                for (std::map< Constraint *, SET_I >::const_iterator it=conflictingMap.begin();
                     it != conflictingMap.end(); ++it) {
                    if (static_cast<int>(it->second.size()) > maxPopularity ||
                        (static_cast<int>(it->second.size()) == maxPopularity && mostPopular &&
                        tagmultiplicity.at(it->first->getTag()) < tagmultiplicity.at(mostPopular->getTag())) ||

                        (static_cast<int>(it->second.size()) == maxPopularity && mostPopular &&
                        tagmultiplicity.at(it->first->getTag()) == tagmultiplicity.at(mostPopular->getTag()) &&
                         it->first->getTag() > mostPopular->getTag())

                    ) {
                        mostPopular = it->first;
                        maxPopularity = it->second.size();
                    }
                }
                if (maxPopularity > 0) {
                    skipped.insert(mostPopular);
                    for (SET_I::const_iterator it=conflictingMap[mostPopular].begin();
                         it != conflictingMap[mostPopular].end(); ++it)
                        satisfiedGroups.insert(*it);
                }
            }

            std::vector<Constraint *> clistTmp;
            clistTmp.reserve(clist.size());
            for (std::vector<Constraint *>::iterator constr=clist.begin();
                constr != clist.end(); ++constr) {
                if (skipped.count(*constr) == 0)
                    clistTmp.push_back(*constr);
            }

            SubSystem *subSysTmp = new SubSystem(clistTmp, pdiagnoselist);
            int res = solve(subSysTmp,true,alg,true);

            if(debugMode==Minimal || debugMode==IterationLevel) {
                std::string solvername;
                switch (alg) {
                    case 0:
                        solvername = "BFGS";
                        break;
                    case 1: // solving with the LevenbergMarquardt solver
                        solvername = "LevenbergMarquardt";
                        break;
                    case 2: // solving with the BFGS solver
                        solvername = "DogLeg";
                        break;
                }

                Base::Console().Log("Sketcher::RedundantSolving-%s-\n",solvername.c_str());
            }

            if (res == Success) {
                subSysTmp->applySolution();
                for (std::set<Constraint *>::const_iterator constr=skipped.begin();
                     constr != skipped.end(); ++constr) {
                    double err = (*constr)->error();
                    if (err * err < convergenceRedundant)
                        redundant.insert(*constr);
                }
                resetToReference();

                if(debugMode==Minimal || debugMode==IterationLevel) {
                    Base::Console().Log("Sketcher Redundant solving: %d redundants\n",redundant.size());
                }

                std::vector< std::vector<Constraint *> > conflictGroupsOrig=conflictGroups;
                conflictGroups.clear();
                for (int i=conflictGroupsOrig.size()-1; i >= 0; i--) {
                    bool isRedundant = false;
                    for (std::size_t j=0; j < conflictGroupsOrig[i].size(); j++) {
                        if (redundant.count(conflictGroupsOrig[i][j]) > 0) {
                            isRedundant = true;
                            break;
                        }
                    }
                    if (!isRedundant)
                        conflictGroups.push_back(conflictGroupsOrig[i]);
                    else
                        constrNum--;
                }
            }
            delete subSysTmp;

            // simplified output of conflicting tags
            SET_I conflictingTagsSet;
            for (std::size_t i=0; i < conflictGroups.size(); i++) {
                for (std::size_t j=0; j < conflictGroups[i].size(); j++) {
                    conflictingTagsSet.insert(conflictGroups[i][j]->getTag());
                }
            }
            conflictingTagsSet.erase(0); // exclude constraints tagged with zero
            conflictingTags.resize(conflictingTagsSet.size());
            std::copy(conflictingTagsSet.begin(), conflictingTagsSet.end(),
                      conflictingTags.begin());

            // output of redundant tags
            SET_I redundantTagsSet;
            for (std::set<Constraint *>::iterator constr=redundant.begin();
                 constr != redundant.end(); ++constr)
                redundantTagsSet.insert((*constr)->getTag());
            // remove tags represented at least in one non-redundant constraint
            for (std::vector<Constraint *>::iterator constr=clist.begin();
                constr != clist.end(); ++constr) {
                if (redundant.count(*constr) == 0)
                    redundantTagsSet.erase((*constr)->getTag());
            }
            redundantTags.resize(redundantTagsSet.size());
            std::copy(redundantTagsSet.begin(), redundantTagsSet.end(),
                      redundantTags.begin());

            if (paramsNum == rank && constrNum > rank) { // over-constrained
                hasDiagnosis = true;
                dofs = paramsNum - constrNum;
                return dofs;
            }
        }

        hasDiagnosis = true;
        dofs = paramsNum - rank;
        return dofs;
    }

    hasDiagnosis = true;
    dofs = pdiagnoselist.size();
    return dofs;
}

void System::clearSubSystems()
{
    isInit = false;
    free(subSystems);
    free(subSystemsAux);
    subSystems.clear();
    subSystemsAux.clear();
}

double lineSearch(SubSystem *subsys, Eigen::VectorXd &xdir)
{
    double f1,f2,f3,alpha1,alpha2,alpha3,alphaStar;

    double alphaMax = subsys->maxStep(xdir);

    Eigen::VectorXd x0, x;

    //Save initial values
    subsys->getParams(x0);

    //Start at the initial position alpha1 = 0
    alpha1 = 0.;
    f1 = subsys->error();

    //Take a step of alpha2 = 1
    alpha2 = 1.;
    x = x0 + alpha2 * xdir;
    subsys->setParams(x);
    f2 = subsys->error();

    //Take a step of alpha3 = 2*alpha2
    alpha3 = alpha2*2;
    x = x0 + alpha3 * xdir;
    subsys->setParams(x);
    f3 = subsys->error();

    //Now reduce or lengthen alpha2 and alpha3 until the minimum is
    //Bracketed by the triplet f1>f2<f3
    while (f2 > f1 || f2 > f3) {
        if (f2 > f1) {
            //If f2 is greater than f1 then we shorten alpha2 and alpha3 closer to f1
            //Effectively both are shortened by a factor of two.
            alpha3 = alpha2;
            f3 = f2;
            alpha2 = alpha2 / 2;
            x = x0 + alpha2 * xdir;
            subsys->setParams(x);
            f2 = subsys->error();
        }
        else if (f2 > f3) {
            if (alpha3 >= alphaMax)
                break;
            //If f2 is greater than f3 then we increase alpha2 and alpha3 away from f1
            //Effectively both are lengthened by a factor of two.
            alpha2 = alpha3;
            f2 = f3;
            alpha3 = alpha3 * 2;
            x = x0 + alpha3 * xdir;
            subsys->setParams(x);
            f3 = subsys->error();
        }
    }
    //Get the alpha for the minimum f of the quadratic approximation
    alphaStar = alpha2 + ((alpha2-alpha1)*(f1-f3))/(3*(f1-2*f2+f3));

    //Guarantee that the new alphaStar is within the bracket
    if (alphaStar >= alpha3 || alphaStar <= alpha1)
        alphaStar = alpha2;

    if (alphaStar > alphaMax)
        alphaStar = alphaMax;

    if (alphaStar != alphaStar)
        alphaStar = 0.;

    //Take a final step to alphaStar
    x = x0 + alphaStar * xdir;
    subsys->setParams(x);

    return alphaStar;
}


void free(VEC_pD &doublevec)
{
    for (VEC_pD::iterator it = doublevec.begin();
        it != doublevec.end(); ++it) {
        if (*it) delete *it;
    }
    doublevec.clear();
}

void free(std::vector<Constraint *> &constrvec)
{
    for (std::vector<Constraint *>::iterator constr=constrvec.begin();
         constr != constrvec.end(); ++constr) {
        if (*constr) {
            switch ((*constr)->getTypeId()) {
                case Equal:
                    delete static_cast<ConstraintEqual *>(*constr);
                    break;
                case Difference:
                    delete static_cast<ConstraintDifference *>(*constr);
                    break;
                case P2PDistance:
                    delete static_cast<ConstraintP2PDistance *>(*constr);
                    break;
                case P2PAngle:
                    delete static_cast<ConstraintP2PAngle *>(*constr);
                    break;
                case P2LDistance:
                    delete static_cast<ConstraintP2LDistance *>(*constr);
                    break;
                case PointOnLine:
                    delete static_cast<ConstraintPointOnLine *>(*constr);
                    break;
                case Parallel:
                    delete static_cast<ConstraintParallel *>(*constr);
                    break;
                case Perpendicular:
                    delete static_cast<ConstraintPerpendicular *>(*constr);
                    break;
                case L2LAngle:
                    delete static_cast<ConstraintL2LAngle *>(*constr);
                    break;
                case MidpointOnLine:
                    delete static_cast<ConstraintMidpointOnLine *>(*constr);
                    break;
                case None:
                default:
                    delete *constr;
            }
        }
    }
    constrvec.clear();
}

void free(std::vector<SubSystem *> &subsysvec)
{
    for (std::vector<SubSystem *>::iterator it=subsysvec.begin();
        it != subsysvec.end(); ++it) {
        if (*it) delete *it;
    }
}


} //namespace GCS
