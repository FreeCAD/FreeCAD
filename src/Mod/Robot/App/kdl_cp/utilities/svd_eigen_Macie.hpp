// Copyright  (C)  2008  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

// Version: 1.0
// Author: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Maintainer: Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// URL: http://www.orocos.org/kdl

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


//implementation of svd according to (Maciejewski and Klein,1989)
//and (Braun, Ulrey, Maciejewski and Siegel,2002)

/**
 * \file svd_eigen_Macie.hpp
 * provides Maciejewski's implementation for SVD.
 */

#ifndef SVD_EIGEN_MACIE
#define SVD_EIGEN_MACIE

#include <Eigen/Core>


namespace KDL
{

	/**
	 * svd_eigen_Macie provides Maciejewski implementation for SVD.
	 *
	 * computes the singular value decomposition of a matrix A, such that
	 * A=U*Sm*V
	 *
	 * (Maciejewski and Klein,1989) and (Braun, Ulrey, Maciejewski and Siegel,2002)
	 *
	 * \param A [INPUT] is an \f$m \times n\f$-matrix, where \f$ m \geq n \f$.
	 * \param S [OUTPUT] is an \f$n\f$-vector, representing the diagonal elements of the diagonal matrix Sm.
	 * \param U [INPUT/OUTPUT] is an \f$m \times m\f$ orthonormal matrix.
	 * \param V [INPUT/OUTPUT] is an \f$n \times n\f$ orthonormal matrix.
	 * \param B [TEMPORARY] is an \f$m \times n\f$ matrix used for temporary storage.
	 * \param tempi [TEMPORARY] is an \f$m\f$ vector used for temporary storage.
	 * \param threshold [INPUT] Threshold to determine orthogonality.
	 * \param toggle [INPUT] toggle this boolean variable on each call of this routine.
	 * \return number of sweeps.
	 */
    int svd_eigen_Macie(const Eigen::MatrixXd& A,Eigen::MatrixXd& U,Eigen::VectorXd& S, Eigen::MatrixXd& V,
                        Eigen::MatrixXd& B, Eigen::VectorXd& tempi,
                        double threshold,bool toggle);


}
#endif
