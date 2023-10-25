/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include "kdl_cp/chainfksolverpos_recursive.hpp"
#include "kdl_cp/chainiksolverpos_nr.hpp"
#include "kdl_cp/chainiksolvervel_pinv.hpp"
#include "kdl_cp/frames_io.hpp"
#endif

#include "RobotAlgos.h"


using namespace Robot;
using namespace std;
using namespace KDL;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#define M_PI 3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif

//===========================================================================
// FeatureView
//===========================================================================

RobotAlgos::RobotAlgos() = default;

RobotAlgos::~RobotAlgos() = default;

void RobotAlgos::Test()
{
    // Definition of a kinematic chain & add segments to the chain
    KDL::Chain chain;
    chain.addSegment(Segment(Joint(Joint::RotZ), Frame(Vector(0.0, 0.0, 1.020))));
    chain.addSegment(Segment(Joint(Joint::RotX), Frame(Vector(0.0, 0.0, 0.480))));
    chain.addSegment(Segment(Joint(Joint::RotX), Frame(Vector(0.0, 0.0, 0.645))));
    chain.addSegment(Segment(Joint(Joint::RotZ)));
    chain.addSegment(Segment(Joint(Joint::RotX), Frame(Vector(0.0, 0.0, 0.120))));
    chain.addSegment(Segment(Joint(Joint::RotZ)));

    // Create solver based on kinematic chain
    ChainFkSolverPos_recursive fksolver = ChainFkSolverPos_recursive(chain);

    // Create joint array
    unsigned int nj = chain.getNrOfJoints();
    KDL::JntArray jointpositions = JntArray(nj);

    // Assign some values to the joint positions
    for (unsigned int i = 0; i < nj; i++) {
        float myinput;
        printf("Enter the position of joint %i: ", i);
        int result = scanf("%e", &myinput);
        (void)result;
        jointpositions(i) = (double)myinput;
    }

    // Create the frame that will contain the results
    KDL::Frame cartpos;

    // Calculate forward position kinematics
    int kinematics_status;
    kinematics_status = fksolver.JntToCart(jointpositions, cartpos);
    if (kinematics_status >= 0) {
        std::cout << cartpos << std::endl;
        printf("%s \n", "Success, thanks KDL!");
    }
    else {
        printf("%s \n", "Error: could not calculate forward kinematics :(");
    }

    // Creation of the solvers:
    ChainFkSolverPos_recursive fksolver1(chain);  // Forward position solver
    ChainIkSolverVel_pinv iksolver1v(chain);      // Inverse velocity solver
    ChainIkSolverPos_NR iksolver1(chain,
                                  fksolver1,
                                  iksolver1v,
                                  100,
                                  1e-6);  // Maximum 100 iterations, stop at accuracy 1e-6

    // Creation of jntarrays:
    JntArray result(chain.getNrOfJoints());
    JntArray q_init(chain.getNrOfJoints());

    // Set destination frame
    Frame F_dest = cartpos;

    iksolver1.CartToJnt(q_init, F_dest, result);

    for (unsigned int i = 0; i < nj; i++) {
        printf("Axle %i: %f \n", i, result(i));
    }
}
