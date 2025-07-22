// Copyright  (C)  2009  Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>

// Version: 1.0
// Author: Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>
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

#include "chaindynparam.hpp"
#include "frames_io.hpp"
#include <iostream>

namespace KDL {

    ChainDynParam::ChainDynParam(const Chain& _chain, Vector _grav):
            chain(_chain),
            //nr(0),
            nj(chain.getNrOfJoints()),
            ns(chain.getNrOfSegments()),
            grav(_grav),
            jntarraynull(nj),
            chainidsolver_coriolis( chain, Vector::Zero()),
            chainidsolver_gravity( chain, grav),
            wrenchnull(ns,Wrench::Zero()),
            X(ns),
            S(ns),
            Ic(ns)
    {
        ag=-Twist(grav,Vector::Zero());
    }

    //calculate inertia matrix H
    int ChainDynParam::JntToMass(const JntArray &q, JntSpaceInertiaMatrix& H)
    {
	//Check sizes when in debug mode
        if(q.rows()!=nj || H.rows()!=nj || H.columns()!=nj )
            return -1;
        unsigned int k=0;
	double q_;
	
	//Sweep from root to leaf
        for(unsigned int i=0;i<ns;i++)
	{
	  //Collect RigidBodyInertia
          Ic[i]=chain.getSegment(i).getInertia();
          if(chain.getSegment(i).getJoint().getType()!=Joint::None)
	  {
	      q_=q(k);
	      k++;
	  }
	  else
	  {
	    q_=0.0;
	  }
	  X[i]=chain.getSegment(i).pose(q_);//Remark this is the inverse of the frame for transformations from the parent to the current coord frame
	  S[i]=X[i].M.Inverse(chain.getSegment(i).twist(q_,1.0));  
        }
	//Sweep from leaf to root
        int j,l;
	k=nj-1; //reset k
        for(int i=ns-1;i>=0;i--)
	{
	  
	  if(i!=0)
	    {
	      //assumption that previous segment is parent
	      Ic[i-1]=Ic[i-1]+X[i]*Ic[i];
	    } 

	  F=Ic[i]*S[i];
	  if(chain.getSegment(i).getJoint().getType()!=Joint::None)
	  {
	      H(k,k)=dot(S[i],F);
	      j=k; //countervariable for the joints
	      l=i; //countervariable for the segments
	      while(l!=0) //go from leaf to root starting at i
		{
		  //assumption that previous segment is parent
		  F=X[l]*F; //calculate the unit force (cfr S) for every segment: F[l-1]=X[l]*F[l]
		  l--; //go down a segment
		  
		  if(chain.getSegment(l).getJoint().getType()!=Joint::None) //if the joint connected to segment is not a fixed joint
		  {    
		    j--;
		    H(k,j)=dot(F,S[l]); //here you actually match a certain not fixed joint with a segment 
		    H(j,k)=H(k,j);
		  }
		} 
	      k--; //this if-loop should be repeated nj times (k=nj-1 to k=0)
	  }

	}
	return 0;
    }

    //calculate coriolis matrix C
    int ChainDynParam::JntToCoriolis(const JntArray &q, const JntArray &q_dot, JntArray &coriolis)
    {
	//make a null matrix with the size of q_dotdot and a null wrench
	SetToZero(jntarraynull);

	
	//the calculation of coriolis matrix C
	chainidsolver_coriolis.CartToJnt(q, q_dot, jntarraynull, wrenchnull, coriolis);
	
	return 0;
    }

    //calculate gravity matrix G
    int ChainDynParam::JntToGravity(const JntArray &q,JntArray &gravity)
    {

	//make a null matrix with the size of q_dotdot and a null wrench
	
	SetToZero(jntarraynull);
	//the calculation of coriolis matrix C
	chainidsolver_gravity.CartToJnt(q, jntarraynull, jntarraynull, wrenchnull, gravity);
	return 0;
    }

    ChainDynParam::~ChainDynParam()
    {
    }


}
