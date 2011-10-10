// Copyright  (C)  2007  Ruben Smits <ruben dot smits at mech dot kuleuven dot be>

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

#include "joint.hpp"

namespace KDL {

    // constructor for joint along x,y or z axis, at origin of reference frame
    Joint::Joint(const std::string& _name, const JointType& _type, const double& _scale, const double& _offset,
                 const double& _inertia, const double& _damping, const double& _stiffness):
      name(_name),type(_type),scale(_scale),offset(_offset),inertia(_inertia),damping(_damping),stiffness(_stiffness)
    {
      if (type == RotAxis || type == TransAxis) throw joint_type_ex;
    }

    // constructor for joint along x,y or z axis, at origin of reference frame
    Joint::Joint(const JointType& _type, const double& _scale, const double& _offset,
                 const double& _inertia, const double& _damping, const double& _stiffness):
      name("NoName"),type(_type),scale(_scale),offset(_offset),inertia(_inertia),damping(_damping),stiffness(_stiffness)
    {
      if (type == RotAxis || type == TransAxis) throw joint_type_ex;
    }

    // constructor for joint along arbitrary axis, at arbitrary origin
    Joint::Joint(const std::string& _name, const Vector& _origin, const Vector& _axis, const JointType& _type, const double& _scale, 
                 const double& _offset, const double& _inertia, const double& _damping, const double& _stiffness):
      name(_name), origin(_origin), axis(_axis / _axis.Norm()), type(_type),scale(_scale),offset(_offset),inertia(_inertia),damping(_damping),stiffness(_stiffness)
    {
      if (type != RotAxis && type != TransAxis) throw joint_type_ex;

      // initialize
      joint_pose.p = origin;
      joint_pose.M = Rotation::Rot2(axis, offset);
      q_previous = 0;
    }

    // constructor for joint along arbitrary axis, at arbitrary origin
    Joint::Joint(const Vector& _origin, const Vector& _axis, const JointType& _type, const double& _scale, 
                 const double& _offset, const double& _inertia, const double& _damping, const double& _stiffness):
      name("NoName"), origin(_origin), axis(_axis / _axis.Norm()), type(_type),scale(_scale),offset(_offset),inertia(_inertia),damping(_damping),stiffness(_stiffness)
    {
      if (type != RotAxis && type != TransAxis) throw joint_type_ex;

      // initialize
      joint_pose.p = origin;
      joint_pose.M = Rotation::Rot2(axis, offset);
      q_previous = 0;
    }

    Joint::~Joint()
    {
    }

    Frame Joint::pose(const double& q)const
    {

        switch(type){
        case RotAxis:{
            // calculate the rotation matrix around the vector "axis"
            if (q != q_previous){
                q_previous = q;
                joint_pose.M = Rotation::Rot2(axis, scale*q+offset);
            }
            return joint_pose;
            break;}
        case RotX:
            return Frame(Rotation::RotX(scale*q+offset));
            break;
        case RotY:
            return  Frame(Rotation::RotY(scale*q+offset));
            break;
        case RotZ:
            return  Frame(Rotation::RotZ(scale*q+offset));
            break;
        case TransAxis:
            return Frame(origin + (axis * (scale*q+offset)));
            break;
        case TransX:
            return  Frame(Vector(scale*q+offset,0.0,0.0));
            break;
        case TransY:
            return Frame(Vector(0.0,scale*q+offset,0.0));
            break;
        case TransZ:
            return Frame(Vector(0.0,0.0,scale*q+offset));
            break;
        case None:
            return Frame::Identity();
            break;
        }
    }

    Twist Joint::twist(const double& qdot)const
    {
        switch(type){
        case RotAxis:
            return Twist(Vector(0,0,0), axis * ( scale * qdot));
            break;
        case RotX:
            return Twist(Vector(0.0,0.0,0.0),Vector(scale*qdot,0.0,0.0));
            break;
        case RotY:
            return Twist(Vector(0.0,0.0,0.0),Vector(0.0,scale*qdot,0.0));
            break;
        case RotZ:
            return Twist(Vector(0.0,0.0,0.0),Vector(0.0,0.0,scale*qdot));
            break;
	case TransAxis:
	    return Twist(axis * (scale * qdot), Vector(0,0,0));
	    break;
        case TransX:
            return Twist(Vector(scale*qdot,0.0,0.0),Vector(0.0,0.0,0.0));
            break;
        case TransY:
            return Twist(Vector(0.0,scale*qdot,0.0),Vector(0.0,0.0,0.0));
            break;
        case TransZ:
            return Twist(Vector(0.0,0.0,scale*qdot),Vector(0.0,0.0,0.0));
            break;
        case None:
            return Twist::Zero();
            break;
        }
    }

  Vector Joint::JointAxis() const
  {
    switch(type)
      {
      case RotAxis:
        return axis;
        break;
      case RotX:
        return Vector(1.,0.,0.);
        break;
      case RotY:
        return Vector(0.,1.,0.);
        break;
      case RotZ:
        return Vector(0.,0.,1.);
        break;
      case TransAxis:
        return axis;
        break;
      case TransX:
        return Vector(1.,0.,0.);
        break;
      case TransY:
        return Vector(0.,1.,0.);
        break;
      case TransZ:
        return Vector(0.,0.,1.);
        break;
      case None:
        return Vector::Zero();
        break;
      }
  }

  Vector Joint::JointOrigin() const
  {
    return origin;
  }

} // end of namespace KDL

