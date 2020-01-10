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

#ifndef SVD_BOOST_MACIE
#define SVD_BOOST_MACIE

#include <Eigen/Core>
using namespace Eigen;


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
    int svd_eigen_Macie(const MatrixXd& A,MatrixXd& U,VectorXd& S, MatrixXd& V,
                        MatrixXd& B, VectorXd& tempi,
                        double threshold,bool toggle)
    {
        bool rotate = true;
        unsigned int sweeps=0;
        unsigned int rotations=0;
        if(toggle){
            //Calculate B from new A and previous V
            B=A.lazyProduct(V);
            while(rotate){
                rotate=false;
                rotations=0;
                //Perform rotations between columns of B
                for(auto i=0;i<B.cols();i++){
                    for(auto j=i+1;j<B.cols();j++){
                        //calculate plane rotation
                        double p = B.col(i).dot(B.col(j));
                        double qi =B.col(i).dot(B.col(i));
                        double qj = B.col(j).dot(B.col(j));
                        double q=qi-qj;
                        double alpha = pow(p,2.0)/(qi*qj);
                        //if columns are orthogonal with precision
                        //threshold, don't perform rotation and continue
                        if(alpha<threshold)
                            continue;
                        rotations++;
                        double c = sqrt(4*pow(p,2.0)+pow(q,2.0));
                        double cos,sin;
                        if(q>=0){
                            cos=sqrt((c+q)/(2*c));
                            sin=p/(c*cos);
                        }else{
                            if(p<0)
                                sin=-sqrt((c-q)/(2*c));
                            else
                                sin=sqrt((c-q)/(2*c));
                            cos=p/(c*sin);
                        }

                        //Apply plane rotation to columns of B
                        tempi = cos*B.col(i) + sin*B.col(j);
                        B.col(j) = - sin*B.col(i) + cos*B.col(j);
                        B.col(i) = tempi;
                        
                        //Apply plane rotation to columns of V
                        tempi.head(V.rows()) = cos*V.col(i) + sin*V.col(j);
                        V.col(j) = - sin*V.col(i) + cos*V.col(j);
                        V.col(i) = tempi.head(V.rows());

                        rotate=true;
                    }
                }
                //Only calculate new U and S if there were any rotations
                if(rotations!=0){
                    for(auto i=0;i<U.rows();i++) {
                        if(i<B.cols()){
                            double si=sqrt(B.col(i).dot(B.col(i)));
                            if(si==0)
                                U.col(i) = B.col(i);
                            else
                                U.col(i) = B.col(i)/si;
                            S(i)=si;
                        }
                        else
                            U.col(i) = 0*tempi;
                    }
                    sweeps++;
                }
            }
            return sweeps;
        }else{
            //Calculate B from new A and previous U'
            B = U.transpose().lazyProduct(A);
            while(rotate){
                rotate=false;
                rotations=0;
                //Perform rotations between rows of B
                for(auto i=0;i<B.cols();i++){
                    for(auto j=i+1;j<B.cols();j++){
                        //calculate plane rotation
                        double p = B.row(i).dot(B.row(j));
                        double qi = B.row(i).dot(B.row(i));
                        double qj = B.row(j).dot(B.row(j));

                        double q=qi-qj;
                        double alpha = pow(p,2.0)/(qi*qj);
                        //if columns are orthogonal with precision
                        //threshold, don't perform rotation and
                        //continue
                        if(alpha<threshold)
                            continue;
                        rotations++;
                        double c = sqrt(4*pow(p,2.0)+pow(q,2.0));
                        double cos,sin;
                        if(q>=0){
                            cos=sqrt((c+q)/(2*c));
                            sin=p/(c*cos);
                        }else{
                            if(p<0)
                                sin=-sqrt((c-q)/(2*c));
                            else
                                sin=sqrt((c-q)/(2*c));
                            cos=p/(c*sin);
                        }

                        //Apply plane rotation to rows of B
                        tempi.head(B.cols()) =  cos*B.row(i) + sin*B.row(j);
                        B.row(j) =  - sin*B.row(i) + cos*B.row(j);
                        B.row(i) =  tempi.head(B.cols());


                        //Apply plane rotation to rows of U
                        tempi.head(U.rows()) = cos*U.col(i) + sin*U.col(j);
                        U.col(j) = - sin*U.col(i) + cos*U.col(j);
                        U.col(i) = tempi.head(U.rows());

                        rotate=true;
                    }
                }

                //Only calculate new U and S if there were any rotations
                if(rotations!=0){
                    for(auto i=0;i<V.rows();i++) {
                        double si=sqrt(B.row(i).dot(B.row(i)));
                        if(si==0)
                            V.col(i) = B.row(i);
                        else
                            V.col(i) = B.row(i)/si;
                        S(i)=si;
                    }
                    sweeps++;
                }
            }
            return sweeps;
        }
    }


}
#endif
