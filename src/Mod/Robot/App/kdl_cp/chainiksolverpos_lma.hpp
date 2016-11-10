#ifndef KDL_CHAINIKSOLVERPOS_GN_HPP
#define KDL_CHAINIKSOLVERPOS_GN_HPP
/**
 \file   chainiksolverpos_lma.hpp
 \brief  computing inverse position kinematics using Levenberg-Marquardt.
*/

/**************************************************************************
    begin                : May 2012
    copyright            : (C) 2012 Erwin Aertbelien
    email                : firstname.lastname@mech.kuleuven.ac.be

 History (only major changes)( AUTHOR-Description ) :

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


#include "chainiksolver.hpp"
#include "chain.hpp"
#include <Eigen/Dense>

namespace KDL
{

/**
 * \brief Solver for the inverse position kinematics that uses Levenberg-Marquardt.
 *
 * The robustness and speed of this solver is improved in several ways:
 *   - by using a Levenberg-Marquardt method that automatically adapts the damping when
 *     computing the inverse damped least squares inverse velocity kinematics.
 *   - by using an internal implementation of forward position kinematics and the
 *     Jacobian kinematics.  This implementation is more numerically robust,
 *     is able to cache previous computations, and implements an \f$ \mathcal{O}(N) \f$
 *     algorithm for the computation of the Jacobian (with \f$N\f$, the number of joints, and for
 *     a fixed size task space).
 *   - by providing a way to specify the weights in task space, you can weigh rotations wrt translations.
 *     This is important e.g. to specify that rotations do not matter for the problem at hand, or to
 *     specify how important you judge rotations w.r.t. translations, typically in S.I.-units, ([m],[rad]),
 *     the rotations are over-specified, this can be avoided using the weight matrix. <B>Weights also
 *     make the solver more robust </B>.
 *   - only the constructors call <B>memory allocation</B>.
 *
 * De general principles behind the optimisation is inspired on:
 *   Jorge Nocedal, Stephen J. Wright, Numerical Optimization,Springer-Verlag New York, 1999.

 * \ingroup KinematicFamily
 */
class ChainIkSolverPos_LMA : public KDL::ChainIkSolverPos
{
private:
	typedef double ScalarType;
    typedef Eigen::Matrix<ScalarType,Eigen::Dynamic,Eigen::Dynamic> MatrixXq;
    typedef Eigen::Matrix<ScalarType,Eigen::Dynamic,1> VectorXq;
public:

    /**
	 * \brief constructs an ChainIkSolverPos_LMA solver.
	 *
	 * The default parameters are choosen to be applicable to industrial-size robots
	 * (e.g. 0.5 to 3 meters range in task space), with an accuracy that is more then
	 * sufficient for typical industrial applications.
	 *
	 * Weights are applied in task space, i.e. the kinematic solver minimizes:
	 * \f$ E = \Delta \mathbf{x}^T \mathbf{L} \mathbf{L}^T \Delta \mathbf{x} \f$, with \f$\mathbf{L}\f$ a diagonal matrix.
	 *
	 * \param _chain specifies the kinematic chain.
	 * \param _L specifies the "square root" of the weight (diagonal) matrix in task space. This diagonal matrix is specified as a vector.
	 * \param _eps specifies the desired accuracy in task space; <B>after</B> weighing with
	 *        the weight matrix, it is applied on \f$E\f$.
	 * \param _maxiter specifies the maximum number of iterations.
	 * \param _eps_joints specifies that the algorithm has to stop when the computed joint angle increments are
	 *        smaller then _eps_joints.  This is to avoid unnecessary computations up to _maxiter when the joint angle
	 *        increments are so small that they effectively (in floating point) do not change the joint angles any more.  The default
	 *        is a few digits above numerical accuracy.
     */
    ChainIkSolverPos_LMA(
    		const KDL::Chain& _chain,
    		const Eigen::Matrix<double,6,1>& _L,
    		double _eps=1E-5,
    		int _maxiter=500,
    		double _eps_joints=1E-15
    );

