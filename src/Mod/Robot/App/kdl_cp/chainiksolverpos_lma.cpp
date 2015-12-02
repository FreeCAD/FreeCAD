/**
 \file   chainiksolverpos_lma.cpp
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

#include "chainiksolverpos_lma.hpp"
#include <iostream>

namespace KDL {




template <typename Derived>
inline void Twist_to_Eigen(const KDL::Twist& t,Eigen::MatrixBase<Derived>& e) {
	e(0)=t.vel.data[0];
	e(1)=t.vel.data[1];
	e(2)=t.vel.data[2];
	e(3)=t.rot.data[0];
	e(4)=t.rot.data[1];
	e(5)=t.rot.data[2];
}


ChainIkSolverPos_LMA::ChainIkSolverPos_LMA(
		const KDL::Chain& _chain,
		const Eigen::Matrix<double,6,1>& _L,
		double _eps,
		int _maxiter,
		double _eps_joints
) :
	lastNrOfIter(0),
	lastSV(_chain.getNrOfJoints()),
	jac(6, _chain.getNrOfJoints()),
	grad(_chain.getNrOfJoints()),
	display_information(false),
	maxiter(_maxiter),
	eps(_eps),
	eps_joints(_eps_joints),
	L(_L.cast<ScalarType>()),
	chain(_chain),
	T_base_jointroot(_chain.getNrOfJoints()),
	T_base_jointtip(_chain.getNrOfJoints()),
	q(_chain.getNrOfJoints()),
	A(_chain.getNrOfJoints(), _chain.getNrOfJoints()),
	tmp(_chain.getNrOfJoints()),
	ldlt(_chain.getNrOfJoints()),
	svd(6, _chain.getNrOfJoints(),Eigen::ComputeThinU | Eigen::ComputeThinV),
	diffq(_chain.getNrOfJoints()),
	q_new(_chain.getNrOfJoints()),
	original_Aii(_chain.getNrOfJoints())
{}

ChainIkSolverPos_LMA::ChainIkSolverPos_LMA(
		const KDL::Chain& _chain,
		double _eps,
		int _maxiter,
		double _eps_joints
) :
	lastNrOfIter(0),
	lastSV(_chain.getNrOfJoints()>6?6:_chain.getNrOfJoints()),
	jac(6, _chain.getNrOfJoints()),
	grad(_chain.getNrOfJoints()),
	display_information(false),
	maxiter(_maxiter),
	eps(_eps),
	eps_joints(_eps_joints),
	chain(_chain),
	T_base_jointroot(_chain.getNrOfJoints()),
	T_base_jointtip(_chain.getNrOfJoints()),
	q(_chain.getNrOfJoints()),
	A(_chain.getNrOfJoints(), _chain.getNrOfJoints()),
	ldlt(_chain.getNrOfJoints()),
	svd(6, _chain.getNrOfJoints(),Eigen::ComputeThinU | Eigen::ComputeThinV),
	diffq(_chain.getNrOfJoints()),
	q_new(_chain.getNrOfJoints()),
	original_Aii(_chain.getNrOfJoints())
{
	L(0)=1;
	L(1)=1;
	L(2)=1;
	L(3)=0.01;
	L(4)=0.01;
	L(5)=0.01;
}

ChainIkSolverPos_LMA::~ChainIkSolverPos_LMA() {}

void ChainIkSolverPos_LMA::compute_fwdpos(const VectorXq& q) {
	using namespace KDL;
	unsigned int jointndx=0;
	T_base_head = Frame::Identity(); // frame w.r.t. base of head
	for (unsigned int i=0;i<chain.getNrOfSegments();i++) {
		const Segment& segment = chain.getSegment(i);
		if (segment.getJoint().getType()!=Joint::None) {
			T_base_jointroot[jointndx] = T_base_head;
			T_base_head = T_base_head * segment.pose(q(jointndx));
			T_base_jointtip[jointndx] = T_base_head;
			jointndx++;
		} else {
			T_base_head = T_base_head * segment.pose(0.0);
		}
	}
}

void ChainIkSolverPos_LMA::compute_jacobian(const VectorXq& q) {
	using namespace KDL;
	unsigned int jointndx=0;
	for (unsigned int i=0;i<chain.getNrOfSegments();i++) {
		const Segment& segment = chain.getSegment(i);
		if (segment.getJoint().getType()!=Joint::None) {
			// compute twist of the end effector motion caused by joint [jointndx]; expressed in base frame, with vel. ref. point equal to the end effector
			KDL::Twist t = ( T_base_jointroot[jointndx].M * segment.twist(q(jointndx),1.0) ).RefPoint( T_base_head.p - T_base_jointtip[jointndx].p);
			jac(0,jointndx)=t[0];
			jac(1,jointndx)=t[1];
			jac(2,jointndx)=t[2];
			jac(3,jointndx)=t[3];
			jac(4,jointndx)=t[4];
			jac(5,jointndx)=t[5];
			jointndx++;
		}
	}
}

void ChainIkSolverPos_LMA::display_jac(const KDL::JntArray& jval) {
	VectorXq q;
	q = jval.data.cast<ScalarType>();
	compute_fwdpos(q);
	compute_jacobian(q);
	svd.compute(jac);
	std::cout << "Singular values : " << svd.singularValues().transpose()<<"\n";
}


int ChainIkSolverPos_LMA::CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& T_base_goal, KDL::JntArray& q_out) {
	using namespace KDL;
	double v      = 2;
	double tau    = 10;
	double rho;
	double lambda;
	Twist t;
	double delta_pos_norm;
	Eigen::Matrix<ScalarType,6,1> delta_pos;
	Eigen::Matrix<ScalarType,6,1> delta_pos_new;


	q=q_init.data.cast<ScalarType>();
	compute_fwdpos(q);
	Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos );
	delta_pos=L.asDiagonal()*delta_pos;
	delta_pos_norm = delta_pos.norm();
	if (delta_pos_norm<eps) {
		lastNrOfIter    =0 ;
		Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos );
		lastDifference  = delta_pos.norm();
		lastTransDiff   = delta_pos.topRows(3).norm();
		lastRotDiff     = delta_pos.bottomRows(3).norm();
		svd.compute(jac);
		original_Aii    = svd.singularValues();
		lastSV          = svd.singularValues();
		q_out.data      = q.cast<double>();
		return 0;
	}
	compute_jacobian(q);
	jac = L.asDiagonal()*jac;

	lambda = tau;
	double dnorm = 1;
	for (unsigned int i=0;i<maxiter;++i) {

		svd.compute(jac);
		original_Aii = svd.singularValues();
		for (unsigned int j=0;j<original_Aii.rows();++j) {
			original_Aii(j) = original_Aii(j)/( original_Aii(j)*original_Aii(j)+lambda);

		}
		tmp = svd.matrixU().transpose()*delta_pos;
		tmp = original_Aii.cwiseProduct(tmp);
		diffq = svd.matrixV()*tmp;
		grad = jac.transpose()*delta_pos;
		if (display_information) {
			std::cout << "------- iteration " << i << " ----------------\n"
					  << "  q              = " << q.transpose() << "\n"
					  << "  weighted jac   = \n" << jac << "\n"
					  << "  lambda         = " << lambda << "\n"
					  << "  eigenvalues    = " << svd.singularValues().transpose() << "\n"
					  << "  difference     = "   << delta_pos.transpose() << "\n"
					  << "  difference norm= "   << delta_pos_norm << "\n"
					  << "  proj. on grad. = "   << grad << "\n";
			std::cout << std::endl;
		}
		dnorm = diffq.lpNorm<Eigen::Infinity>();
		if (dnorm < eps_joints) {
				lastDifference = delta_pos_norm;
				lastNrOfIter   = i;
				lastSV         = svd.singularValues();
				q_out.data     = q.cast<double>();
				compute_fwdpos(q);
				Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos );
				lastTransDiff  = delta_pos.topRows(3).norm();
				lastRotDiff    = delta_pos.bottomRows(3).norm();
				return -2;
		}


		if (grad.transpose()*grad < eps_joints*eps_joints ) {
			compute_fwdpos(q);
			Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos );
			lastDifference = delta_pos_norm;
			lastTransDiff = delta_pos.topRows(3).norm();
			lastRotDiff   = delta_pos.bottomRows(3).norm();
			lastSV        = svd.singularValues();
			lastNrOfIter  = i;
			q_out.data    = q.cast<double>();
			return -1;
		}

		q_new = q+diffq;
		compute_fwdpos(q_new);
		Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos_new );
		delta_pos_new             = L.asDiagonal()*delta_pos_new;
		double delta_pos_new_norm = delta_pos_new.norm();
		rho                       = delta_pos_norm*delta_pos_norm - delta_pos_new_norm*delta_pos_new_norm;
		rho                      /= diffq.transpose()*(lambda*diffq + grad);
		if (rho > 0) {
			q               = q_new;
			delta_pos       = delta_pos_new;
			delta_pos_norm  = delta_pos_new_norm;
			if (delta_pos_norm<eps) {
				Twist_to_Eigen( diff( T_base_head, T_base_goal), delta_pos );
				lastDifference = delta_pos_norm;
				lastTransDiff  = delta_pos.topRows(3).norm();
				lastRotDiff    = delta_pos.bottomRows(3).norm();
				lastSV         = svd.singularValues();
				lastNrOfIter   = i;
				q_out.data     = q.cast<double>();
				return 0;
			}
			compute_jacobian(q_new);
			jac = L.asDiagonal()*jac;
			double tmp=2*rho-1;
			lambda = lambda*max(1/3.0, 1-tmp*tmp*tmp);
			v = 2;
		} else {
			lambda = lambda*v;
			v      = 2*v;
		}
	}
	lastDifference = delta_pos_norm;
	lastTransDiff  = delta_pos.topRows(3).norm();
	lastRotDiff    = delta_pos.bottomRows(3).norm();
	lastSV         = svd.singularValues();
	lastNrOfIter   = maxiter;
	q_out.data     = q.cast<double>();
	return -3;

}



};//namespace KDL
