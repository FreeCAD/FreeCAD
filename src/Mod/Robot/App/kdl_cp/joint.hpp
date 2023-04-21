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

#ifndef KDL_JOINT_HPP
#define KDL_JOINT_HPP

#include "frames.hpp"
#include <string>
#include <exception>


namespace KDL {

    /**
	  * \brief This class encapsulates a simple joint, that is with one
	  * parameterized degree of freedom and with scalar dynamic properties.
     *
     * A simple joint is described by the following properties :
     *      - scale: ratio between motion input and motion output
     *      - offset: between the "physical" and the "logical" zero position.
     *      - type: revolute or translational, along one of the basic frame axes
	  *      - inertia, stiffness and damping: scalars representing the physical
	  *      effects along/about the joint axis only.
     *
     * @ingroup KinematicFamily
     */
    class Joint {
    public:
        typedef enum { RotAxis,RotX,RotY,RotZ,TransAxis,TransX,TransY,TransZ,None} JointType;
        /**
         * Constructor of a joint.
         *
         * @param name of the joint
         * @param type type of the joint, default: Joint::None
         * @param scale scale between joint input and actual geometric
         * movement, default: 1
         * @param offset offset between joint input and actual
         * geometric input, default: 0
         * @param inertia 1D inertia along the joint axis, default: 0
         * @param damping 1D damping along the joint axis, default: 0
         * @param stiffness 1D stiffness along the joint axis,
         * default: 0
         */
        explicit Joint(const std::string& name, const JointType& type=None,const double& scale=1,const double& offset=0,
              const double& inertia=0,const double& damping=0,const double& stiffness=0);
        /**
         * Constructor of a joint.
         *
         * @param type type of the joint, default: Joint::None
         * @param scale scale between joint input and actual geometric
         * movement, default: 1
         * @param offset offset between joint input and actual
         * geometric input, default: 0
         * @param inertia 1D inertia along the joint axis, default: 0
         * @param damping 1D damping along the joint axis, default: 0
         * @param stiffness 1D stiffness along the joint axis,
         * default: 0
         */
        explicit Joint(const JointType& type=None,const double& scale=1,const double& offset=0,
               const double& inertia=0,const double& damping=0,const double& stiffness=0);
        /**
         * Constructor of a joint.
         *
         * @param name of the joint
         * @param _origin the origin of the joint
         * @param _axis the axis of the joint
         * @param type type of the joint
         * @param _scale scale between joint input and actual geometric
         * movement, default: 1
         * @param _offset offset between joint input and actual
         * geometric input, default: 0
         * @param _inertia 1D inertia along the joint axis, default: 0
         * @param _damping 1D damping along the joint axis, default: 0
         * @param _stiffness 1D stiffness along the joint axis,
         * default: 0
         */
        Joint(const std::string& name, const Vector& _origin, const Vector& _axis, const JointType& type, const double& _scale=1, const double& _offset=0,
	      const double& _inertia=0, const double& _damping=0, const double& _stiffness=0);
        /**
         * Constructor of a joint.
         *
         * @param _origin the origin of the joint
         * @param _axis the axis of the joint
         * @param type type of the joint
         * @param _scale scale between joint input and actual geometric
         * movement, default: 1
         * @param _offset offset between joint input and actual
         * geometric input, default: 0
         * @param _inertia 1D inertia along the joint axis, default: 0
         * @param _damping 1D damping along the joint axis, default: 0
         * @param _stiffness 1D stiffness along the joint axis,
         * default: 0
         */
        Joint(const Vector& _origin, const Vector& _axis, const JointType& type, const double& _scale=1, const double& _offset=0,
	      const double& _inertia=0, const double& _damping=0, const double& _stiffness=0);

        /**
         * Request the 6D-pose between the beginning and the end of
         * the joint at joint position q
         *
         * @param q the 1D joint position
         *
         * @return the resulting 6D-pose
         */
        Frame pose(const double& q)const;
        /**
         * Request the resulting 6D-velocity with a joint velocity qdot
         *
         * @param qdot the 1D joint velocity
         *
         * @return the resulting 6D-velocity
         */
        Twist twist(const double& qdot)const;

        /**                                                                     
         * Request the Vector corresponding to the axis of a revolute joint.    
         *                                                                      
         * @return Vector. e.g (1,0,0) for RotX etc.                            
         */
        Vector JointAxis() const;

        /**                                                                     
         * Request the Vector corresponding to the origin of a revolute joint.    
         *                                                                      
         * @return Vector
         */
        Vector JointOrigin() const;
        /**
           * Request the name of the joint
           *
           *
           * @return const reference to the name of the joint
           */
          const std::string& getName()const
          {
              return name;
          }
          /**
         * Request the type of the joint.
         *
         * @return const reference to the type
         */
        const JointType& getType() const
        {
            return type;
        };
      
        /** 
         * Request the stringified type of the joint.
         *
         * @return const string
         */
        const std::string getTypeName() const
        {
            switch (type) {
	    case RotAxis:
	        return "RotAxis";
            case TransAxis:
	        return "TransAxis";
	    case RotX:
                return "RotX";
            case RotY:
                return "RotY";
            case RotZ:
                return "RotZ";
            case TransX:
                return "TransX";
            case TransY:
                return "TransY";
            case TransZ:
                return "TransZ";
            case None:
                return "None";
            default:
                return "None";
            }
        };

        virtual ~Joint();

    private:
        std::string name;
        Joint::JointType type;
        double scale;
        double offset;
        double inertia;
        double damping;
        double stiffness;

        // variables for RotAxis joint
        Vector axis, origin;
        mutable Frame  joint_pose;
        mutable double q_previous;


      
      class joint_type_exception: public std::exception{
	const char* what() const throw() override{
	  return "Joint Type excption";}
      } joint_type_ex;

    };

} // end of namespace KDL

#endif