    /**
     * \brief identical the full constructor for ChainIkSolverPos_LMA, but provides for a default weight matrix.
     *
     *  \f$\mathbf{L} = \mathrm{diag}\left( \begin{bmatrix} 1 & 1 & 1 & 0.01 & 0.01 & 0.01 \end{bmatrix} \right) \f$.
     */
    ChainIkSolverPos_LMA(
    		const KDL::Chain& _chain,
    		double _eps=1E-5,
    		int _maxiter=500,
    		double _eps_joints=1E-15
    );

    /**
     * \brief computes the inverse position kinematics.
     *
     * \param q_init initial joint position.
     * \param T_base_goal goal position expressed with respect to the robot base.
     * \param q_out  joint position that achieves the specified goal position (if successful).
     * \return 0 if successful,
     *        -1 the gradient of \f$ E \f$ towards the joints is to small,
     *        -2 if joint position increments are to small,
     *        -3 if number of iterations is exceeded.
     */
    virtual int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& T_base_goal, KDL::JntArray& q_out);

    /**
     * \brief destructor.
     */
    virtual ~ChainIkSolverPos_LMA();

    /**
     * \brief for internal use only.
     *
     * Only exposed for test and diagnostic purposes.
     */
    void compute_fwdpos(const VectorXq& q);

    /**
     * \brief for internal use only.
     * Only exposed for test and diagnostic purposes.
     * compute_fwdpos(q) should always have been called before.
     */
    void compute_jacobian(const VectorXq& q);

    /**
     * \brief for internal use only.
     * Only exposed for test and diagnostic purposes.
     */
    void display_jac(const KDL::JntArray& jval);




public:


    /**
     * \brief contains the last number of  iterations for an execution of CartToJnt.
     */
    int lastNrOfIter;

    /**
     * \brief contains the last value for \f$ E \f$ after an execution of CartToJnt.
     */
    double lastDifference;

    /**
     * \brief contains the last value for the (unweighted) translational difference after an execution of CartToJnt.
     */
    double lastTransDiff;

    /**
     * \brief contains the last value for the (unweighted) rotational difference after an execution of CartToJnt.
     */
    double lastRotDiff;

    /**
     * \brief contains the last values for the singular values of the weighted Jacobian after an execution of CartToJnt.
     */
    VectorXq lastSV;

    /**
     * \brief for internal use only.
     *
     * contains the last value for the Jacobian after an execution of compute_jacobian.
     */
    MatrixXq jac;

    /**
     * \brief for internal use only.
     *
     * contains the gradient of the error criterion after an execution of CartToJnt.
     */
    VectorXq grad;
    /**
     * \brief for internal use only.
     *
     * contains the last value for the position of the tip of the robot (head) with respect to the base, after an execution of compute_jacobian.
     */
    KDL::Frame T_base_head;

    /**
     * \brief display information on each iteration step to the console.
     */
    bool display_information;
private:
    // additional specification of the inverse position kinematics problem:


    unsigned int maxiter;
    double eps;
    double eps_joints;
    Eigen::Matrix<ScalarType,6,1> L;
    const KDL::Chain& chain;


    // state of compute_fwdpos and compute_jacobian:
    std::vector<KDL::Frame> T_base_jointroot;
    std::vector<KDL::Frame> T_base_jointtip;
					// need 2 vectors because of the somewhat strange definition of segment.hpp
					// you could also recompute jointtip out of jointroot,
    				// but then you'll need more expensive cos/sin functions.


    // the following are state of CartToJnt that is pre-allocated:

    VectorXq q;
    MatrixXq A;
    VectorXq tmp;
    Eigen::LDLT<MatrixXq> ldlt;
    Eigen::JacobiSVD<MatrixXq> svd;
    VectorXq diffq;
    VectorXq q_new;
    VectorXq original_Aii;
};





}; // namespace KDL






#endif
