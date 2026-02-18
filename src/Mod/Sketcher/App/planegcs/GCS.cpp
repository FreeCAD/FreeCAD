// SPDX-License-Identifier: LGPL-2.1-or-later

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
# pragma warning(disable : 4251)
# pragma warning(disable : 4244)
# pragma warning(disable : 4996)
#endif

#undef _GCS_DEBUG
#undef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
#undef _DEBUG_TO_FILE

// This has to be included BEFORE any EIGEN include
// This format is Sage compatible, so you can just copy/paste the matrix into Sage
#ifdef _GCS_DEBUG
# define EIGEN_DEFAULT_IO_FORMAT Eigen::IOFormat(3, 0, ",", ",\n", "[", "]", "[", "]")
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

#include <algorithm>
#include <future>
#include <iostream>
#include <limits>
#include <numbers>

#include "GCS.h"
#include "qp_eq.h"

// NOTE: In CMakeList.txt -DEIGEN_NO_DEBUG is set (it does not work with a define here), to solve
// this: this is needed to fix this SparseQR crash
// https://forum.freecad.org/viewtopic.php?f=10&t=11341&p=92146#p92146, until Eigen library fixes
// its own problem with the assertion (definitely not solved in 3.2.0 branch) NOTE2: solved in
// eigen3.3


// Extraction of Q matrix for Debugging used to crash
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
# if EIGEN_VERSION >= 30304
#  define SPARSE_Q_MATRIX
# endif
#endif

#if EIGEN_VERSION > 30290  // This regulates that only starting in Eigen 3.3, the problem with
                           // https://forum.freecad.org/viewtopic.php?f=3&t=4651&start=40
                           // was solved in Eigen:
                           // https://forum.freecad.org/viewtopic.php?f=10&t=12769&start=60#p106492
                           // https://forum.kde.org/viewtopic.php?f=74&t=129439
# define EIGEN_STOCK_FULLPIVLU_COMPUTE
#endif

// #undef EIGEN_SPARSEQR_COMPATIBLE


#ifdef EIGEN_SPARSEQR_COMPATIBLE
# include <Eigen/OrderingMethods>
#endif

// _GCS_EXTRACT_SOLVER_SUBSYSTEM_ to be enabled in Constraints.h when needed.
#if defined(_GCS_EXTRACT_SOLVER_SUBSYSTEM_) || defined(_DEBUG_TO_FILE)
# include <fstream>

# define CASE_NOT_IMP(X) \
     case X: { \
         subsystemfile << "//" #X "not yet implemented" << std::endl; \
         break; \
     }
#endif

#include <Base/Console.h>
#include <FCConfig.h>

#include <boost/graph/connected_components.hpp>
#include <boost_graph_adjacency_list.hpp>

using MatrixIndexType = Eigen::FullPivHouseholderQR<Eigen::MatrixXd>::IntDiagSizeVectorType;

#ifndef EIGEN_STOCK_FULLPIVLU_COMPUTE
namespace Eigen
{

using MatrixdType = Matrix<double, -1, -1, 0, -1, -1>;

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
    // number of NONTRIVIAL transpositions, i.e. m_rowsTranspositions[i]!=i
    Index number_of_transpositions = 0;

    // the generic case is that in which all pivots are nonzero (invertible case)
    m_nonzero_pivots = size;
    m_maxpivot = RealScalar(0);
    RealScalar cutoff(0);

    for (Index k = 0; k < size; ++k) {
        // First, we need to find the pivot.

        // biggest coefficient in the remaining bottom-right corner (starting at row k, col k)
        Index row_of_biggest_in_corner, col_of_biggest_in_corner;
        RealScalar biggest_in_corner;
        biggest_in_corner = m_lu.bottomRightCorner(rows - k, cols - k)
                                .cwiseAbs()
                                .maxCoeff(&row_of_biggest_in_corner, &col_of_biggest_in_corner);

        // correct the values! since they were computed in the corner,
        row_of_biggest_in_corner += k;
        col_of_biggest_in_corner += k;  // need to add k to them.

        // when k==0, biggest_in_corner is the biggest coeff absolute value in the original
        // matrix
        if (k == 0) {
            cutoff = biggest_in_corner * NumTraits<Scalar>::epsilon();
        }

        // if the pivot (hence the corner) is "zero", terminate to avoid generating nan/inf
        // values. Notice that using an exact comparison (biggest_in_corner==0) here, as
        // Golub-van Loan do in their pseudo-code, results in numerical instability! The cutoff
        // here has been validated by running the unit test 'lu' with many repetitions.
        if (biggest_in_corner < cutoff) {
            // before exiting, make sure to initialize the still uninitialized transpositions
            // in a sane state without destroying what we already have.
            m_nonzero_pivots = k;
            for (Index i = k; i < size; ++i) {
                m_rowsTranspositions.coeffRef(i) = i;
                m_colsTranspositions.coeffRef(i) = i;
            }
            break;
        }

        if (biggest_in_corner > m_maxpivot) {
            m_maxpivot = biggest_in_corner;
        }

        // Now that we've found the pivot, we need to apply the row/col swaps to
        // bring it to the location (k,k).

        m_rowsTranspositions.coeffRef(k) = row_of_biggest_in_corner;
        m_colsTranspositions.coeffRef(k) = col_of_biggest_in_corner;
        if (k != row_of_biggest_in_corner) {
            m_lu.row(k).swap(m_lu.row(row_of_biggest_in_corner));
            ++number_of_transpositions;
        }
        if (k != col_of_biggest_in_corner) {
            m_lu.col(k).swap(m_lu.col(col_of_biggest_in_corner));
            ++number_of_transpositions;
        }

        // Now that the pivot is at the right location, we update the remaining
        // bottom-right corner by Gaussian elimination.

        if (k < rows - 1) {
            m_lu.col(k).tail(rows - k - 1) /= m_lu.coeff(k, k);
        }
        if (k < size - 1) {
            m_lu.block(k + 1, k + 1, rows - k - 1, cols - k - 1).noalias()
                -= m_lu.col(k).tail(rows - k - 1) * m_lu.row(k).tail(cols - k - 1);
        }
    }

    // the main loop is over, we still have to accumulate the transpositions to find the
    // permutations P and Q

    m_p.setIdentity(rows);
    for (Index k = size - 1; k >= 0; --k) {
        m_p.applyTranspositionOnTheRight(k, m_rowsTranspositions.coeff(k));
    }

    m_q.setIdentity(cols);
    for (Index k = 0; k < size; ++k) {
        m_q.applyTranspositionOnTheRight(k, m_colsTranspositions.coeff(k));
    }

    m_det_pq = (number_of_transpositions % 2) ? -1 : 1;
    return *this;
}

}  // namespace Eigen
#endif

namespace GCS
{

class SolverReportingManager
{
public:
    SolverReportingManager(SolverReportingManager const&) = delete;
    SolverReportingManager(SolverReportingManager&&) = delete;
    SolverReportingManager& operator=(SolverReportingManager const&) = delete;
    SolverReportingManager& operator=(SolverReportingManager&&) = delete;

    static SolverReportingManager& Manager();

    inline void LogString(const std::string& str);

    inline void LogToConsole(const std::string& str);

    inline void LogToFile(const std::string& str);

    void LogQRSystemInformation(const System& system, int paramsNum = 0, int constrNum = 0, int rank = 0);

    void LogGroupOfConstraints(
        const std::string& str,
        std::vector<std::vector<Constraint*>> constraintgroups
    );
    void LogSetOfConstraints(const std::string& str, std::set<Constraint*> constraintset);
    void LogGroupOfParameters(const std::string& str, std::vector<std::vector<double*>> parametergroups);

    void LogMatrix(const std::string str, Eigen::MatrixXd matrix);
    void LogMatrix(const std::string str, MatrixIndexType matrix);

private:
    SolverReportingManager();
    ~SolverReportingManager();

