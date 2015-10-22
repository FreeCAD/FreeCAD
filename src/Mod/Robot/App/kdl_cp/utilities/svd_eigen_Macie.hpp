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
#ifndef SVD_BOOST_MACIE
#define SVD_BOOST_MACIE

#include <Eigen/Core>
USING_PART_OF_NAMESPACE_EIGEN

namespace KDL
{
    int svd_eigen_Macie(const MatrixXd& A,MatrixXd& U,VectorXd& S, MatrixXd& V,
                        MatrixXd& B, VectorXd& tempi,
                        double treshold,bool toggle)
    {
        bool rotate = true;
        unsigned int sweeps=0;
        unsigned int rotations=0;
        if(toggle){
            //Calculate B from new A and previous V
            B=(A*V).lazy();
            while(rotate){
                rotate=false;
                rotations=0;
                //Perform rotations between columns of B
                for(unsigned int i=0;i<B.cols();i++){
                    for(unsigned int j=i+1;j<B.cols();j++){
                        //calculate plane rotation
                        double p = B.col(i).dot(B.col(j));
                        double qi =B.col(i).dot(B.col(i));
                        double qj = B.col(j).dot(B.col(j));
                        double q=qi-qj;
                        double alpha = pow(p,2.0)/(qi*qj);
                        //if columns are orthogonal with precision
                        //treshold, don't perform rotation and continue
                        if(alpha<treshold)
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
                        tempi.start(V.rows()) = cos*V.col(i) + sin*V.col(j);
                        V.col(j) = - sin*V.col(i) + cos*V.col(j);
                        V.col(i) = tempi.start(V.rows());

                        rotate=true;
                    }
                }
                //Only calculate new U and S if there were any rotations
                if(rotations!=0){
                    for(unsigned int i=0;i<U.rows();i++) {
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
            B =(U.transpose() * A).lazy();
            while(rotate){
                rotate=false;
                rotations=0;
                //Perform rotations between rows of B
                for(unsigned int i=0;i<B.cols();i++){
                    for(unsigned int j=i+1;j<B.cols();j++){
                        //calculate plane rotation
                        double p = B.row(i).dot(B.row(j));
                        double qi = B.row(i).dot(B.row(i));
                        double qj = B.row(j).dot(B.row(j));

                        double q=qi-qj;
                        double alpha = pow(p,2.0)/(qi*qj);
                        //if columns are orthogonal with precision
                        //treshold, don't perform rotation and
                        //continue
                        if(alpha<treshold)
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
                        tempi.start(B.cols()) =  cos*B.row(i) + sin*B.row(j);
                        B.row(j) =  - sin*B.row(i) + cos*B.row(j);
                        B.row(i) =  tempi.start(B.cols());


                        //Apply plane rotation to rows of U
                        tempi.start(U.rows()) = cos*U.col(i) + sin*U.col(j);
                        U.col(j) = - sin*U.col(i) + cos*U.col(j);
                        U.col(i) = tempi.start(U.rows());

                        rotate=true;
                    }
                }

                //Only calculate new U and S if there were any rotations
                if(rotations!=0){
                    for(unsigned int i=0;i<V.rows();i++) {
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
