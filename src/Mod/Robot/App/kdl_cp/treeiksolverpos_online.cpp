// Copyright  (C)  2011  PAL Robotics S.L.  All rights reserved.
// Copyright  (C)  2007-2008  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>
// Copyright  (C)  2008  Mikael Mayer
// Copyright  (C)  2008  Julia Jesse

// Version: 1.0
// Author: Marcus Liebhardt
// This class has been derived from the KDL::TreeIkSolverPos_NR_JL class
// by Julia Jesse, Mikael Mayer and Ruben Smits

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

#include "treeiksolverpos_online.hpp"

namespace KDL {

TreeIkSolverPos_Online::TreeIkSolverPos_Online(const double& nr_of_jnts,
                                               const std::vector<std::string>& endpoints,
                                               const JntArray& q_min,
                                               const JntArray& q_max,
                                               const JntArray& q_dot_max,
                                               const double x_dot_trans_max,
                                               const double x_dot_rot_max,
                                               TreeFkSolverPos& fksolver,
                                               TreeIkSolverVel& iksolver) :
                                               q_min_(nr_of_jnts),
                                               q_max_(nr_of_jnts),
                                               q_dot_max_(nr_of_jnts),
                                               fksolver_(fksolver),
                                               iksolver_(iksolver),
                                               q_dot_(nr_of_jnts)
{
    q_min_ = q_min;
    q_max_ = q_max;
    q_dot_max_ = q_dot_max;
    x_dot_trans_max_ = x_dot_trans_max;
    x_dot_rot_max_ = x_dot_rot_max;

    for (size_t i = 0; i < endpoints.size(); i++)
    {
      frames_.insert(Frames::value_type(endpoints[i], Frame::Identity()));
      delta_twists_.insert(Twists::value_type(endpoints[i], Twist::Zero()));
    }
}


TreeIkSolverPos_Online::~TreeIkSolverPos_Online()
{}


double TreeIkSolverPos_Online::CartToJnt(const JntArray& q_in, const Frames& p_in, JntArray& q_out)
{
  q_out = q_in;

  // First check, if all elements in p_in are available
  for(Frames::const_iterator f_des_it=p_in.begin();f_des_it!=p_in.end();++f_des_it)
    if(frames_.find(f_des_it->first)==frames_.end())
      return -2;

  for (Frames::const_iterator f_des_it=p_in.begin();f_des_it!=p_in.end();++f_des_it)
  {
    // Get all iterators for this endpoint
    Frames::iterator f_it = frames_.find(f_des_it->first);
    Twists::iterator delta_twists_it = delta_twists_.find(f_des_it->first);

    fksolver_.JntToCart(q_out, f_it->second, f_it->first);
    twist_ = diff(f_it->second, f_des_it->second);

    // Checks, if the twist (twist_) exceeds the maximum translational and/or rotational velocity
    // And scales them, if necessary
    enforceCartVelLimits();

    delta_twists_it->second = twist_;
  }

  double res = iksolver_.CartToJnt(q_out, delta_twists_, q_dot_);

  if(res<0)
      return res;
  //If we got here q_out is definitely of the right size
  if(q_out.rows()!=q_min_.rows() || q_out.rows()!=q_max_.rows() || q_out.rows()!= q_dot_max_.rows())
      return -1;

  // Checks, if joint velocities (q_dot_) exceed their maximum and scales them, if necessary
  enforceJointVelLimits();

  // Integrate
  Add(q_out, q_dot_, q_out);

  // Limit joint positions
  for (unsigned int j = 0; j < q_min_.rows(); j++)
  {
    if (q_out(j) < q_min_(j))
      q_out(j) = q_min_(j);
    else if (q_out(j) > q_max_(j))
      q_out(j) = q_max_(j);
  }

  return res;
}


void TreeIkSolverPos_Online::enforceJointVelLimits()
{
  // check, if one (or more) joint velocities exceed the maximum value
  // and if so, safe the biggest overshoot for scaling q_dot_ properly
  // to keep the direction of the velocity vector the same
  double rel_os, rel_os_max = 0.0; // relative overshoot and the biggest relative overshoot
  bool max_exceeded = false;

  for (unsigned int i = 0; i < q_dot_.rows(); i++)
  {
    if ( q_dot_(i) > q_dot_max_(i) )
    {
      max_exceeded = true;
      rel_os = (q_dot_(i) - q_dot_max_(i)) / q_dot_max_(i);
      if ( rel_os > rel_os_max )
      {
        rel_os_max = rel_os;
      }
    }
    else if ( q_dot_(i) < -q_dot_max_(i) )
    {
      max_exceeded = true;
      rel_os = (-q_dot_(i) - q_dot_max_(i)) / q_dot_max_(i);
      if ( rel_os > rel_os_max)
      {
        rel_os_max = rel_os;
      }
    }
  }

  // scales q_out, if one joint exceeds the maximum value
  if ( max_exceeded == true )
  {
    Multiply(q_dot_, ( 1.0 / ( 1.0 + rel_os_max ) ), q_dot_);
  }
}


void TreeIkSolverPos_Online::enforceCartVelLimits()
{
  double x_dot_trans, x_dot_rot; // vector lengths
  x_dot_trans = sqrt( pow(twist_.vel.x(), 2) + pow(twist_.vel.y(), 2) + pow(twist_.vel.z(), 2));
  x_dot_rot = sqrt( pow(twist_.rot.x(), 2) + pow(twist_.rot.y(), 2) + pow(twist_.rot.z(), 2));

  if ( x_dot_trans > x_dot_trans_max_ || x_dot_rot > x_dot_rot_max_ )
  {
    if ( x_dot_trans > x_dot_rot )
    {
      twist_.vel = twist_.vel * ( x_dot_trans_max_ / x_dot_trans );
      twist_.rot = twist_.rot * ( x_dot_trans_max_ / x_dot_trans );
    }
    else if ( x_dot_rot > x_dot_trans )
    {
      twist_.vel = twist_.vel * ( x_dot_rot_max_ / x_dot_rot );
      twist_.rot = twist_.rot * ( x_dot_rot_max_ / x_dot_rot );
    }
  }
}

} // namespace

