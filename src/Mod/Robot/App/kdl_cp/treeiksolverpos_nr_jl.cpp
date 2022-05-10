// Copyright  (C)  2007-2008  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008  Mikael Mayer
// Copyright  (C)  2008  Julia Jesse

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

#include "treeiksolverpos_nr_jl.hpp"

namespace KDL {
    TreeIkSolverPos_NR_JL::TreeIkSolverPos_NR_JL(const Tree& _tree,
                                                 const std::vector<std::string>& _endpoints,
                                                 const JntArray& _q_min, const JntArray& _q_max,
                                                 TreeFkSolverPos& _fksolver, TreeIkSolverVel& _iksolver,
                                                 unsigned int _maxiter, double _eps) :
        tree(_tree), q_min(_q_min), q_max(_q_max), iksolver(_iksolver),
        fksolver(_fksolver), delta_q(tree.getNrOfJoints()),
        endpoints(_endpoints), maxiter(_maxiter), eps(_eps)
    {
        for (size_t i = 0; i < endpoints.size(); i++) {
            frames.insert(Frames::value_type(endpoints[i], Frame::Identity()));
            delta_twists.insert(Twists::value_type(endpoints[i], Twist::Zero()));
        }
    }
    
    double TreeIkSolverPos_NR_JL::CartToJnt(const JntArray& q_init, const Frames& p_in, JntArray& q_out) {
        q_out = q_init;
        
        //First check if all elements in p_in are available:
        for(Frames::const_iterator f_des_it=p_in.begin();f_des_it!=p_in.end();++f_des_it)
            if(frames.find(f_des_it->first)==frames.end())
                return -2;
        
        unsigned int k=0;
        while(++k <= maxiter) {
            for (Frames::const_iterator f_des_it=p_in.begin();f_des_it!=p_in.end();++f_des_it){
                //Get all iterators for this endpoint
                Frames::iterator f_it = frames.find(f_des_it->first);
                Twists::iterator delta_twist = delta_twists.find(f_des_it->first);
                
                fksolver.JntToCart(q_out, f_it->second, f_it->first);
                delta_twist->second = diff(f_it->second, f_des_it->second);
            }
            double res = iksolver.CartToJnt(q_out, delta_twists, delta_q);
            if (res < eps)
                return res;
            
            Add(q_out, delta_q, q_out);
            
            for (unsigned int j = 0; j < q_min.rows(); j++) {
                if (q_out(j) < q_min(j))
                    q_out( j) = q_min(j);
                else if (q_out(j) > q_max(j))
                    q_out( j) = q_max(j);
            }
        }
        if (k <= maxiter)
            return 0;
        else
            return -3;
    }
    
    
    TreeIkSolverPos_NR_JL::~TreeIkSolverPos_NR_JL() {
        
    }
    
}//namespace