    inline void initStream();
    inline void flushStream();

private:
#ifdef _DEBUG_TO_FILE
    std::ofstream stream;
#endif
};

SolverReportingManager::SolverReportingManager()
{
    initStream();
}

SolverReportingManager::~SolverReportingManager()
{
#ifdef _DEBUG_TO_FILE
    stream.flush();
    stream.close();
#endif
}

void SolverReportingManager::initStream()
{
#ifdef _DEBUG_TO_FILE
    if (!stream.is_open()) {
        stream.open("GCS_debug.txt", std::ofstream::out | std::ofstream::app);
    }
#endif
}

void SolverReportingManager::flushStream()
{
// Akwardly in some systems flushing does not force the write to the file, requiring a close
#ifdef _DEBUG_TO_FILE
    stream.flush();
    stream.close();
#endif
}

SolverReportingManager& SolverReportingManager::Manager()
{
    static SolverReportingManager theInstance;

    return theInstance;
}

void SolverReportingManager::LogToConsole(const std::string& str)
{
    Base::Console().log(str.c_str());
}

void SolverReportingManager::LogToFile(const std::string& str)
{
#ifdef _DEBUG_TO_FILE
    initStream();

    stream << str << std::endl;

    flushStream();
#else
    (void)(str);  // silence unused parameter
    LogToConsole("Debugging to file not enabled!");
#endif
}

void SolverReportingManager::LogString(const std::string& str)
{
    LogToConsole(str);

#ifdef _DEBUG_TO_FILE
    LogToFile(str);
#endif
}

void SolverReportingManager::LogQRSystemInformation(
    const System& system,
    int paramsNum,
    int constrNum,
    int rank
)
{

    std::stringstream tempstream;

    tempstream
        << (system.qrAlgorithm == EigenSparseQR
                ? "EigenSparseQR"
                : (system.qrAlgorithm == EigenDenseQR ? "DenseQR" : ""));

    if (paramsNum > 0) {
        tempstream
#ifdef EIGEN_SPARSEQR_COMPATIBLE
            << ", Threads: " << Eigen::nbThreads()
#endif
#ifdef EIGEN_VECTORIZE
            << ", Vectorization: On"
#endif
            << ", Pivot Threshold: " << system.qrpivotThreshold << ", Params: " << paramsNum
            << ", Constr: " << constrNum << ", Rank: " << rank << std::endl;
    }
    else {
        tempstream
#ifdef EIGEN_SPARSEQR_COMPATIBLE
            << ", Threads: " << Eigen::nbThreads()
#endif
#ifdef EIGEN_VECTORIZE
            << ", Vectorization: On"
#endif
            << ", Empty Sketch, nothing to solve" << std::endl;
    }

    LogString(tempstream.str());
}

void SolverReportingManager::LogGroupOfConstraints(
    const std::string& str,
    std::vector<std::vector<Constraint*>> constraintgroups
)
{
    std::stringstream tempstream;

    tempstream << str << ":" << '\n';

    for (const auto& group : constraintgroups) {
        tempstream << "[";

        for (auto c : group) {
            tempstream << c->getTag() << " ";
        }

        tempstream << "]" << '\n';
    }

    LogString(tempstream.str());
}

void SolverReportingManager::LogSetOfConstraints(
    const std::string& str,
    std::set<Constraint*> constraintset
)
{
    std::stringstream tempstream;

    tempstream << str << ": [";

    for (auto c : constraintset) {
        tempstream << c->getTag() << " ";
    }

    tempstream << "]" << '\n';

    LogString(tempstream.str());
}

void SolverReportingManager::LogGroupOfParameters(
    const std::string& str,
    std::vector<std::vector<double*>> parametergroups
)
{
    std::stringstream tempstream;

    tempstream << str << ":" << '\n';

    for (size_t i = 0; i < parametergroups.size(); i++) {
        tempstream << "[";

        for (auto p : parametergroups[i]) {
            tempstream << std::hex << p << " ";
        }

        tempstream << "]" << '\n';
    }

    LogString(tempstream.str());
}

#ifdef _GCS_DEBUG
void SolverReportingManager::LogMatrix(const std::string str, Eigen::MatrixXd matrix)
{
    std::stringstream tempstream;

    tempstream << '\n' << " " << str << " =" << '\n';
    tempstream << "[" << '\n';
    tempstream << matrix << '\n';
    tempstream << "]" << '\n';

    LogString(tempstream.str());
}

void SolverReportingManager::LogMatrix(const std::string str, MatrixIndexType matrix)
{
    std::stringstream tempstream;

    stream << '\n' << " " << str << " =" << '\n';
    stream << "[" << '\n';
    stream << matrix << '\n';
    stream << "]" << '\n';

    LogString(tempstream.str());
}
#endif


using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;

///////////////////////////////////////
// Solver
///////////////////////////////////////

// System
System::System()
    : plist(0)
    , pdrivenlist(0)
    , pDependentParameters(0)
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
    , emptyDiagnoseMatrix(true)
    , maxIter(100)
    , maxIterRedundant(100)
    , sketchSizeMultiplier(false)
    , sketchSizeMultiplierRedundant(false)
    , convergence(1e-10)
    , convergenceRedundant(1e-10)
    , qrAlgorithm(EigenSparseQR)
    , autoChooseAlgorithm(true)
    , autoQRThreshold(1000)
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

System::~System()
{
    clear();
}

void System::clear()
{
    plist.clear();
    pdrivenlist.clear();
    pIndex.clear();
    pDependentParameters.clear();
    pDependentParametersGroups.clear();
    hasUnknowns = false;
    hasDiagnosis = false;

    emptyDiagnoseMatrix = true;

    redundant.clear();
    conflictingTags.clear();
    redundantTags.clear();
    partiallyRedundantTags.clear();

    reference.clear();
    clearSubSystems();
    deleteAllContent(clist);
    drivenConstraints.clear();
    c2p.clear();
    p2c.clear();
}

void System::invalidatedDiagnosis()
{
    hasDiagnosis = false;
    pDependentParameters.clear();
    pDependentParametersGroups.clear();
}

void System::clearByTag(int tagId)
{
    std::vector<Constraint*> constrvec;
    for (const auto& constr : clist) {
        if (constr->getTag() == tagId) {
            constrvec.push_back(constr);
        }
    }
    for (const auto& constr : constrvec) {
        removeConstraint(constr);
    }
}

int System::addConstraint(Constraint* constr)
{
    isInit = false;
    if (constr->getTag() >= 0) {  // negatively tagged constraints have no impact
        hasDiagnosis = false;     // on the diagnosis
    }
    if (!constr->isDriving()) {
        drivenConstraints.push_back(constr);
    }

    clist.push_back(constr);
    VEC_pD constr_params = constr->params();
    for (const auto& param : constr_params) {
        // jacobi.set(constr, *param, 0.);
        c2p[constr].push_back(param);
        p2c[param].push_back(constr);
    }
    return clist.size() - 1;
}

void System::removeConstraint(Constraint* constr)
{
    if (std::erase(clist, constr) == 0) {
        return;
    }
    std::erase(drivenConstraints, constr);

    if (constr->getTag() >= 0) {
        hasDiagnosis = false;
    }
    clearSubSystems();

    for (const auto& param : c2p[constr]) {
        p2c[param].erase(std::ranges::find(p2c[param], constr));
    }
    c2p.erase(constr);

    delete (constr);
}

// basic constraints

int System::addConstraintEqual(
    double* param1,
    double* param2,
    int tagId,
    bool driving,
    Constraint::Alignment internalalignment
)
{
    Constraint* constr = new ConstraintEqual(param1, param2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    constr->setInternalAlignment(internalalignment);
    return addConstraint(constr);
}

int System::addConstraintProportional(double* param1, double* param2, double ratio, int tagId, bool driving)
{
    Constraint* constr = new ConstraintEqual(param1, param2, ratio);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintDifference(double* param1, double* param2, double* difference, int tagId, bool driving)
{
    Constraint* constr = new ConstraintDifference(param1, param2, difference);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PDistance(Point& p1, Point& p2, double* distance, int tagId, bool driving)
{
    Constraint* constr = new ConstraintP2PDistance(p1, p2, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PAngle(Point& p1, Point& p2, double* angle, double incrAngle, int tagId, bool driving)
{
    Constraint* constr = new ConstraintP2PAngle(p1, p2, angle, incrAngle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2PAngle(Point& p1, Point& p2, double* angle, int /*tagId*/, bool driving)
{
    return addConstraintP2PAngle(p1, p2, angle, 0., 0, driving);
}

int System::addConstraintP2LDistance(Point& p, Line& l, double* distance, int tagId, bool driving)
{
    Constraint* constr = new ConstraintP2LDistance(p, l, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnLine(Point& p, Line& l, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnLine(p, l);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnLine(Point& p, Point& lp1, Point& lp2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnLine(p, lp1, lp2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnPerpBisector(Point& p, Line& l, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnPerpBisector(p, l);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnPerpBisector(Point& p, Point& lp1, Point& lp2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnPerpBisector(p, lp1, lp2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintParallel(Line& l1, Line& l2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintParallel(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPerpendicular(Line& l1, Line& l2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPerpendicular(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPerpendicular(
    Point& l1p1,
    Point& l1p2,
    Point& l2p1,
    Point& l2p2,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintPerpendicular(l1p1, l1p2, l2p1, l2p2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintL2LAngle(Line& l1, Line& l2, double* angle, int tagId, bool driving)
{
    Constraint* constr = new ConstraintL2LAngle(l1, l2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintL2LAngle(
    Point& l1p1,
    Point& l1p2,
    Point& l2p1,
    Point& l2p2,
    double* angle,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintL2LAngle(l1p1, l1p2, l2p1, l2p2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintAngleViaPoint(Curve& crv1, Curve& crv2, Point& p, double* angle, int tagId, bool driving)
{
    Constraint* constr = new ConstraintAngleViaPoint(crv1, crv2, p, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintAngleViaTwoPoints(
    Curve& crv1,
    Curve& crv2,
    Point& p1,
    Point& p2,
    double* angle,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintAngleViaTwoPoints(crv1, crv2, p1, p2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintAngleViaPointAndParam(
    Curve& crv1,
    Curve& crv2,
    Point& p,
    double* cparam,
    double* angle,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintAngleViaPointAndParam(crv1, crv2, p, cparam, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintAngleViaPointAndTwoParams(
    Curve& crv1,
    Curve& crv2,
    Point& p,
    double* cparam1,
    double* cparam2,
    double* angle,
    int tagId,
    bool driving
)
{
    Constraint* constr
        = new ConstraintAngleViaPointAndTwoParams(crv1, crv2, p, cparam1, cparam2, angle);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintMidpointOnLine(Line& l1, Line& l2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintMidpointOnLine(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintMidpointOnLine(
    Point& l1p1,
    Point& l1p2,
    Point& l2p1,
    Point& l2p2,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintMidpointOnLine(l1p1, l1p2, l2p1, l2p2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintTangentCircumf(
    Point& p1,
    Point& p2,
    double* rad1,
    double* rad2,
    bool internal,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintTangentCircumf(p1, p2, rad1, rad2, internal);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintTangentAtBSplineKnot(
    BSpline& b,
    Line& l,
    unsigned int knotindex,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintSlopeAtBSplineKnot(b, l, knotindex);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintC2CDistance(Circle& c1, Circle& c2, double* dist, int tagId, bool driving)
{
    Constraint* constr = new ConstraintC2CDistance(c1, c2, dist);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintC2LDistance(Circle& c, Line& l, double* dist, int tagId, bool driving)
{
    Constraint* constr = new ConstraintC2LDistance(c, l, dist);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintP2CDistance(Point& p, Circle& c, double* distance, int tagId, bool driving)
{
    Constraint* constr = new ConstraintP2CDistance(p, c, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintArcLength(Arc& a, double* distance, int tagId, bool driving)
{
    Constraint* constr = new ConstraintArcLength(a, distance);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}


// derived constraints

int System::addConstraintP2PCoincident(Point& p1, Point& p2, int tagId, bool driving)
{
    addConstraintEqual(p1.x, p2.x, tagId, driving);
    return addConstraintEqual(p1.y, p2.y, tagId, driving);
}

int System::addConstraintHorizontal(Line& l, int tagId, bool driving)
{
    return addConstraintEqual(l.p1.y, l.p2.y, tagId, driving);
}

int System::addConstraintHorizontal(Point& p1, Point& p2, int tagId, bool driving)
{
    return addConstraintEqual(p1.y, p2.y, tagId, driving);
}

int System::addConstraintVertical(Line& l, int tagId, bool driving)
{
    return addConstraintEqual(l.p1.x, l.p2.x, tagId, driving);
}

int System::addConstraintVertical(Point& p1, Point& p2, int tagId, bool driving)
{
    return addConstraintEqual(p1.x, p2.x, tagId, driving);
}

int System::addConstraintCoordinateX(Point& p, double* x, int tagId, bool driving)
{
    return addConstraintEqual(p.x, x, tagId, driving);
}

int System::addConstraintCoordinateY(Point& p, double* y, int tagId, bool driving)
{
    return addConstraintEqual(p.y, y, tagId, driving);
}

int System::addConstraintArcRules(Arc& a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start, a, a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end, a, a.endAngle, tagId, driving);
}

int System::addConstraintPointOnCircle(Point& p, Circle& c, int tagId, bool driving)
{
    return addConstraintP2PDistance(p, c.center, c.rad, tagId, driving);
}

int System::addConstraintPointOnEllipse(Point& p, Ellipse& e, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnEllipse(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnHyperbolicArc(Point& p, ArcOfHyperbola& e, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnHyperbola(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnParabolicArc(Point& p, ArcOfParabola& e, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnParabola(p, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintPointOnBSpline(Point& p, BSpline& b, double* pointparam, int tagId, bool driving)
{
    Constraint* constr = new ConstraintPointOnBSpline(p.x, pointparam, 0, b);
    constr->setTag(tagId);
    constr->setDriving(driving);
    addConstraint(constr);

    constr = new ConstraintPointOnBSpline(p.y, pointparam, 1, b);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintArcOfEllipseRules(ArcOfEllipse& a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start, a, a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end, a, a.endAngle, tagId, driving);
}

int System::addConstraintCurveValue(Point& p, Curve& a, double* u, int tagId, bool driving)
{
    Constraint* constr = new ConstraintCurveValue(p, p.x, a, u);
    constr->setTag(tagId);
    constr->setDriving(driving);
    addConstraint(constr);
    constr = new ConstraintCurveValue(p, p.y, a, u);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintArcOfHyperbolaRules(ArcOfHyperbola& a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start, a, a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end, a, a.endAngle, tagId, driving);
}

int System::addConstraintArcOfParabolaRules(ArcOfParabola& a, int tagId, bool driving)
{
    addConstraintCurveValue(a.start, a, a.startAngle, tagId, driving);
    return addConstraintCurveValue(a.end, a, a.endAngle, tagId, driving);
}

int System::addConstraintPointOnArc(Point& p, Arc& a, int tagId, bool driving)
{
    return addConstraintP2PDistance(p, a.center, a.rad, tagId, driving);
}

int System::addConstraintPerpendicularLine2Arc(Point& p1, Point& p2, Arc& a, int tagId, bool driving)
{
    using std::numbers::pi;

    addConstraintP2PCoincident(p2, a.start, tagId, driving);
    double dx = *(p2.x) - *(p1.x);
    double dy = *(p2.y) - *(p1.y);
    if (dx * cos(*(a.startAngle)) + dy * sin(*(a.startAngle)) > 0) {
        return addConstraintP2PAngle(p1, p2, a.startAngle, 0, tagId, driving);
    }
    else {
        return addConstraintP2PAngle(p1, p2, a.startAngle, pi, tagId, driving);
    }
}

int System::addConstraintPerpendicularArc2Line(Arc& a, Point& p1, Point& p2, int tagId, bool driving)
{
    using std::numbers::pi;

    addConstraintP2PCoincident(p1, a.end, tagId, driving);
    double dx = *(p2.x) - *(p1.x);
    double dy = *(p2.y) - *(p1.y);
    if (dx * cos(*(a.endAngle)) + dy * sin(*(a.endAngle)) > 0) {
        return addConstraintP2PAngle(p1, p2, a.endAngle, 0, tagId, driving);
    }
    else {
        return addConstraintP2PAngle(p1, p2, a.endAngle, pi, tagId, driving);
    }
}

int System::addConstraintPerpendicularCircle2Arc(Point& center, double* radius, Arc& a, int tagId, bool driving)
{
    using std::numbers::pi;

    addConstraintP2PDistance(a.start, center, radius, tagId, driving);
    double incrAngle = *(a.startAngle) < *(a.endAngle) ? pi / 2 : -pi / 2;
    double tangAngle = *a.startAngle + incrAngle;
    double dx = *(a.start.x) - *(center.x);
    double dy = *(a.start.y) - *(center.y);
    if (dx * cos(tangAngle) + dy * sin(tangAngle) > 0) {
        return addConstraintP2PAngle(center, a.start, a.startAngle, incrAngle, tagId, driving);
    }
    else {
        return addConstraintP2PAngle(center, a.start, a.startAngle, -incrAngle, tagId, driving);
    }
}

int System::addConstraintPerpendicularArc2Circle(Arc& a, Point& center, double* radius, int tagId, bool driving)
{
    using std::numbers::pi;

    addConstraintP2PDistance(a.end, center, radius, tagId, driving);
    double incrAngle = *(a.startAngle) < *(a.endAngle) ? -pi / 2 : pi / 2;
    double tangAngle = *a.endAngle + incrAngle;
    double dx = *(a.end.x) - *(center.x);
    double dy = *(a.end.y) - *(center.y);
    if (dx * cos(tangAngle) + dy * sin(tangAngle) > 0) {
        return addConstraintP2PAngle(center, a.end, a.endAngle, incrAngle, tagId, driving);
    }
    else {
        return addConstraintP2PAngle(center, a.end, a.endAngle, -incrAngle, tagId, driving);
    }
}

int System::addConstraintPerpendicularArc2Arc(
    Arc& a1,
    bool reverse1,
    Arc& a2,
    bool reverse2,
    int tagId,
    bool driving
)
{
    Point& p1 = reverse1 ? a1.start : a1.end;
    Point& p2 = reverse2 ? a2.end : a2.start;
    addConstraintP2PCoincident(p1, p2, tagId, driving);
    return addConstraintPerpendicular(a1.center, p1, a2.center, p2, tagId, driving);
}

int System::addConstraintTangent(Line& l, Circle& c, int tagId, bool driving)
{
    return addConstraintP2LDistance(c.center, l, c.rad, tagId, driving);
}

int System::addConstraintTangent(Line& l, Ellipse& e, int tagId, bool driving)
{
    Constraint* constr = new ConstraintEllipseTangentLine(l, e);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintTangent(Line& l, Arc& a, int tagId, bool driving)
{
    return addConstraintP2LDistance(a.center, l, a.rad, tagId, driving);
}

int System::addConstraintTangent(Circle& c1, Circle& c2, int tagId, bool driving)
{
    double dx = *(c2.center.x) - *(c1.center.x);
    double dy = *(c2.center.y) - *(c1.center.y);
    double d = sqrt(dx * dx + dy * dy);
    return addConstraintTangentCircumf(
        c1.center,
        c2.center,
        c1.rad,
        c2.rad,
        (d < *c1.rad || d < *c2.rad),
        tagId,
        driving
    );
}

int System::addConstraintTangent(Arc& a1, Arc& a2, int tagId, bool driving)
{
    double dx = *(a2.center.x) - *(a1.center.x);
    double dy = *(a2.center.y) - *(a1.center.y);
    double d = sqrt(dx * dx + dy * dy);
    return addConstraintTangentCircumf(
        a1.center,
        a2.center,
        a1.rad,
        a2.rad,
        (d < *a1.rad || d < *a2.rad),
        tagId,
        driving
    );
}

int System::addConstraintTangent(Circle& c, Arc& a, int tagId, bool driving)
{
    double dx = *(a.center.x) - *(c.center.x);
    double dy = *(a.center.y) - *(c.center.y);
    double d = sqrt(dx * dx + dy * dy);
    return addConstraintTangentCircumf(
        c.center,
        a.center,
        c.rad,
        a.rad,
        (d < *c.rad || d < *a.rad),
        tagId,
        driving
    );
}

int System::addConstraintCircleRadius(Circle& c, double* radius, int tagId, bool driving)
{
    return addConstraintEqual(c.rad, radius, tagId, driving);
}

int System::addConstraintArcRadius(Arc& a, double* radius, int tagId, bool driving)
{
    return addConstraintEqual(a.rad, radius, tagId, driving);
}

int System::addConstraintCircleDiameter(Circle& c, double* diameter, int tagId, bool driving)
{
    return addConstraintProportional(c.rad, diameter, 0.5, tagId, driving);
}

int System::addConstraintArcDiameter(Arc& a, double* diameter, int tagId, bool driving)
{
    return addConstraintProportional(a.rad, diameter, 0.5, tagId, driving);
}

int System::addConstraintEqualLength(Line& l1, Line& l2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintEqualLineLength(l1, l2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualRadius(Circle& c1, Circle& c2, int tagId, bool driving)
{
    return addConstraintEqual(c1.rad, c2.rad, tagId, driving);
}

int System::addConstraintEqualRadii(Ellipse& e1, Ellipse& e2, int tagId, bool driving)
{
    addConstraintEqual(e1.radmin, e2.radmin, tagId, driving);

    Constraint* constr = new ConstraintEqualMajorAxesConic(&e1, &e2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualRadii(ArcOfHyperbola& a1, ArcOfHyperbola& a2, int tagId, bool driving)
{
    addConstraintEqual(a1.radmin, a2.radmin, tagId, driving);

    Constraint* constr = new ConstraintEqualMajorAxesConic(&a1, &a2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualFocus(ArcOfParabola& a1, ArcOfParabola& a2, int tagId, bool driving)
{
    Constraint* constr = new ConstraintEqualFocalDistance(&a1, &a2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintEqualRadius(Circle& c1, Arc& a2, int tagId, bool driving)
{
    return addConstraintEqual(c1.rad, a2.rad, tagId, driving);
}

int System::addConstraintEqualRadius(Arc& a1, Arc& a2, int tagId, bool driving)
{
    return addConstraintEqual(a1.rad, a2.rad, tagId, driving);
}

int System::addConstraintP2PSymmetric(Point& p1, Point& p2, Line& l, int tagId, bool driving)
{
    addConstraintPerpendicular(p1, p2, l.p1, l.p2, tagId, driving);
    return addConstraintMidpointOnLine(p1, p2, l.p1, l.p2, tagId, driving);
}

int System::addConstraintP2PSymmetric(Point& p1, Point& p2, Point& p, int tagId, bool driving)
{
    addConstraintPointOnPerpBisector(p, p1, p2, tagId, driving);
    return addConstraintPointOnLine(p, p1, p2, tagId, driving);
}

int System::addConstraintSnellsLaw(
    Curve& ray1,
    Curve& ray2,
    Curve& boundary,
    Point p,
    double* n1,
    double* n2,
    bool flipn1,
    bool flipn2,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintSnell(ray1, ray2, boundary, p, n1, n2, flipn1, flipn2);
    constr->setTag(tagId);
    constr->setDriving(driving);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentPoint2Ellipse(
    Ellipse& e,
    Point& p1,
    InternalAlignmentType alignmentType,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintInternalAlignmentPoint2Ellipse(e, p1, alignmentType);
    constr->setTag(tagId);
    constr->setDriving(driving);
    constr->setInternalAlignment(Constraint::Alignment::InternalAlignment);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentPoint2Hyperbola(
    Hyperbola& e,
    Point& p1,
    InternalAlignmentType alignmentType,
    int tagId,
    bool driving
)
{
    Constraint* constr = new ConstraintInternalAlignmentPoint2Hyperbola(e, p1, alignmentType);
    constr->setTag(tagId);
    constr->setDriving(driving);
    constr->setInternalAlignment(Constraint::Alignment::InternalAlignment);
    return addConstraint(constr);
}

int System::addConstraintInternalAlignmentEllipseMajorDiameter(
    Ellipse& e,
    Point& p1,
    Point& p2,
    int tagId,
    bool driving
)
{
    double X_1 = *p1.x;
    double Y_1 = *p1.y;
    double X_2 = *p2.x;
    double Y_2 = *p2.y;
    double X_c = *e.center.x;
    double Y_c = *e.center.y;
    double X_F1 = *e.focus1.x;
    double Y_F1 = *e.focus1.y;
    double b = *e.radmin;

    double closertopositivemajor
        = pow(X_1 - X_c
                  - (X_F1 - X_c) * sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        - pow(X_2 - X_c
                  - (X_F1 - X_c) * sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        + pow(Y_1 - Y_c
                  - (Y_F1 - Y_c) * sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        - pow(Y_2 - Y_c
                  - (Y_F1 - Y_c) * sqrt(pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2);

    if (closertopositivemajor > 0) {
        // p2 is closer to  positivemajor. Assign constraints back-to-front.
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipsePositiveMajorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipsePositiveMajorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseNegativeMajorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseNegativeMajorY, tagId, driving);
    }
    else {
        // p1 is closer to  positivemajor
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipsePositiveMajorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipsePositiveMajorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipseNegativeMajorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipseNegativeMajorY, tagId, driving);
    }
}

int System::addConstraintInternalAlignmentEllipseMinorDiameter(
    Ellipse& e,
    Point& p1,
    Point& p2,
    int tagId,
    bool driving
)
{
    double X_1 = *p1.x;
    double Y_1 = *p1.y;
    double X_2 = *p2.x;
    double Y_2 = *p2.y;
    double X_c = *e.center.x;
    double Y_c = *e.center.y;
    double X_F1 = *e.focus1.x;
    double Y_F1 = *e.focus1.y;
    double b = *e.radmin;

    double closertopositiveminor
        = pow(X_1 - X_c + b * (Y_F1 - Y_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2)
        - pow(X_2 - X_c + b * (Y_F1 - Y_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2)
        + pow(-Y_1 + Y_c + b * (X_F1 - X_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2)
        - pow(-Y_2 + Y_c + b * (X_F1 - X_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)), 2);

    if (closertopositiveminor > 0) {
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipsePositiveMinorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipsePositiveMinorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseNegativeMinorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseNegativeMinorY, tagId, driving);
    }
    else {
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipsePositiveMinorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipsePositiveMinorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipseNegativeMinorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Ellipse(e, p2, EllipseNegativeMinorY, tagId, driving);
    }
}

int System::addConstraintInternalAlignmentEllipseFocus1(Ellipse& e, Point& p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving, Constraint::Alignment::InternalAlignment);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving, Constraint::Alignment::InternalAlignment);
}

int System::addConstraintInternalAlignmentEllipseFocus2(Ellipse& e, Point& p1, int tagId, bool driving)
{
    addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseFocus2X, tagId, driving);
    return addConstraintInternalAlignmentPoint2Ellipse(e, p1, EllipseFocus2Y, tagId, driving);
}

int System::addConstraintInternalAlignmentHyperbolaMajorDiameter(
    Hyperbola& e,
    Point& p1,
    Point& p2,
    int tagId,
    bool driving
)
{
    double X_1 = *p1.x;
    double Y_1 = *p1.y;
    double X_2 = *p2.x;
    double Y_2 = *p2.y;
    double X_c = *e.center.x;
    double Y_c = *e.center.y;
    double X_F1 = *e.focus1.x;
    double Y_F1 = *e.focus1.y;
    double b = *e.radmin;

    double closertopositivemajor = pow(-X_1 + X_c
                                           + (X_F1 - X_c)
                                               * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                                               / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
                                       2)
        - pow(-X_2 + X_c
                  + (X_F1 - X_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        + pow(-Y_1 + Y_c
                  + (Y_F1 - Y_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        - pow(-Y_2 + Y_c
                  + (Y_F1 - Y_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2);

    if (closertopositivemajor > 0) {
        // p2 is closer to  positivemajor. Assign constraints back-to-front.
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaPositiveMajorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaPositiveMajorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaNegativeMajorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaNegativeMajorY, tagId, driving);
    }
    else {
        // p1 is closer to  positivemajor
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaPositiveMajorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaPositiveMajorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaNegativeMajorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaNegativeMajorY, tagId, driving);
    }
}

int System::addConstraintInternalAlignmentHyperbolaMinorDiameter(
    Hyperbola& e,
    Point& p1,
    Point& p2,
    int tagId,
    bool driving
)
{
    double X_1 = *p1.x;
    double Y_1 = *p1.y;
    double X_2 = *p2.x;
    double Y_2 = *p2.y;
    double X_c = *e.center.x;
    double Y_c = *e.center.y;
    double X_F1 = *e.focus1.x;
    double Y_F1 = *e.focus1.y;
    double b = *e.radmin;

    double closertopositiveminor
        = pow(-X_1 + X_c + b * (Y_F1 - Y_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                  + (X_F1 - X_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        - pow(-X_2 + X_c + b * (Y_F1 - Y_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                  + (X_F1 - X_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        + pow(-Y_1 + Y_c - b * (X_F1 - X_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                  + (Y_F1 - Y_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2)
        - pow(-Y_2 + Y_c - b * (X_F1 - X_c) / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                  + (Y_F1 - Y_c) * (-pow(b, 2) + pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2))
                      / sqrt(pow(X_F1 - X_c, 2) + pow(Y_F1 - Y_c, 2)),
              2);

    if (closertopositiveminor < 0) {
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaPositiveMinorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaPositiveMinorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaNegativeMinorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaNegativeMinorY, tagId, driving);
    }
    else {
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaPositiveMinorX, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p1, HyperbolaPositiveMinorY, tagId, driving);
        addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaNegativeMinorX, tagId, driving);
        return addConstraintInternalAlignmentPoint2Hyperbola(e, p2, HyperbolaNegativeMinorY, tagId, driving);
    }
}

int System::addConstraintInternalAlignmentHyperbolaFocus(Hyperbola& e, Point& p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving, Constraint::Alignment::InternalAlignment);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving, Constraint::Alignment::InternalAlignment);
}

int System::addConstraintInternalAlignmentParabolaFocus(Parabola& e, Point& p1, int tagId, bool driving)
{
    addConstraintEqual(e.focus1.x, p1.x, tagId, driving, Constraint::Alignment::InternalAlignment);
    return addConstraintEqual(e.focus1.y, p1.y, tagId, driving, Constraint::Alignment::InternalAlignment);
}

int System::addConstraintInternalAlignmentBSplineControlPoint(
    BSpline& b,
    Circle& c,
    unsigned int poleindex,
    int tagId,
    bool driving
)
{
    addConstraintEqual(
        b.poles[poleindex].x,
        c.center.x,
        tagId,
        driving,
        Constraint::Alignment::InternalAlignment
    );
    addConstraintEqual(
        b.poles[poleindex].y,
        c.center.y,
        tagId,
        driving,
        Constraint::Alignment::InternalAlignment
    );
    return addConstraintEqual(
        b.weights[poleindex],
        c.rad,
        tagId,
        driving,
        Constraint::Alignment::InternalAlignment
    );
}

int System::addConstraintInternalAlignmentKnotPoint(
    BSpline& b,
    Point& p,
    unsigned int knotindex,
    int tagId,
    bool driving
)
{
    if (b.periodic && knotindex == 0) {
        // This is done here since knotpoints themselves aren't stored
        addConstraintP2PCoincident(p, b.start, tagId, driving);
        addConstraintP2PCoincident(p, b.end, tagId, driving);
    }

    size_t numpoles = b.degree - b.mult[knotindex] + 1;
    if (numpoles == 0) {
        numpoles = 1;
    }

    // `startpole` is the first pole affecting the knot with `knotindex`
    size_t startpole = 0;
    std::vector<double*> pvec;
    pvec.push_back(p.x);

    std::vector<double> factors(numpoles, 1.0 / numpoles);

    // Only poles with indices `[i, i+1,... i+b.degree]` affect the interval
    // `flattenedknots[b.degree+i]` to `flattenedknots[b.degree+i+1]`.
    // When a knot has higher multiplicity, it can be seen as spanning
    // multiple of these intervals, and thus is only affected by an
    // intersection of the poles that affect it.
    // The `knotindex` gives us the intervals, so work backwards and find
    // the affecting poles.
    // Note that this works also for periodic B-splines, just that the poles wrap around if needed.
    for (size_t j = 1; j <= knotindex; ++j) {
        startpole += b.mult[j];
    }
    // For the last knot, even the upper limit of the last interval range
    // is included. So offset for that.
    // For periodic B-splines the `flattenedknots` are defined differently,
    // so this is not needed for them.
    if (!b.periodic && startpole >= b.poles.size()) {
        startpole = b.poles.size() - 1;
    }

    // Calculate the factors to be passed to weighted linear combination constraint.
    // The if condition has a small performance benefit, but that is not why it is here.
    // One case when numpoles <= 1 is for the last knot of a non-periodic B-spline.
    // In this case, the interval `k` passed to `getLinCombFactor` is degenerate, and this is the
    // cleanest way to handle it.
    if (numpoles > 1) {
        for (size_t i = 0; i < numpoles; ++i) {
            factors[i] = b.getLinCombFactor(*(b.knots[knotindex]), startpole + b.degree, startpole + i);
        }
    }

    // The mod operation is to adjust for periodic B-splines.
    // This can be separated for performance reasons but it will be less readable.
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.poles[(startpole + i) % b.poles.size()].x);
    }
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.weights[(startpole + i) % b.poles.size()]);
    }

    Constraint* constr = new ConstraintWeightedLinearCombination(numpoles, pvec, factors);
    constr->setTag(tagId);
    constr->setDriving(driving);
    constr->setInternalAlignment(Constraint::Alignment::InternalAlignment);
    addConstraint(constr);

    pvec.clear();
    pvec.push_back(p.y);
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.poles[(startpole + i) % b.poles.size()].y);
    }
    for (size_t i = 0; i < numpoles; ++i) {
        pvec.push_back(b.weights[(startpole + i) % b.poles.size()]);
    }

    constr = new ConstraintWeightedLinearCombination(numpoles, pvec, factors);
    constr->setTag(tagId);
    constr->setDriving(driving);
    constr->setInternalAlignment(Constraint::Alignment::InternalAlignment);
    return addConstraint(constr);
}

// calculates angle between two curves at point of their intersection p. If two
// points are supplied, p is used for first curve and p2 for second, yielding a
// remote angle computation (this is useful when the endpoints haven't) been
// made coincident yet
double System::calculateAngleViaPoint(const Curve& crv1, const Curve& crv2, Point& p) const
{
    return calculateAngleViaPoint(crv1, crv2, p, p);
}

double System::calculateAngleViaPoint(const Curve& crv1, const Curve& crv2, Point& p1, Point& p2) const
{
    GCS::DeriVector2 n1 = crv1.CalculateNormal(p1);
    GCS::DeriVector2 n2 = crv2.CalculateNormal(p2);
    return atan2(-n2.x * n1.y + n2.y * n1.x, n2.x * n1.x + n2.y * n1.y);
}

double System::calculateAngleViaParams(
    const Curve& crv1,
    const Curve& crv2,
    double* param1,
    double* param2
) const
{
    GCS::DeriVector2 n1 = crv1.CalculateNormal(param1);
    GCS::DeriVector2 n2 = crv2.CalculateNormal(param2);
    return atan2(-n2.x * n1.y + n2.y * n1.x, n2.x * n1.x + n2.y * n1.y);
}

void System::calculateNormalAtPoint(const Curve& crv, const Point& p, double& rtnX, double& rtnY) const
{
    GCS::DeriVector2 n1 = crv.CalculateNormal(p);
    rtnX = n1.x;
    rtnY = n1.y;
}

double System::calculateConstraintErrorByTag(int tagId)
{
    int cnt = 0;         // how many constraints have been accumulated
    double sqErr = 0.0;  // accumulator of squared errors
    double err = 0.0;    // last computed signed error value

    for (const auto& constr : clist) {
        if (constr->getTag() == tagId) {
            err = constr->error();
            sqErr += err * err;
            cnt++;
        };
    }
    switch (cnt) {
        case 0:  // constraint not found!
            return std::numeric_limits<double>::quiet_NaN();
            break;
        case 1:
            return err;
            break;
        default:
            return sqrt(sqErr / (double)cnt);
    }
}

void System::rescaleConstraint(int id, double coeff)
{
    if (id >= static_cast<int>(clist.size()) || id < 0) {
        return;
    }
    if (clist[id]) {
        clist[id]->rescale(coeff);
    }
}

void System::declareUnknowns(VEC_pD& params)
{
    plist = params;
    pIndex.clear();
    for (int i = 0; i < int(plist.size()); ++i) {
        pIndex[plist[i]] = i;
    }
    hasUnknowns = true;
}

void System::declareDrivenParams(VEC_pD& params)
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
    if (!hasUnknowns) {
        return;
    }

    // storing reference configuration
    setReference();

    // diagnose conflicting or redundant constraints
    if (!hasDiagnosis) {
        diagnose(alg);
    }

    // if still no diagnosis after explicitly calling `diagnose`, nothing to do here
    if (!hasDiagnosis) {
        return;
    }

    std::vector<Constraint*> clistR;
    if (!redundant.empty()) {
        std::ranges::copy_if(clist, std::back_inserter(clistR), [this](auto constr) {
            return this->redundant.count(constr) == 0 && constr->isDriving();
        });
    }
    else {
        std::ranges::copy_if(clist, std::back_inserter(clistR), [this](auto constr) {
            return constr->isDriving();
        });
    }

    // partitioning into decoupled components
    Graph g;
    for (int i = 0; i < int(plist.size() + clistR.size()); i++) {
        boost::add_vertex(g);
    }

    int cvtid = int(plist.size());
    for (const auto constr : clistR) {
        VEC_pD& cparams = c2p[constr];
        for (const auto param : cparams) {
            MAP_pD_I::const_iterator it = pIndex.find(param);
            if (it != pIndex.end()) {
                boost::add_edge(cvtid, it->second, g);
            }
        }
        ++cvtid;
    }

    VEC_I components(boost::num_vertices(g));
    int componentsSize = 0;
    if (!components.empty()) {
        componentsSize = boost::connected_components(g, &components[0]);
    }

    // identification of equality constraints and parameter reduction
    std::set<Constraint*> reducedConstrs;  // constraints that will be eliminated through reduction
    reductionmaps.clear();                 // destroy any maps
    reductionmaps.resize(componentsSize);  // create empty maps to be filled in
    {
        VEC_pD reducedParams = plist;

        for (const auto& constr : clistR) {
            if (!(constr->getTag() >= 0 && constr->getTypeId() == Equal)) {
                continue;
            }
            const auto it1 = pIndex.find(constr->params()[0]);
            const auto it2 = pIndex.find(constr->params()[1]);
            if (it1 == pIndex.end() || it2 == pIndex.end()) {
                continue;
            }
            reducedConstrs.insert(constr);
            double* p_kept = reducedParams[it1->second];
            double* p_replaced = reducedParams[it2->second];
            std::ranges::replace(reducedParams, p_replaced, p_kept);
        }
        for (size_t i = 0; i < plist.size(); ++i) {
            if (plist[i] != reducedParams[i]) {
                int cid = components[i];
                reductionmaps[cid][plist[i]] = reducedParams[i];
            }
        }
    }

    // TODO: Why are the later (constraint-related) items added first?
    // Adding plist-related items first would simplify assignment of `i`, but is not a big expense
    // overall. Leaving as is to avoid any unintended consequences.
    clists.clear();                 // destroy any lists
    clists.resize(componentsSize);  // create empty lists to be filled in
    size_t i = plist.size();
    for (const auto& constr : clistR) {
        if (reducedConstrs.count(constr) == 0) {
            int cid = components[i];
            clists[cid].push_back(constr);
        }
        ++i;
    }

    plists.clear();                 // destroy any lists
    plists.resize(componentsSize);  // create empty lists to be filled in
    for (size_t i = 0; i < plist.size(); ++i) {
        int cid = components[i];
        plists[cid].push_back(plist[i]);
    }

    // calculates subSystems and subSystemsAux from clists, plists and reductionmaps
    clearSubSystems();
    subSystems.resize(clists.size(), nullptr);
    subSystemsAux.resize(clists.size(), nullptr);
    for (std::size_t cid = 0; cid < clists.size(); ++cid) {
        std::vector<Constraint*> clist0, clist1;
        std::ranges::partition_copy(
            clists[cid],
            std::back_inserter(clist0),
            std::back_inserter(clist1),
            [](auto constr) { return constr->getTag() >= 0; }
        );

        if (!clist0.empty()) {
            subSystems[cid] = new SubSystem(clist0, plists[cid], reductionmaps[cid]);
        }
        if (!clist1.empty()) {
            subSystemsAux[cid] = new SubSystem(clist1, plists[cid], reductionmaps[cid]);
        }
    }

    isInit = true;
}

void System::setReference()
{
    reference.clear();
    reference.reserve(plist.size());
    for (const auto& param : plist) {
        reference.push_back(*param);
    }
}

void System::resetToReference()
{
    if (reference.size() == plist.size()) {
        VEC_D::const_iterator ref = reference.begin();
        VEC_pD::iterator param = plist.begin();
        for (; ref != reference.end(); ++ref, ++param) {
            **param = *ref;
        }
    }
}

int System::solve(VEC_pD& params, bool isFine, Algorithm alg, bool isRedundantsolving)
{
    declareUnknowns(params);
    initSolution();
    return solve(isFine, alg, isRedundantsolving);
}

int System::solve(bool isFine, Algorithm alg, bool isRedundantsolving)
{
    if (!isInit) {
        return Failed;
    }

    bool isReset = false;
    // return success by default in order to permit coincidence constraints to be applied
    // even if no other system has to be solved
    int res = Success;
    for (int cid = 0; cid < int(subSystems.size()); cid++) {
        if ((subSystems[cid] || subSystemsAux[cid]) && !isReset) {
            resetToReference();
            isReset = true;
        }
        if (subSystems[cid] && subSystemsAux[cid]) {
            res = std::max(res, solve(subSystems[cid], subSystemsAux[cid], isFine, isRedundantsolving));
        }
        else if (subSystems[cid]) {
            res = std::max(res, solve(subSystems[cid], isFine, alg, isRedundantsolving));
        }
        else if (subSystemsAux[cid]) {
            res = std::max(res, solve(subSystemsAux[cid], isFine, alg, isRedundantsolving));
        }
    }
    if (res == Success) {
        for (std::set<Constraint*>::const_iterator constr = redundant.begin();
             constr != redundant.end();
             ++constr) {
            // DeepSOIC: there used to be a comparison of signed error value to
            // convergence, which makes no sense. Potentially I fixed bug, and
            // chances are low I've broken anything.
            double err = (*constr)->error();
            if (err * err > (isRedundantsolving ? convergenceRedundant : convergence)) {
                res = Converged;
                return res;
            }
        }
    }
    return res;
}

int System::solve(SubSystem* subsys, bool isFine, Algorithm alg, bool isRedundantsolving)
{
    if (alg == BFGS) {
        return solve_BFGS(subsys, isFine, isRedundantsolving);
    }
    else if (alg == LevenbergMarquardt) {
        return solve_LM(subsys, isRedundantsolving);
    }
    else if (alg == DogLeg) {
        return solve_DL(subsys, isRedundantsolving);
    }
    else {
        return Failed;
    }
}

int System::solve_BFGS(SubSystem* subsys, bool /*isFine*/, bool isRedundantsolving)
{
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
#endif

    int xsize = subsys->pSize();
    if (xsize == 0) {
        return Success;
    }

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
    h = x - h;  // = x - xold

    // double convergence = isFine ? convergence : XconvergenceRough;
    int maxIterNumber = (sketchSizeMultiplier ? maxIter * xsize : maxIter);
    double convCriterion = convergence;
    if (isRedundantsolving) {
        maxIterNumber = (sketchSizeMultiplierRedundant ? maxIterRedundant * xsize : maxIterRedundant);
        convCriterion = convergenceRedundant;
    }

    if (debugMode == IterationLevel) {
        std::stringstream stream;
        stream << "BFGS: convergence: " << convCriterion << ", xsize: " << xsize
               << ", maxIter: " << maxIterNumber << "\n";

        const std::string tmp = stream.str();
        Base::Console().log(tmp.c_str());
    }

    double divergingLim = 1e6 * err + 1e12;
    double h_norm {};

    for (int iter = 1; iter < maxIterNumber; ++iter) {
        h_norm = h.norm();
        if (h_norm <= convCriterion || err <= smallF) {
            if (debugMode == IterationLevel) {
                std::stringstream stream;
                stream << "BFGS Converged!!: "
                       << ", err: " << err << ", h_norm: " << h_norm << "\n";

                const std::string tmp = stream.str();
                Base::Console().log(tmp.c_str());
            }
            break;
        }
        if (err > divergingLim || err != err) {
            // check for diverging and NaN
            if (debugMode == IterationLevel) {
                std::stringstream stream;
                stream << "BFGS Failed: Diverging!!: "
                       << ", err: " << err << ", divergingLim: " << divergingLim << "\n";

                const std::string tmp = stream.str();
                Base::Console().log(tmp.c_str());
            }
            break;
        }

        y = grad;
        subsys->calcGrad(grad);
        y = grad - y;  // = grad - gradold

        double hty = h.dot(y);
        // make sure that hty is never 0
        if (hty == 0) {
            hty = .0000000001;
        }

        Dy = D * y;

        double ytDy = y.dot(Dy);

        // Now calculate the BFGS update on D
        D += (1. + ytDy / hty) / hty * h * h.transpose();
        D -= 1. / hty * (h * Dy.transpose() + Dy * h.transpose());

        xdir = -D * grad;
        lineSearch(subsys, xdir);
        err = subsys->error();

        h = x;
        subsys->getParams(x);
        h = x - h;  // = x - xold

        if (debugMode == IterationLevel) {
            std::stringstream stream;
            stream << "BFGS, Iteration: " << iter << ", err: " << err << ", h_norm: " << h_norm
                   << "\n";

            const std::string tmp = stream.str();
            Base::Console().log(tmp.c_str());
        }
    }

    subsys->revertParams();

    if (err <= smallF) {
        return Success;
    }
    if (h.norm() <= convCriterion) {
        return Converged;
    }
    return Failed;
}

int System::solve_LM(SubSystem* subsys, bool isRedundantsolving)
{
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
#endif

    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    if (xsize == 0) {
        return Success;
    }

    Eigen::VectorXd e(csize),
        e_new(csize);  // vector of all function errors (every constraint is one function)
    Eigen::MatrixXd J(csize, xsize);  // Jacobi of the subsystem
    Eigen::MatrixXd A(xsize, xsize);
    Eigen::VectorXd x(xsize), h(xsize), x_new(xsize), g(xsize), diag_A(xsize);

    subsys->redirectParams();

    subsys->getParams(x);
    subsys->calcResidual(e);
    e *= -1;

    int maxIterNumber = (sketchSizeMultiplier ? maxIter * xsize : maxIter);

    double divergingLim = 1e6 * e.squaredNorm() + 1e12;

    double eps = LM_eps;
    double eps1 = LM_eps1;
    double tau = LM_tau;

    if (isRedundantsolving) {
        maxIterNumber = (sketchSizeMultiplierRedundant ? maxIterRedundant * xsize : maxIterRedundant);
        eps = LM_epsRedundant;
        eps1 = LM_eps1Redundant;
        tau = LM_tauRedundant;
    }

    if (debugMode == IterationLevel) {
        std::stringstream stream;
        stream << "LM: eps: " << eps << ", eps1: " << eps1 << ", tau: " << tau
               << ", convergence: " << (isRedundantsolving ? convergenceRedundant : convergence)
               << ", xsize: " << xsize << ", maxIter: " << maxIterNumber << "\n";

        const std::string tmp = stream.str();
        Base::Console().log(tmp.c_str());
    }

    double nu = 2, mu = 0;
    int iter = 0, stop = 0;
    for (iter = 0; iter < maxIterNumber && !stop; ++iter) {
        // check error
        double err = e.squaredNorm();
        if (err <= eps * eps) {
            // error is small, Success
            stop = 1;
            break;
        }
        else if (err > divergingLim || err != err) {
            // check for diverging and NaN
            stop = 6;
            break;
        }

        // J^T J, J^T e
        subsys->calcJacobi(J);

        A = J.transpose() * J;
        g = J.transpose() * e;

        // Compute ||J^T e||_inf
        double g_inf = g.lpNorm<Eigen::Infinity>();
        diag_A = A.diagonal();  // save diagonal entries so that augmentation can be later canceled

        // check for convergence
        if (g_inf <= eps1) {
            stop = 2;
            break;
        }

        // compute initial damping factor
        if (iter == 0) {
            mu = tau * diag_A.lpNorm<Eigen::Infinity>();
        }

        double h_norm {};
        // determine increment using adaptive damping
        int k = 0;
        while (k < 50) {
            // augment normal equations A = A+uI
            for (int i = 0; i < xsize; ++i) {
                A(i, i) += mu;
            }

            // solve augmented functions A*h=-g
            h = A.fullPivLu().solve(g);
            double rel_error = (A * h - g).norm() / g.norm();

            // check if solving works
            if (rel_error < 1e-5) {
                // restrict h according to maxStep
                double scale = subsys->maxStep(h);
                if (scale < 1.) {
                    h *= scale;
                }

                // compute par's new estimate and ||d_par||^2
                x_new = x + h;
                h_norm = h.squaredNorm();

                constexpr double epsilon = std::numeric_limits<double>::epsilon();
                if (h_norm <= eps1 * eps1 * x.norm()) {
                    // relative change in p is small, stop
                    stop = 3;
                    break;
                }
                else if (h_norm >= (x.norm() + eps1) / (epsilon * epsilon)) {
                    // almost singular
                    stop = 4;
                    break;
                }

                subsys->setParams(x_new);
                subsys->calcResidual(e_new);
                e_new *= -1;

                double dF = e.squaredNorm() - e_new.squaredNorm();
                double dL = h.dot(mu * h + g);

                if (dF > 0. && dL > 0.) {  // reduction in error, increment is accepted
                    double tmp = 2 * dF / dL - 1.;
                    mu *= std::max(1. / 3., 1. - tmp * tmp * tmp);
                    nu = 2;

                    // update par's estimate
                    x = x_new;
                    e = e_new;
                    break;
                }
            }

            // if this point is reached, either the linear system could not be solved or
            // the error did not reduce; in any case, the increment must be rejected

            mu *= nu;
            nu *= 2.0;
            for (int i = 0; i < xsize; ++i) {  // restore diagonal J^T J entries
                A(i, i) = diag_A(i);
            }

            k++;
        }
        if (k > 50) {
            stop = 7;
            break;
        }

        if (debugMode == IterationLevel) {
            std::stringstream stream;
            // Iteration: 1, residual: 1e-3, tolg: 1e-5, tolx: 1e-3
            stream << "LM, Iteration: " << iter << ", err(eps): " << err
                   << ", g_inf(eps1): " << g_inf << ", h_norm: " << h_norm << "\n";

            const std::string tmp = stream.str();
            Base::Console().log(tmp.c_str());
        }
    }

    if (iter >= maxIterNumber) {
        stop = 5;
    }

    subsys->revertParams();

    return (stop == 1) ? Success : Failed;
}

int System::solve_DL(SubSystem* subsys, bool isRedundantsolving)
{
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    extractSubsystem(subsys, isRedundantsolving);
#endif

    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    if (xsize == 0) {
        return Success;
    }

    double tolg = DL_tolg;
    double tolx = DL_tolx;
    double tolf = DL_tolf;

    int maxIterNumber = (sketchSizeMultiplier ? maxIter * xsize : maxIter);
    if (isRedundantsolving) {
        tolg = DL_tolgRedundant;
        tolx = DL_tolxRedundant;
        tolf = DL_tolfRedundant;

        maxIterNumber = (sketchSizeMultiplierRedundant ? maxIterRedundant * xsize : maxIterRedundant);
    }

    if (debugMode == IterationLevel) {
        std::stringstream stream;
        stream << "DL: tolg: " << tolg << ", tolx: " << tolx << ", tolf: " << tolf
               << ", convergence: " << (isRedundantsolving ? convergenceRedundant : convergence)
               << ", dogLegGaussStep: "
               << (dogLegGaussStep == FullPivLU
                       ? "FullPivLU"
                       : (dogLegGaussStep == LeastNormFullPivLU ? "LeastNormFullPivLU"
                                                                : "LeastNormLdlt"))
               << ", xsize: " << xsize << ", csize: " << csize << ", maxIter: " << maxIterNumber
               << "\n";

        const std::string tmp = stream.str();
        Base::Console().log(tmp.c_str());
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

    g = Jx.transpose() * (-fx);

    // get the infinity norm fx_inf and g_inf
    double g_inf = g.lpNorm<Eigen::Infinity>();
    double fx_inf = fx.lpNorm<Eigen::Infinity>();

    double divergingLim = 1e6 * err + 1e12;

    double delta = 0.1;
    double alpha = 0.;
    double nu = 2.;
    int iter = 0, stop = 0, reduce = 0;
    while (!stop) {
        // check if finished
        if (fx_inf <= tolf) {
            // Success
            stop = 1;
            break;
        }
        else if (g_inf <= tolg) {
            stop = 2;
            break;
        }
        else if (delta <= tolx * (tolx + x.norm())) {
            stop = 2;
            break;
        }
        else if (iter >= maxIterNumber) {
            stop = 4;
            break;
        }
        else if (err > divergingLim || err != err) {
            // check for diverging and NaN
            stop = 6;
            break;
        }

        // get the steepest descent direction
        alpha = g.squaredNorm() / (Jx * g).squaredNorm();
        h_sd = alpha * g;

        // get the gauss-newton step
        // https://forum.freecad.org/viewtopic.php?f=10&t=12769&start=50#p106220
        // https://forum.kde.org/viewtopic.php?f=74&t=129439#p346104
        switch (dogLegGaussStep) {
            case FullPivLU:
                h_gn = Jx.fullPivLu().solve(-fx);
                break;
            case LeastNormFullPivLU:
                h_gn = Jx.adjoint() * (Jx * Jx.adjoint()).fullPivLu().solve(-fx);
                break;
            case LeastNormLdlt:
                h_gn = Jx.adjoint() * (Jx * Jx.adjoint()).ldlt().solve(-fx);
                break;
        }

        double rel_error = (Jx * h_gn + fx).norm() / fx.norm();
        if (rel_error > 1e15) {
            break;
        }

        // compute the dogleg step
        if (h_gn.norm() < delta) {
            h_dl = h_gn;
            if (h_dl.norm() <= tolx * (tolx + x.norm())) {
                stop = 5;
                break;
            }
        }
        else if (alpha * g.norm() >= delta) {
            h_dl = (delta / (alpha * g.norm())) * h_sd;
        }
        else {
            // compute beta
            double beta = 0;
            Eigen::VectorXd b = h_gn - h_sd;
            double bb = (b.transpose() * b).norm();
            double gb = (h_sd.transpose() * b).norm();
            double c = (delta + h_sd.norm()) * (delta - h_sd.norm());

            if (gb > 0) {
                beta = c / (gb + sqrt(gb * gb + c * bb));
            }
            else {
                beta = (sqrt(gb * gb + c * bb) - gb) / bb;
            }

            // and update h_dl and dL with beta
            h_dl = h_sd + beta * b;
        }

        // get the new values
        double err_new;
        x_new = x + h_dl;
        subsys->setParams(x_new);
        subsys->calcResidual(fx_new, err_new);
        subsys->calcJacobi(Jx_new);

        // calculate the linear model and the update ratio
        double dL = err - 0.5 * (fx + Jx * h_dl).squaredNorm();
        double dF = err - err_new;
        double rho = dL / dF;

        if (dF > 0 && dL > 0) {
            x = x_new;
            Jx = Jx_new;
            fx = fx_new;
            err = err_new;

            g = Jx.transpose() * (-fx);

            // get infinity norms
            g_inf = g.lpNorm<Eigen::Infinity>();
            fx_inf = fx.lpNorm<Eigen::Infinity>();
        }
        else {
            rho = -1;
        }

        // update delta
        if (fabs(rho - 1.) < 0.2 && h_dl.norm() > delta / 3. && reduce <= 0) {
            delta = 3 * delta;
            nu = 2;
            reduce = 0;
        }
        else if (rho < 0.25) {
            delta = delta / nu;
            nu = 2 * nu;
            reduce = 2;
        }
        else {
            reduce--;
        }

        if (debugMode == IterationLevel) {
            std::stringstream stream;
            // Iteration: 1, residual: 1e-3, tolg: 1e-5, tolx: 1e-3
            stream << "DL, Iteration: " << iter << ", fx_inf(tolf): " << fx_inf
                   << ", g_inf(tolg): " << g_inf << ", delta(f(tolx)): " << delta
                   << ", err(divergingLim): " << err << "\n";

            const std::string tmp = stream.str();
            Base::Console().log(tmp.c_str());
        }

        // count this iteration and start again
        iter++;
    }

    subsys->revertParams();

    if (debugMode == IterationLevel) {
        std::stringstream stream;
        stream << "DL: stopcode: " << stop << ((stop == 1) ? ", Success" : ", Failed") << "\n";

        const std::string tmp = stream.str();
        Base::Console().log(tmp.c_str());
    }

    return (stop == 1) ? Success : Failed;
}

#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
void System::extractSubsystem(SubSystem* subsys, bool isRedundantsolving)
{
    VEC_pD plistout;  // std::vector<double *>
    std::vector<Constraint*> clist_;
    VEC_pD clist_params_;

    subsys->getParamList(plistout);
    subsys->getConstraintList(clist_);

    std::ofstream subsystemfile;
    subsystemfile.open("subsystemfile.txt", std::ofstream::out | std::ofstream::app);
    subsystemfile << std::endl;
    subsystemfile << std::endl;
    subsystemfile << "Solving: " << (isRedundantsolving ? "Redundant" : "Normal")
                  << " Subsystem Dump starts here................................" << std::endl;

    int ip = 0;

    subsystemfile << "GCS::VEC_pD plist_;" << std::endl;                     // all SYSTEM params
    subsystemfile << "std::vector<GCS::Constraint *> clist_;" << std::endl;  // SUBSYSTEM constraints
    subsystemfile << "GCS::VEC_pD plistsub_;" << std::endl;                  // all SUBSYSTEM params
    // constraint params not within SYSTEM params
    subsystemfile << "GCS::VEC_pD clist_params_;" << std::endl;

    // these are all the parameters, including those not in the subsystem to
    // which constraints in the subsystem may make reference.
    for (VEC_pD::iterator it = plist.begin(); it != plist.end(); ++it, ++ip) {
        subsystemfile << "plist_.push_back(new double(" << *(*it) << ")); // " << ip
                      << " address: " << (void*)(*it) << std::endl;
    }

    int ips = 0;
    for (VEC_pD::iterator it = plistout.begin(); it != plistout.end(); ++it, ++ips) {
        VEC_pD::iterator p = std::ranges::find(plist, (*it));
        size_t p_index = std::distance(plist.begin(), p);

        if (p_index == plist.size()) {
            subsystemfile << "// Error: Subsystem parameter not in system params"
                          << "address: " << (void*)(*it) << std::endl;
        }

        subsystemfile << "plistsub_.push_back(plist_[" << p_index << "]); // " << ips << std::endl;
    }

    int ic = 0;   // constraint index
    int icp = 0;  // index of constraint params not within SYSTEM params
    for (std::vector<Constraint*>::iterator it = clist_.begin(); it != clist_.end(); ++it, ++ic) {

        switch ((*it)->getTypeId()) {
            case Equal: {  // 2
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                subsystemfile << "clist_.push_back(new ConstraintEqual("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]," << (npb2 ? ("clist_params_[") : ("plist_["))
                              << (npb2 ? ni2 : i2) << "])); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << std::endl;
                break;
            }
            case Difference: {  // 3
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                subsystemfile << "clist_.push_back(new ConstraintDifference("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]," << (npb2 ? ("clist_params_[") : ("plist_["))
                              << (npb2 ? ni2 : i2) << "],"
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "])); // addresses = " << (*it)->pvec[0] << "," << (*it)->pvec[1]
                              << "," << (*it)->pvec[2] << std::endl;
                break;
            }
            case P2PDistance: {  // 5
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                subsystemfile << "ConstraintP2PDistance * c" << ic
                              << "=new ConstraintP2PDistance();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << std::endl;
                break;
            }
            case P2PAngle: {  // 5
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                subsystemfile << "ConstraintP2PAngle * c" << ic << "=new ConstraintP2PAngle();"
                              << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << std::endl;
                break;
            }
            case P2LDistance: {  // 7
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                subsystemfile << "ConstraintP2LDistance * c" << ic
                              << "=new ConstraintP2LDistance();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5]
                              << "," << (*it)->pvec[6] << std::endl;
                break;
            }
            case PointOnLine: {  // 6
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                subsystemfile << "ConstraintPointOnLine * c" << ic
                              << "=new ConstraintPointOnLine();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic
                              << "); // addresses = " << (*it)->pvec[0] << "," << (*it)->pvec[1]
                              << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << ","
                              << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case PointOnPerpBisector: {  // 6
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                subsystemfile << "ConstraintPointOnPerpBisector * c" << ic
                              << "=new ConstraintPointOnPerpBisector();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic
                              << "); // addresses = " << (*it)->pvec[0] << "," << (*it)->pvec[1]
                              << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << ","
                              << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case Parallel: {  // 8
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                VEC_pD::iterator p8 = std::ranges::find(plist, (*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                bool npb8 = false;
                VEC_pD::iterator np8 = std::ranges::find(clist_params_, (*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if (i8 == plist.size()) {
                    if (ni8 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[7])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[7]
                                      << std::endl;
                        icp++;
                    }
                    npb8 = true;
                }

                subsystemfile << "ConstraintParallel * c" << ic << "=new ConstraintParallel();"
                              << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb8 ? ("clist_params_[") : ("plist_[")) << (npb8 ? ni8 : i8)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5]
                              << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case Perpendicular: {  // 8
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                VEC_pD::iterator p8 = std::ranges::find(plist, (*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                bool npb8 = false;
                VEC_pD::iterator np8 = std::ranges::find(clist_params_, (*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if (i8 == plist.size()) {
                    if (ni8 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[7])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[7]
                                      << std::endl;
                        icp++;
                    }
                    npb8 = true;
                }

                subsystemfile << "ConstraintPerpendicular * c" << ic
                              << "=new ConstraintPerpendicular();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb8 ? ("clist_params_[") : ("plist_[")) << (npb8 ? ni8 : i8)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5]
                              << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case L2LAngle: {  // 9
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                VEC_pD::iterator p8 = std::ranges::find(plist, (*it)->pvec[7]);
                VEC_pD::iterator p9 = std::ranges::find(plist, (*it)->pvec[8]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);
                size_t i9 = std::distance(plist.begin(), p9);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                bool npb8 = false;
                VEC_pD::iterator np8 = std::ranges::find(clist_params_, (*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if (i8 == plist.size()) {
                    if (ni8 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[7])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[7]
                                      << std::endl;
                        icp++;
                    }
                    npb8 = true;
                }

                bool npb9 = false;
                VEC_pD::iterator np9 = std::ranges::find(clist_params_, (*it)->pvec[8]);
                size_t ni9 = std::distance(clist_params_.begin(), np9);

                if (i9 == plist.size()) {
                    if (ni9 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[8]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[8])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[8]
                                      << std::endl;
                        icp++;
                    }
                    npb9 = true;
                }

                subsystemfile << "ConstraintL2LAngle * c" << ic << "=new ConstraintL2LAngle();"
                              << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb8 ? ("clist_params_[") : ("plist_[")) << (npb8 ? ni8 : i8)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb9 ? ("clist_params_[") : ("plist_[")) << (npb9 ? ni9 : i9)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic
                              << "); // addresses = " << (*it)->pvec[0] << "," << (*it)->pvec[1]
                              << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << ","
                              << (*it)->pvec[4] << "," << (*it)->pvec[5] << "," << (*it)->pvec[6]
                              << "," << (*it)->pvec[7] << "," << (*it)->pvec[8] << std::endl;
                break;
            }
            case MidpointOnLine: {  // 8
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                VEC_pD::iterator p8 = std::ranges::find(plist, (*it)->pvec[7]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);
                size_t i8 = std::distance(plist.begin(), p8);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                bool npb8 = false;
                VEC_pD::iterator np8 = std::ranges::find(clist_params_, (*it)->pvec[7]);
                size_t ni8 = std::distance(clist_params_.begin(), np8);

                if (i8 == plist.size()) {
                    if (ni8 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[7]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[7])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[7]
                                      << std::endl;
                        icp++;
                    }
                    npb8 = true;
                }

                subsystemfile << "ConstraintMidpointOnLine * c" << ic
                              << "=new ConstraintMidpointOnLine();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb8 ? ("clist_params_[") : ("plist_[")) << (npb8 ? ni8 : i8)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5]
                              << "," << (*it)->pvec[6] << "," << (*it)->pvec[7] << std::endl;
                break;
            }
            case TangentCircumf: {  // 6
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                subsystemfile << "ConstraintTangentCircumf * c" << ic
                              << "=new ConstraintTangentCircumf("
                              << (static_cast<ConstraintTangentCircumf*>(*it)->getInternal()
                                      ? "true"
                                      : "false")
                              << ");" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic
                              << "); // addresses = " << (*it)->pvec[0] << "," << (*it)->pvec[1]
                              << "," << (*it)->pvec[2] << "," << (*it)->pvec[3] << ","
                              << (*it)->pvec[4] << "," << (*it)->pvec[5] << std::endl;
                break;
            }
            case PointOnEllipse: {  // 7
                VEC_pD::iterator p1 = std::ranges::find(plist, (*it)->pvec[0]);
                VEC_pD::iterator p2 = std::ranges::find(plist, (*it)->pvec[1]);
                VEC_pD::iterator p3 = std::ranges::find(plist, (*it)->pvec[2]);
                VEC_pD::iterator p4 = std::ranges::find(plist, (*it)->pvec[3]);
                VEC_pD::iterator p5 = std::ranges::find(plist, (*it)->pvec[4]);
                VEC_pD::iterator p6 = std::ranges::find(plist, (*it)->pvec[5]);
                VEC_pD::iterator p7 = std::ranges::find(plist, (*it)->pvec[6]);
                size_t i1 = std::distance(plist.begin(), p1);
                size_t i2 = std::distance(plist.begin(), p2);
                size_t i3 = std::distance(plist.begin(), p3);
                size_t i4 = std::distance(plist.begin(), p4);
                size_t i5 = std::distance(plist.begin(), p5);
                size_t i6 = std::distance(plist.begin(), p6);
                size_t i7 = std::distance(plist.begin(), p7);

                bool npb1 = false;
                VEC_pD::iterator np1 = std::ranges::find(clist_params_, (*it)->pvec[0]);
                size_t ni1 = std::distance(clist_params_.begin(), np1);

                if (i1 == plist.size()) {
                    if (ni1 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[0]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[0])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[0]
                                      << std::endl;
                        icp++;
                    }
                    npb1 = true;
                }

                bool npb2 = false;
                VEC_pD::iterator np2 = std::ranges::find(clist_params_, (*it)->pvec[1]);
                size_t ni2 = std::distance(clist_params_.begin(), np2);

                if (i2 == plist.size()) {
                    if (ni2 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[1]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[1])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[1]
                                      << std::endl;
                        icp++;
                    }
                    npb2 = true;
                }

                bool npb3 = false;
                VEC_pD::iterator np3 = std::ranges::find(clist_params_, (*it)->pvec[2]);
                size_t ni3 = std::distance(clist_params_.begin(), np3);

                if (i3 == plist.size()) {
                    if (ni3 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[2]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[2])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[2]
                                      << std::endl;
                        icp++;
                    }
                    npb3 = true;
                }

                bool npb4 = false;
                VEC_pD::iterator np4 = std::ranges::find(clist_params_, (*it)->pvec[3]);
                size_t ni4 = std::distance(clist_params_.begin(), np4);

                if (i4 == plist.size()) {
                    if (ni4 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[3]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[3])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[3]
                                      << std::endl;
                        icp++;
                    }
                    npb4 = true;
                }

                bool npb5 = false;
                VEC_pD::iterator np5 = std::ranges::find(clist_params_, (*it)->pvec[4]);
                size_t ni5 = std::distance(clist_params_.begin(), np5);

                if (i5 == plist.size()) {
                    if (ni5 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[4]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[4])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[4]
                                      << std::endl;
                        icp++;
                    }
                    npb5 = true;
                }

                bool npb6 = false;
                VEC_pD::iterator np6 = std::ranges::find(clist_params_, (*it)->pvec[5]);
                size_t ni6 = std::distance(clist_params_.begin(), np6);

                if (i6 == plist.size()) {
                    if (ni6 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[5]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[5])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[5]
                                      << std::endl;
                        icp++;
                    }
                    npb6 = true;
                }

                bool npb7 = false;
                VEC_pD::iterator np7 = std::ranges::find(clist_params_, (*it)->pvec[6]);
                size_t ni7 = std::distance(clist_params_.begin(), np7);

                if (i7 == plist.size()) {
                    if (ni7 == clist_params_.size()) {
                        subsystemfile
                            << "// Address not in System params...rebuilding into clist_params_"
                            << std::endl;
                        clist_params_.push_back((*it)->pvec[6]);
                        subsystemfile << "clist_params_.push_back(new double(" << *((*it)->pvec[6])
                                      << ")); // " << icp << " address: " << (void*)(*it)->pvec[6]
                                      << std::endl;
                        icp++;
                    }
                    npb7 = true;
                }

                subsystemfile << "ConstraintPointOnEllipse * c" << ic
                              << "=new ConstraintPointOnEllipse();" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb1 ? ("clist_params_[") : ("plist_[")) << (npb1 ? ni1 : i1)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb2 ? ("clist_params_[") : ("plist_[")) << (npb2 ? ni2 : i2)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb3 ? ("clist_params_[") : ("plist_[")) << (npb3 ? ni3 : i3)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb4 ? ("clist_params_[") : ("plist_[")) << (npb4 ? ni4 : i4)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb5 ? ("clist_params_[") : ("plist_[")) << (npb5 ? ni5 : i5)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb6 ? ("clist_params_[") : ("plist_[")) << (npb6 ? ni6 : i6)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->pvec.push_back("
                              << (npb7 ? ("clist_params_[") : ("plist_[")) << (npb7 ? ni7 : i7)
                              << "]);" << std::endl;
                subsystemfile << "c" << ic << "->origpvec=c" << ic << "->pvec;" << std::endl;
                subsystemfile << "c" << ic << "->rescale();" << std::endl;
                subsystemfile << "clist_.push_back(c" << ic << "); // addresses = " << (*it)->pvec[0]
                              << "," << (*it)->pvec[1] << "," << (*it)->pvec[2] << ","
                              << (*it)->pvec[3] << "," << (*it)->pvec[4] << "," << (*it)->pvec[5]
                              << "," << (*it)->pvec[6] << std::endl;
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
int System::solve(SubSystem* subsysA, SubSystem* subsysB, bool /*isFine*/, bool isRedundantsolving)
{
    int xsizeA = subsysA->pSize();
    int xsizeB = subsysB->pSize();
    int csizeA = subsysA->cSize();

    VEC_pD plistAB(xsizeA + xsizeB);
    {
        VEC_pD plistA, plistB;
        subsysA->getParamList(plistA);
        subsysB->getParamList(plistB);

        std::sort(plistA.begin(), plistA.end());
        std::sort(plistB.begin(), plistB.end());

        VEC_pD::const_iterator it;
        it = std::set_union(plistA.begin(), plistA.end(), plistB.begin(), plistB.end(), plistAB.begin());
        plistAB.resize(it - plistAB.begin());
    }
    int xsize = plistAB.size();

    Eigen::MatrixXd B = Eigen::MatrixXd::Identity(xsize, xsize);
    Eigen::MatrixXd JA(csizeA, xsize);
    Eigen::MatrixXd Y, Z;

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

    subsysB->getParams(plistAB, x);
    subsysA->getParams(plistAB, x);
    subsysB->setParams(plistAB, x);  // just to ensure that A and B are synchronized

    subsysB->calcGrad(plistAB, grad);
    subsysA->calcJacobi(plistAB, JA);
    subsysA->calcResidual(resA);

    // double convergence = isFine ? XconvergenceFine : XconvergenceRough;
    int maxIterNumber
        = (isRedundantsolving
               ? (sketchSizeMultiplierRedundant ? maxIterRedundant * xsize : maxIterRedundant)
               : (sketchSizeMultiplier ? maxIter * xsize : maxIter));

    double divergingLim = 1e6 * subsysA->error() + 1e12;

    double mu = 0;
    lambda.setZero();
    for (int iter = 1; iter < maxIterNumber; iter++) {
        int status = qp_eq(B, grad, JA, resA, xdir, Y, Z);
        if (status) {
            break;
        }

        x0 = x;
        lambda0 = lambda;
        lambda = Y.transpose() * (B * xdir + grad);
        lambdadir = lambda - lambda0;

        // line search
        {
            double eta = 0.25;
            double tau = 0.5;
            double rho = 0.5;
            double alpha = 1;
            alpha = std::min(alpha, subsysA->maxStep(plistAB, xdir));

            // Eq. 18.36
            mu = std::max(
                mu,
                (grad.dot(xdir) + std::max(0., 0.5 * xdir.dot(B * xdir)))
                    / ((1. - rho) * resA.lpNorm<1>())
            );

            // Eq. 18.27
            double f0 = subsysB->error() + mu * resA.lpNorm<1>();

            // Eq. 18.29
            double deriv = grad.dot(xdir) - mu * resA.lpNorm<1>();

            x = x0 + alpha * xdir;
            subsysA->setParams(plistAB, x);
            subsysB->setParams(plistAB, x);
            subsysA->calcResidual(resA);
            double f = subsysB->error() + mu * resA.lpNorm<1>();

            // line search, Eq. 18.28
            bool first = true;
            while (f > f0 + eta * alpha * deriv) {
                if (first) {
                    xdir1 = -Y * resA;
                    x += xdir1;  // = x0 + alpha * xdir + xdir1
                    subsysA->setParams(plistAB, x);
                    subsysB->setParams(plistAB, x);
                    subsysA->calcResidual(resA);
                    f = subsysB->error() + mu * resA.lpNorm<1>();
                    if (f < f0 + eta * alpha * deriv) {
                        break;
                    }
                }
                alpha = tau * alpha;
                if (alpha < 1e-8) {  // let the linesearch fail
                    alpha = 0.;
                }
                x = x0 + alpha * xdir;
                subsysA->setParams(plistAB, x);
                subsysB->setParams(plistAB, x);
                subsysA->calcResidual(resA);
                f = subsysB->error() + mu * resA.lpNorm<1>();
                if (alpha < 1e-8) {  // let the linesearch fail
                    break;
                }
            }
            lambda = lambda0 + alpha * lambdadir;
        }
        h = x - x0;

        y = grad - JA.transpose() * lambda;
        {
            subsysB->calcGrad(plistAB, grad);
            subsysA->calcJacobi(plistAB, JA);
            subsysA->calcResidual(resA);
        }
        y = grad - JA.transpose() * lambda - y;  // Eq. 18.13

        if (iter > 1) {
            double yTh = y.dot(h);
            if (yTh != 0) {
                Bh = B * h;
                // Now calculate the BFGS update on B
                B += 1. / yTh * y * y.transpose();
                B -= 1. / h.dot(Bh) * (Bh * Bh.transpose());
            }
        }

        double err = subsysA->error();
        if (h.norm() <= (isRedundantsolving ? convergenceRedundant : convergence) && err <= smallF) {
            break;
        }
        if (err > divergingLim || err != err) {  // check for diverging and NaN
            break;
        }
    }

    int ret;
    if (subsysA->error() <= smallF) {
        ret = Success;
    }
    else if (h.norm() <= (isRedundantsolving ? convergenceRedundant : convergence)) {
        ret = Converged;
    }
    else {
        ret = Failed;
    }

    subsysA->revertParams();
    subsysB->revertParams();
    return ret;
}

void System::applySolution()
{
    for (int cid = 0; cid < int(subSystems.size()); cid++) {
        if (subSystemsAux[cid]) {
            subSystemsAux[cid]->applySolution();
        }
        if (subSystems[cid]) {
            subSystems[cid]->applySolution();
        }
        for (MAP_pD_pD::const_iterator it = reductionmaps[cid].begin();
             it != reductionmaps[cid].end();
             ++it) {
            *(it->first) = *(it->second);
        }
    }
    evaluateDrivenConstraints();
}
void System::evaluateDrivenConstraints()
{
    for (auto dconstr : drivenConstraints) {
        dconstr->evaluate();
    }
}

void System::undoSolution()
{
    resetToReference();
}

void System::makeReducedJacobian(
    Eigen::MatrixXd& J,
    std::map<int, int>& jacobianconstraintmap,
    GCS::VEC_pD& pdiagnoselist,
    std::map<int, int>& tagmultiplicity
)
{
    // construct specific parameter list for diagonose ignoring driven constraint parameters
    for (int j = 0; j < int(plist.size()); j++) {
        auto result1 = std::ranges::find(pdrivenlist, plist[j]);

        if (result1 == std::end(pdrivenlist)) {
            pdiagnoselist.push_back(plist[j]);
        }
    }


    J = Eigen::MatrixXd::Zero(clist.size(), pdiagnoselist.size());

    int jacobianconstraintcount = 0;
    int allcount = 0;
    for (auto& constr : clist) {
        constr->revertParams();
        ++allcount;
        if (constr->getTag() >= 0 && constr->isDriving()) {
            jacobianconstraintcount++;
            for (int j = 0; j < int(pdiagnoselist.size()); j++) {
                J(jacobianconstraintcount - 1, j) = constr->grad(pdiagnoselist[j]);
            }

            // parallel processing: create tag multiplicity map
            if (tagmultiplicity.find(constr->getTag()) == tagmultiplicity.end()) {
                tagmultiplicity[constr->getTag()] = 0;
            }
            else {
                tagmultiplicity[constr->getTag()]++;
            }

            jacobianconstraintmap[jacobianconstraintcount - 1] = allcount - 1;
        }
    }

    if (jacobianconstraintcount == 0) {  // only driven constraints
        J.resize(0, 0);
    }
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

#ifdef _DEBUG_TO_FILE
    SolverReportingManager::Manager().LogToFile("GCS::System::diagnose()\n");
#endif

    // Input parameters' lists:
    // plist            =>  list of all the parameters of the system, e.g. each coordinate
    //                      of a point
    // pdrivenlist      =>  list of the parameters that are driven by other parameters
    //                      (e.g. value of driven constraints)

    // When adding an external geometry or a constraint on an external geometry the array
    // 'plist' is empty.
    // So, we must abort here because otherwise we would create an invalid matrix and make
    // the application eventually crash. This fixes issues #0002372/#0002373.
    if (plist.empty() || (plist.size() - pdrivenlist.size()) == 0) {
        hasDiagnosis = true;
        emptyDiagnoseMatrix = true;
        dofs = 0;
        return dofs;
    }

    redundant.clear();
    conflictingTags.clear();
    redundantTags.clear();
    partiallyRedundantTags.clear();

    // This QR diagnosis uses a reduced Jacobian matrix to calculate the rank of the system
    // and identify conflicting and redundant constraints.
    //
    // reduced Jacobian matrix
    // The Jacobian has been reduced to:
    // 1. only contain driving constraints, but keep a full size (zero padded).
    // 2. remove the parameters of the values of driven constraints.
    Eigen::MatrixXd J;

    // maps the index of the rows of the reduced jacobian matrix (solver constraints) to
    // the index those constraints would have in a full size Jacobian matrix
    std::map<int, int> jacobianconstraintmap;

    // list of parameters to be diagnosed in this routine (removes value parameters from driven
    // constraints)
    GCS::VEC_pD pdiagnoselist;

    // tag multiplicity gives the number of solver constraints associated with the same tag
    // A tag generally corresponds to the Sketcher constraint index - There are special tag values,
    // like 0 and -1.
    std::map<int, int> tagmultiplicity;

    makeReducedJacobian(J, jacobianconstraintmap, pdiagnoselist, tagmultiplicity);

    // this function will exit with a diagnosis and, unless overridden by functions below, with full
    // DoFs
    hasDiagnosis = true;
    dofs = pdiagnoselist.size();

    // Use DenseQR for small to medium systems to avoid SparseQR rank issues.
    // SparseQR is known to fail rank detection on specific geometric structures (e.g. aligned slots).
    // 200 parameters roughly corresponds to ~100 points/curves, covering most complex sketches
    // where stability is preferred over pure O(N) performance.
    // See: https://github.com/FreeCAD/FreeCAD/issues/10903
    if (autoChooseAlgorithm) {
        qrAlgorithm = dofs < autoQRThreshold ? EigenDenseQR : EigenSparseQR;
    }

    // There is a legacy decision to use QR decomposition. I (abdullah) do not know all the
    // consideration taken in that decisions. I see that:
    // - QR decomposition is able to provide information about the rank and
    // redundant/conflicting
    //   constraints
    // - The QR decomposition of J and the QR decomposition of the transpose of J are unrelated
    //   (for reasons see below):
    //   https://mathoverflow.net/questions/338729/translate-between-qr-decomposition-of-a-and-a-transpose
    // - QR is cheaper than a SVD decomposition
    // - QR is more expensive than a rank revealing LU factorization
    // - QR is less stable than SVD with respect to rank
    // - It is unclear whether it is possible to obtain information about redundancy with SVD
    // and LU

    // Given this legacy decision, the following is observed:
    // - A = QR decomposition can be used for the diagonise of dependency of the "columns" of A.
    // the reason is that matrix R is upper triangular with columns of A showing the
    // dependencies.
    // - The same does not apply to the "rows".
    // - For this reason, to enable a full diagnose of constraints, a QR decomposition must be
    // done
    //   on the transpose of the Jacobian matrix (J), this is JT.

    // Eigen capabilities:
    // - If Eigen full pivoting QR decomposition is used, it is possible to track the rows of JT
    //   during the decomposition. This can be leveraged to identify a set of independent rows
    //   of JT (geometry) that form a rank N basis. However, because the R matrix is of the JT
    //   decomposition and not the J decomposition, it is not possible to reduce the system to
    //   identify exactly which rows are dependent.
    // - The effect is that it provides a set of parameters of geometry that are not constraint,
    //   but it does not identify ALL geometries that are not fixed.
    // - If SpareQR is used, then it is not possible to track the rows of JT during
    // decomposition.
    //   I do not know if it is still possible to obtain geometry information at all from
    //   SparseQR. After several years these questions remain open:
    //      https://stackoverflow.com/questions/49009771/getting-rows-transpositions-with-sparse-qr
    //      https://forum.kde.org/viewtopic.php?f=74&t=151239
    //
    // Implementation below:
    //
    // Two QR decompositions are used below. One for diagnosis of constraints and a second one
    // for diagnosis of parameters, i.e. to identify whether the parameter is fully constraint
    // (independent) or not (i.e. it is dependent).

    // QR decomposition method selection: SparseQR vs DenseQR

#ifndef EIGEN_SPARSEQR_COMPATIBLE
    if (qrAlgorithm == EigenSparseQR) {
        Base::Console().warning(
            "SparseQR not supported by you current version of Eigen. It "
            "requires Eigen 3.2.2 or higher. Falling back to Dense QR\n"
        );
        qrAlgorithm = EigenDenseQR;
    }
#endif

    if (J.rows() == 0) {
        return dofs;
    }

    // From here on, presuming `J.rows() > 0`.
    emptyDiagnoseMatrix = false;

    if (qrAlgorithm == EigenDenseQR) {
#ifdef PROFILE_DIAGNOSE
        Base::TimeElapsed DenseQR_start_time;
#endif

        int rank = 0;  // rank is not cheap to retrieve from qrJT in DenseQR
        Eigen::MatrixXd R;
        Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qrJT;
        // Here we give the system the possibility to run the two QR decompositions in parallel,
        // depending on the load of the system so we are using the default std::launch::async |
        // std::launch::deferred policy, as nobody better than the system nows if it can run the
        // task in parallel or is oversubscribed and should deferred it. Care to wait() for the
        // future before any prospective detection of conflicting/redundant, because the
        // redundant solve modifies pdiagnoselist and it would NOT be thread-safe. Care to call
        // the thread with silent=true, unless the present thread does not use Base::Console, or
        // the launch policy is set to std::launch::deferred policy, as it is not thread-safe to
        // use them in both at the same time.
        //
        // identifyDependentParametersDenseQR(J, jacobianconstraintmap, pdiagnoselist, true)
        //
        auto fut = std::async(
            &System::identifyDependentParametersDenseQR,
            this,
            J,
            jacobianconstraintmap,
            pdiagnoselist,
            true
        );

        makeDenseQRDecomposition(J, jacobianconstraintmap, qrJT, rank, R);

        int paramsNum = qrJT.rows();
        int constrNum = qrJT.cols();

        // This function is legacy code that was used to obtain partial geometry dependency
        // information from a SINGLE Dense QR decomposition. I am reluctant to remove it from
        // here until everything new is well tested.
        // identifyDependentGeometryParametersInTransposedJacobianDenseQRDecomposition( qrJT,
        // pdiagnoselist, paramsNum, rank);

        fut.wait();  // wait for the execution of identifyDependentParametersSparseQR to finish

        dofs = paramsNum - rank;  // unless overconstraint, which will be overridden below

        // Detecting conflicting or redundant constraints
        if (constrNum > rank) {
            // conflicting or redundant constraints
            int nonredundantconstrNum;
            identifyConflictingRedundantConstraints(
                alg,
                qrJT,
                jacobianconstraintmap,
                tagmultiplicity,
                pdiagnoselist,
                R,
                constrNum,
                rank,
                nonredundantconstrNum
            );
            if (paramsNum == rank && nonredundantconstrNum > rank) {  // over-constrained
                dofs = paramsNum - nonredundantconstrNum;
            }
        }

#ifdef PROFILE_DIAGNOSE
        Base::TimeElapsed DenseQR_end_time;

        auto SolveTime = Base::TimeElapsed::diffTimeF(DenseQR_start_time, DenseQR_end_time);

        Base::Console().log("\nDenseQR - Lapsed Time: %f seconds\n", SolveTime);
#endif
    }

#ifdef EIGEN_SPARSEQR_COMPATIBLE
    else if (qrAlgorithm == EigenSparseQR) {
# ifdef PROFILE_DIAGNOSE
        Base::TimeElapsed SparseQR_start_time;
# endif
        int rank = 0;
        Eigen::MatrixXd R;
        Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> SqrJT;
        // Here we give the system the possibility to run the two QR decompositions in parallel,
        // depending on the load of the system so we are using the default std::launch::async |
        // std::launch::deferred policy, as nobody better than the system nows if it can run the
        // task in parallel or is oversubscribed and should deferred it. Care to wait() for the
        // future before any prospective detection of conflicting/redundant, because the
        // redundant solve modifies pdiagnoselist and it would NOT be thread-safe. Care to call
        // the thread with silent=true, unless the present thread does not use Base::Console, or
        // the launch policy is set to std::launch::deferred policy, as it is not thread-safe to
        // use them in both at the same time.
        //
        // identifyDependentParametersSparseQR(J, jacobianconstraintmap, pdiagnoselist, true)
        //
        // Debug:
        // auto fut =
        // std::async(std::launch::deferred,&System::identifyDependentParametersSparseQR, this,
        // J, jacobianconstraintmap, pdiagnoselist, false);
        auto fut = std::async(
            &System::identifyDependentParametersSparseQR,
            this,
            J,
            jacobianconstraintmap,
            pdiagnoselist,
            /*silent=*/true
        );

        makeSparseQRDecomposition(
            J,
            jacobianconstraintmap,
            SqrJT,
            rank,
            R,
            /*transposed=*/true,
            /*silent=*/false
        );

        int paramsNum = SqrJT.rows();
        int constrNum = SqrJT.cols();

        fut.wait();  // wait for the execution of identifyDependentParametersSparseQR to finish

        dofs = paramsNum - rank;  // unless overconstraint, which will be overridden below

        // Detecting conflicting or redundant constraints
        if (constrNum > rank) {
            int nonredundantconstrNum;

            identifyConflictingRedundantConstraints(
                alg,
                SqrJT,
                jacobianconstraintmap,
                tagmultiplicity,
                pdiagnoselist,
                R,
                constrNum,
                rank,
                nonredundantconstrNum
            );

            if (paramsNum == rank && nonredundantconstrNum > rank) {
                // over-constrained
                dofs = paramsNum - nonredundantconstrNum;
            }
        }

# ifdef PROFILE_DIAGNOSE
        Base::TimeElapsed SparseQR_end_time;

        auto SolveTime = Base::TimeElapsed::diffTimeF(SparseQR_start_time, SparseQR_end_time);

        Base::Console().log("\nSparseQR - Lapsed Time: %f seconds\n", SolveTime);
# endif
    }
#endif

    return dofs;
}

void System::makeDenseQRDecomposition(
    const Eigen::MatrixXd& J,
    const std::map<int, int>& jacobianconstraintmap,
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd>& qrJT,
    int& rank,
    Eigen::MatrixXd& R,
    bool transposeJ,
    bool silent
)
{

#ifdef _GCS_DEBUG
    if (!silent) {
        SolverReportingManager::Manager().LogMatrix("J", J);
    }
#endif

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    Eigen::MatrixXd Q;   // Obtaining the Q matrix with Sparse QR is buggy, see comments below
    Eigen::MatrixXd R2;  // Intended for a trapezoidal matrix, where R is the top triangular matrix
                         // of the R2 trapezoidal matrix
#endif

    // For a transposed J SJG rows are paramsNum and cols are constrNum
    // For a non-transposed J SJG rows are constrNum and cols are paramsNum
    int rowsNum = 0;
    int colsNum = 0;

    if (J.rows() > 0) {
        Eigen::MatrixXd JG;
        if (transposeJ) {
            JG = J.topRows(jacobianconstraintmap.size()).transpose();
        }
        else {
            JG = J.topRows(jacobianconstraintmap.size());
        }

        if (JG.rows() > 0 && JG.cols() > 0) {

            qrJT.compute(JG);

            rowsNum = qrJT.rows();
            colsNum = qrJT.cols();
            qrJT.setThreshold(qrpivotThreshold);
            rank = qrJT.rank();

            if (colsNum >= rowsNum) {
                R = qrJT.matrixQR().triangularView<Eigen::Upper>();
            }
            else {
                R = qrJT.matrixQR().topRows(colsNum).triangularView<Eigen::Upper>();
            }
        }
        else {
            rowsNum = JG.rows();
            colsNum = JG.cols();
        }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        R2 = qrJT.matrixQR();
        Q = qrJT.matrixQ();
#endif
    }

    if (debugMode == IterationLevel && !silent) {
        SolverReportingManager::Manager().LogQRSystemInformation(*this, rowsNum, colsNum, rank);
    }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    if (J.rows() > 0 && !silent) {
        SolverReportingManager::Manager().LogMatrix("R", R);

        SolverReportingManager::Manager().LogMatrix("R2", R2);

        SolverReportingManager::Manager().LogMatrix("Q", Q);
        SolverReportingManager::Manager().LogMatrix("RowTransp", qrJT.rowsTranspositions());
    }
#endif
}

#ifdef EIGEN_SPARSEQR_COMPATIBLE
void System::makeSparseQRDecomposition(
    const Eigen::MatrixXd& J,
    const std::map<int, int>& jacobianconstraintmap,
    Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>>& SqrJT,
    int& rank,
    Eigen::MatrixXd& R,
    bool transposeJ,
    bool silent
)
{

    Eigen::SparseMatrix<double> SJ;

    // this creation is not optimized (done using triplets)
    // however the time this takes is negligible compared to the
    // time the QR decomposition itself takes
    SJ = J.sparseView();
    SJ.makeCompressed();

# ifdef _GCS_DEBUG
    if (!silent) {
        SolverReportingManager::Manager().LogMatrix("J", J);
    }
# endif

# ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    Eigen::MatrixXd Q;   // Obtaining the Q matrix with Sparse QR is buggy, see comments below
    Eigen::MatrixXd R2;  // Intended for a trapezoidal matrix, where R is the top triangular matrix
                         // of the R2 trapezoidal matrix
# endif

    // For a transposed J SJG rows are paramsNum and cols are constrNum
    // For a non-transposed J SJG rows are constrNum and cols are paramsNum
    int rowsNum = 0;
    int colsNum = 0;

    if (SJ.rows() > 0) {
        Eigen::SparseMatrix<double> SJG;
        if (transposeJ) {
            SJG = SJ.topRows(jacobianconstraintmap.size()).transpose();
        }
        else {
            SJG = SJ.topRows(jacobianconstraintmap.size());
        }

        if (SJG.rows() > 0 && SJG.cols() > 0) {
            SqrJT.compute(SJG);
// Do not ask for Q Matrix!!
// At Eigen 3.2 still has a bug that this only works for square matrices
// if enabled it will crash
# ifdef SPARSE_Q_MATRIX
            Q = SqrJT.matrixQ();
// Q = QS;
# endif

            rowsNum = SqrJT.rows();
            colsNum = SqrJT.cols();
            SqrJT.setPivotThreshold(qrpivotThreshold);
            rank = SqrJT.rank();

            if (colsNum >= rowsNum) {
                R = SqrJT.matrixR().triangularView<Eigen::Upper>();
            }
            else {
                R = SqrJT.matrixR().topRows(colsNum).triangularView<Eigen::Upper>();
            }

# ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
            R2 = SqrJT.matrixR();
# endif
        }
        else {
            rowsNum = SJG.rows();
            colsNum = SJG.cols();
        }
    }

    if (debugMode == IterationLevel && !silent) {
        SolverReportingManager::Manager().LogQRSystemInformation(*this, rowsNum, colsNum, rank);
    }

# ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    if (J.rows() > 0 && !silent) {

        SolverReportingManager::Manager().LogMatrix("R", R);

        SolverReportingManager::Manager().LogMatrix("R2", R2);

#  ifdef SPARSE_Q_MATRIX
        SolverReportingManager::Manager().LogMatrix("Q", Q);
#  endif
    }
# endif  //_GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
}
#endif  // EIGEN_SPARSEQR_COMPATIBLE

void System::identifyDependentParametersDenseQR(
    const Eigen::MatrixXd& J,
    const std::map<int, int>& jacobianconstraintmap,
    const GCS::VEC_pD& pdiagnoselist,
    bool silent
)
{
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qrJ;
    Eigen::MatrixXd Rparams;

    int rank;

    makeDenseQRDecomposition(J, jacobianconstraintmap, qrJ, rank, Rparams, false, true);

    identifyDependentParameters(qrJ, Rparams, rank, pdiagnoselist, silent);
}

#ifdef EIGEN_SPARSEQR_COMPATIBLE
void System::identifyDependentParametersSparseQR(
    const Eigen::MatrixXd& J,
    const std::map<int, int>& jacobianconstraintmap,
    const GCS::VEC_pD& pdiagnoselist,
    bool silent
)
{
    Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> SqrJ;
    Eigen::MatrixXd Rparams;

    int nontransprank;

    makeSparseQRDecomposition(
        J,
        jacobianconstraintmap,
        SqrJ,
        nontransprank,
        Rparams,
        false,
        true
    );  // do not transpose allow one to diagnose parameters

    identifyDependentParameters(SqrJ, Rparams, nontransprank, pdiagnoselist, silent);
}
#endif

template<typename T>
void System::identifyDependentParameters(
    T& qrJ,
    Eigen::MatrixXd& Rparams,
    int rank,
    const GCS::VEC_pD& pdiagnoselist,
    bool silent
)
{
    (void)silent;  // silent is only used in debug code, but it is important as Base::Console is not
                   // thread-safe. Removes warning in non Debug mode.

    // int constrNum = SqrJ.rows(); // this is the other way around than for the transposed J
    // int paramsNum = SqrJ.cols();

    eliminateNonZerosOverPivotInUpperTriangularMatrix(Rparams, rank);

#ifdef _GCS_DEBUG
    if (!silent) {
        SolverReportingManager::Manager().LogMatrix("Rparams_nonzeros_over_pilot", Rparams);
    }
#endif

    pDependentParametersGroups.resize(qrJ.cols() - rank);
    for (int j = rank; j < qrJ.cols(); j++) {
        for (int row = 0; row < rank; row++) {
            if (fabs(Rparams(row, j)) > 1e-10) {
                int origCol = qrJ.colsPermutation().indices()[row];

                pDependentParametersGroups[j - rank].push_back(pdiagnoselist[origCol]);
                pDependentParameters.push_back(pdiagnoselist[origCol]);
            }
        }
        int origCol = qrJ.colsPermutation().indices()[j];

        pDependentParametersGroups[j - rank].push_back(pdiagnoselist[origCol]);
        pDependentParameters.push_back(pdiagnoselist[origCol]);
    }

#ifdef _GCS_DEBUG
    if (!silent) {
        SolverReportingManager::Manager().LogMatrix(
            "PermMatrix",
            (Eigen::MatrixXd)qrJ.colsPermutation()
        );

        SolverReportingManager::Manager().LogGroupOfParameters(
            "ParameterGroups",
            pDependentParametersGroups
        );
    }

#endif
}

void System::identifyDependentGeometryParametersInTransposedJacobianDenseQRDecomposition(
    const Eigen::FullPivHouseholderQR<Eigen::MatrixXd>& qrJT,
    const GCS::VEC_pD& pdiagnoselist,
    int paramsNum,
    int rank
)
{
    // DETECTING CONSTRAINT SOLVER PARAMETERS
    //
    // NOTE: This is only true for dense QR with full pivoting, because solve parameters get
    // reordered. I am unable to adapt it to Sparse QR. (abdullah). See:
    //
    // https://stackoverflow.com/questions/49009771/getting-rows-transpositions-with-sparse-qr
    // https://forum.kde.org/viewtopic.php?f=74&t=151239
    //
    // R (original version, not R here which is trimmed to not have empty rows)
    // has paramsNum rows, the first "rank" rows correspond to parameters that are constraint

    // Calculate the Permutation matrix from the Transposition matrix
    Eigen::PermutationMatrix<Eigen::Dynamic, Eigen::Dynamic> rowPermutations;

    rowPermutations.setIdentity(paramsNum);

    // P.J.P' = Q.R see https://eigen.tuxfamily.org/dox/classEigen_1_1FullPivHouseholderQR.html
    const MatrixIndexType rowTranspositions = qrJT.rowsTranspositions();

    for (int k = 0; k < rank; ++k) {
        rowPermutations.applyTranspositionOnTheRight(k, rowTranspositions.coeff(k));
    }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    std::stringstream stream;
#endif

    // params (in the order of J) shown as independent from QR
    std::set<int> indepParamCols;
    std::set<int> depParamCols;

    for (int j = 0; j < rank; j++) {

        int origRow = rowPermutations.indices()[j];

        indepParamCols.insert(origRow);

        // NOTE: Q*R = transpose(J), so the row of R corresponds to the col of J (the rows of
        // transpose(J)). The cols of J are the parameters, the rows are the constraints.
#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
        stream << "R row " << j << " = J col " << origRow << std::endl;
#endif
    }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    std::string tmp = stream.str();

    SolverReportingManager::Manager().LogString(tmp);
#endif

    // If not independent, must be dependent
    for (int j = 0; j < paramsNum; j++) {
        auto result = indepParamCols.find(j);
        if (result == indepParamCols.end()) {
            depParamCols.insert(j);
        }
    }

#ifdef _GCS_DEBUG_SOLVER_JACOBIAN_QR_DECOMPOSITION_TRIANGULAR_MATRIX
    stream.flush();

    stream << "Indep params: [";
    for (auto indep : indepParamCols) {
        stream << indep;
    }
    stream << "]" << std::endl;

    stream << "Dep params: [";
    for (auto dep : depParamCols) {
        stream << dep;
    }
    stream << "]" << std::endl;

    tmp = stream.str();
    SolverReportingManager::Manager().LogString(tmp);
#endif


    for (auto param : depParamCols) {
        pDependentParameters.push_back(pdiagnoselist[param]);
    }
}

void System::eliminateNonZerosOverPivotInUpperTriangularMatrix(Eigen::MatrixXd& R, int rank)
{
    for (int i = 1; i < rank; i++) {
        // eliminate non zeros above pivot
        assert(R(i, i) != 0);
        for (int row = 0; row < i; row++) {
            if (R(row, i) != 0) {
                double coef = R(row, i) / R(i, i);
                R.block(row, i + 1, 1, R.cols() - i - 1) -= coef
                    * R.block(i, i + 1, 1, R.cols() - i - 1);
                R(row, i) = 0;
            }
        }
    }
}

template<typename T>
void System::identifyConflictingRedundantConstraints(
    Algorithm alg,
    const T& qrJT,
    const std::map<int, int>& jacobianconstraintmap,
    const std::map<int, int>& tagmultiplicity,
    GCS::VEC_pD& pdiagnoselist,
    Eigen::MatrixXd& R,
    int constrNum,
    int rank,
    int& nonredundantconstrNum
)
{
    eliminateNonZerosOverPivotInUpperTriangularMatrix(R, rank);

    std::vector<std::vector<Constraint*>> conflictGroups(constrNum - rank);
    for (int j = rank; j < constrNum; j++) {
        for (int row = 0; row < rank; row++) {
            if (fabs(R(row, j)) > 1e-10) {
                int origCol = qrJT.colsPermutation().indices()[row];

                conflictGroups[j - rank].push_back(clist[jacobianconstraintmap.at(origCol)]);
            }
        }
        int origCol = qrJT.colsPermutation().indices()[j];

        conflictGroups[j - rank].push_back(clist[jacobianconstraintmap.at(origCol)]);
    }

    // Augment the information regarding the group of constraints that are conflicting or redundant.
    if (debugMode == IterationLevel) {
        SolverReportingManager::Manager().LogGroupOfConstraints(
            "Analysing groups of constraints of special interest",
            conflictGroups
        );
    }

    // try to remove the conflicting constraints and solve the
    // system in order to check if the removed constraints were
    // just redundant but not really conflicting
    std::set<Constraint*> skipped;
    SET_I satisfiedGroups;
    while (1) {
        // conflictingMap contains all the eligible constraints of conflict groups not yet
        // satisfied. As groups get satisfied, the map created on every iteration is smaller, until
        // such time it is empty and the infinite loop is exited. The guarantee that the loop will
        // be exited originates from the fact that in each iteration the algorithm will select one
        // constraint from the conflict groups, which will satisfy at least one group.
        std::map<Constraint*, SET_I> conflictingMap;
        for (std::size_t i = 0; i < conflictGroups.size(); i++) {
            if (satisfiedGroups.count(i) != 0) {
                continue;
            }

            for (const auto& constr : conflictGroups[i]) {
                bool isinternalalignment
                    = (constr->isInternalAlignment() == Constraint::Alignment::InternalAlignment);
                bool priorityconstraint = (constr->getTag() == 0);
                if (!priorityconstraint && !isinternalalignment) {
                    // exclude constraints tagged with zero and internal alignment
                    conflictingMap[constr].insert(i);
                }
            }
        }

        if (conflictingMap.empty()) {
            break;
        }

        /* This is a heuristic algorithm to propose the user which constraints from a
         * redundant/conflicting set should be removed. It is based on the following principles:
         * 1. if the current constraint is more popular than previous ones (appears in more
         * sets), take it. This prioritises removal of constraints that cause several
         * independent groups of constraints to be conflicting/redundant. It is based on the
         * observation that the redundancy/conflict is caused by the lesser amount of
         * constraints.
         * 2. if there is already a constraint ranking in the contest, and the current one is as
         * popular, prefer the constraint that removes a lesser amount of DoFs. This prioritises
         * removal of sketcher constraints (not solver constraints) that generates a higher
         * amount of solver constraints. It is based on the observation that constraints taking
         * a higher amount of DoFs (such as symmetry) are preferred by the user, who may not see
         * the redundancy of simpler ones.
         * 3. if there is already a constraint ranking in the context, the current one is as
         * popular, and they remove the same amount of DoFs, prefer removal of the latest
         * introduced.
         */
        auto iterMostPopular = std::max_element(
            conflictingMap.begin(),
            conflictingMap.end(),
            [&tagmultiplicity](const auto& pair1, const auto& pair2) {
                size_t sizeOfSet1 = pair1.second.size();
                size_t sizeOfSet2 = pair2.second.size();
                auto tag1 = pair1.first->getTag();
                auto tag2 = pair2.first->getTag();

                return (
                    sizeOfSet2 > sizeOfSet1  // (1)
                    || (sizeOfSet2 == sizeOfSet1
                        && tagmultiplicity.at(tag2) < tagmultiplicity.at(tag1))  // (2)
                    || (sizeOfSet2 == sizeOfSet1
                        && tagmultiplicity.at(tag2) == tagmultiplicity.at(tag1) && tag2 > tag1)
                );  // (3)
            }
        );

        Constraint* mostPopular = iterMostPopular->first;
        int maxPopularity = iterMostPopular->second.size();

        if (!(maxPopularity > 0)) {
            continue;
        }

        // adding for skipping not only the mostPopular, but also any other constraint in the
        // conflicting map associated with the same tag (namely any other solver
        // constraint associated with the same sketcher constraint that is also conflicting)
        auto maxPopularityTag = mostPopular->getTag();

        for (const auto& [constr, conflSet] : conflictingMap) {
            if (!(constr->getTag() == maxPopularityTag)) {
                continue;
            }

            skipped.insert(constr);
            std::ranges::copy(conflSet, std::inserter(satisfiedGroups, satisfiedGroups.begin()));
        }
    }

    // Augment information regarding the choice made by popularity contest
    if (debugMode == IterationLevel) {
        SolverReportingManager::Manager().LogSetOfConstraints("Chosen redundants", skipped);
    }

    std::vector<Constraint*> clistTmp;
    clistTmp.reserve(clist.size());
    std::ranges::copy_if(clist, std::back_inserter(clistTmp), [&skipped](const auto& constr) {
        return (constr->isDriving() && skipped.count(constr) == 0);
    });

    SubSystem* subSysTmp = new SubSystem(clistTmp, pdiagnoselist);
    int res = solve(subSysTmp, true, alg, true);

    if (debugMode == Minimal || debugMode == IterationLevel) {
        std::string solvername;
        switch (alg) {
            case 0:
                solvername = "BFGS";
                break;
            case 1:  // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                break;
            case 2:  // solving with the BFGS solver
                solvername = "DogLeg";
                break;
        }

        Base::Console().log("Sketcher::RedundantSolving-%s-\n", solvername.c_str());
    }

    if (res == Success) {
        subSysTmp->applySolution();
        std::ranges::copy_if(
            skipped,
            std::inserter(redundant, redundant.begin()),
            [this](const auto& constr) {
                double err = constr->error();
                return (err * err < this->convergenceRedundant);
            }
        );
        resetToReference();

        if (debugMode == Minimal || debugMode == IterationLevel) {
            Base::Console().log("Sketcher Redundant solving: %d redundants\n", redundant.size());
        }

        // TODO: Figure out why we need to iterate in reverse order and add explanation here.
        std::vector<std::vector<Constraint*>> conflictGroupsOrig = conflictGroups;
        conflictGroups.clear();
        for (int i = conflictGroupsOrig.size() - 1; i >= 0; i--) {
            auto iterRedundantEntry = std::ranges::find_if(
                conflictGroupsOrig[i],
                [this](const auto item) { return (this->redundant.count(item) > 0); }
            );
            bool hasRedundant = (iterRedundantEntry != conflictGroupsOrig[i].end());
            if (!hasRedundant) {
                conflictGroups.push_back(conflictGroupsOrig[i]);
                continue;
            }

            if (debugMode == IterationLevel) {
                Base::Console().log(
                    "(Partially) Redundant, Group %d, index %d, Tag: %d\n",
                    i,
                    iterRedundantEntry - conflictGroupsOrig[i].begin(),
                    (*iterRedundantEntry)->getTag()
                );
            }

            constrNum--;
        }
    }
    delete subSysTmp;

    // simplified output of conflicting tags
    SET_I conflictingTagsSet;
    for (const auto& cGroup : conflictGroups) {
        // exclude internal alignment
        std::ranges::transform(
            cGroup,
            std::inserter(conflictingTagsSet, conflictingTagsSet.begin()),
            [](const auto& constr) {
                bool isinternalalignment
                    = (constr->isInternalAlignment() == Constraint::Alignment::InternalAlignment);
                return (isinternalalignment ? 0 : constr->getTag());
            }
        );
    }

    // exclude constraints tagged with zero
    conflictingTagsSet.erase(0);

    conflictingTags.resize(conflictingTagsSet.size());
    std::ranges::copy(conflictingTagsSet, conflictingTags.begin());

    // output of redundant tags
    SET_I redundantTagsSet, partiallyRedundantTagsSet;
    for (const auto& constr : redundant) {
        redundantTagsSet.insert(constr->getTag());
        partiallyRedundantTagsSet.insert(constr->getTag());
    }

    // remove tags represented at least in one non-redundant constraint
    for (const auto& constr : clist) {
        if (redundant.count(constr) == 0) {
            redundantTagsSet.erase(constr->getTag());
        }
    }

    redundantTags.resize(redundantTagsSet.size());
    std::ranges::copy(redundantTagsSet, redundantTags.begin());

    for (auto r : redundantTagsSet) {
        partiallyRedundantTagsSet.erase(r);
    }

    partiallyRedundantTags.resize(partiallyRedundantTagsSet.size());
    std::ranges::copy(partiallyRedundantTagsSet, partiallyRedundantTags.begin());

    nonredundantconstrNum = constrNum;
}

void System::clearSubSystems()
{
    isInit = false;
    deleteAllContent(subSystems);
    deleteAllContent(subSystemsAux);
    subSystems.clear();
    subSystemsAux.clear();
}

double lineSearch(SubSystem* subsys, Eigen::VectorXd& xdir)
{
    double f1, f2, f3, alpha1, alpha2, alpha3, alphaStar;

    double alphaMax = subsys->maxStep(xdir);

    Eigen::VectorXd x0, x;

    // Save initial values
    subsys->getParams(x0);

    // Start at the initial position alpha1 = 0
    alpha1 = 0.;
    f1 = subsys->error();

    // Take a step of alpha2 = 1
    alpha2 = 1.;
    x = x0 + alpha2 * xdir;
    subsys->setParams(x);
    f2 = subsys->error();

    // Take a step of alpha3 = 2*alpha2
    alpha3 = alpha2 * 2;
    x = x0 + alpha3 * xdir;
    subsys->setParams(x);
    f3 = subsys->error();

    // Now reduce or lengthen alpha2 and alpha3 until the minimum is
    // Bracketed by the triplet f1>f2<f3
    while (f2 > f1 || f2 > f3) {
        if (f2 > f1) {
            // If f2 is greater than f1 then we shorten alpha2 and alpha3 closer to f1
            // Effectively both are shortened by a factor of two.
            alpha3 = alpha2;
            f3 = f2;
            alpha2 = alpha2 / 2;
            x = x0 + alpha2 * xdir;
            subsys->setParams(x);
            f2 = subsys->error();
        }
        else if (f2 > f3) {
            if (alpha3 >= alphaMax) {
                break;
            }
            // If f2 is greater than f3 then we increase alpha2 and alpha3 away from f1
            // Effectively both are lengthened by a factor of two.
            alpha2 = alpha3;
            f2 = f3;
            alpha3 = alpha3 * 2;
            x = x0 + alpha3 * xdir;
            subsys->setParams(x);
            f3 = subsys->error();
        }
    }
    // Get the alpha for the minimum f of the quadratic approximation
    alphaStar = alpha2 + ((alpha2 - alpha1) * (f1 - f3)) / (3 * (f1 - 2 * f2 + f3));

    // Guarantee that the new alphaStar is within the bracket
    if (alphaStar >= alpha3 || alphaStar <= alpha1) {
        alphaStar = alpha2;
    }

    if (alphaStar > alphaMax) {
        alphaStar = alphaMax;
    }

    if (alphaStar != alphaStar) {
        alphaStar = 0.;
    }

    // Take a final step to alphaStar
    x = x0 + alphaStar * xdir;
    subsys->setParams(x);

    return alphaStar;
}

void deleteAllContent(VEC_pD& doublevec)
{
    for (auto& doubleptr : doublevec) {
        delete doubleptr;
    }
    doublevec.clear();
}

void deleteAllContent(std::vector<Constraint*>& constrvec)
{
    for (auto& constr : constrvec) {
        delete constr;
    }
    constrvec.clear();
}

void deleteAllContent(std::vector<SubSystem*>& subsysvec)
{
    for (auto& subsys : subsysvec) {
        delete subsys;
    }
}

}  // namespace GCS
