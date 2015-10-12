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

#ifndef KDL_JNTARRAY_HPP
#define KDL_JNTARRAY_HPP

#include "frames.hpp"
#include "jacobian.hpp"

#include <Eigen/Core>

namespace KDL
{
    /**
     * @brief This class represents an fixed size array containing
     * joint values of a KDL::Chain.
     *
     * \warning An object constructed with the default constructor provides
     * a valid, but inert, object. Many of the member functions will do
     * the correct thing and have no affect on this object, but some 
     * member functions can _NOT_ deal with an inert/empty object. These 
     * functions will assert() and exit the program instead. The intended use 
     * case for the default constructor (in an RTT/OCL setting) is outlined in
     * code below - the default constructor plus the resize() function allow
     * use of JntArray objects whose size is set within a configureHook() call
     * (typically based on a size determined from a property).
	 
\code
class MyTask : public RTT::TaskContext
{
   JntArray		j;
   MyTask() 
   {}			// invokes j's default constructor

   bool configureHook()
   {
       unsigned int size = some_property.rvalue();
	   j.resize(size)
	   ...
   }

   void updateHook()
   {
   ** use j here
   }
};
\endcode

     */	

    class JntArray
    {
    public:
        Eigen::VectorXd data;

        /** Construct with _no_ data array
         * @post NULL == data
         * @post 0 == rows()
         * @warning use of an object constructed like this, without
         * a resize() first, may result in program exit! See class
         * documentation.
         */
        JntArray();
        /**
         * Constructor of the joint array
         *
         * @param size size of the array, this cannot be changed
         * afterwards.
         * @pre 0 < size
         * @post NULL != data
         * @post 0 < rows()
         * @post all elements in data have 0 value
         */
        explicit JntArray(unsigned int size);
        /** Copy constructor 
         * @note Will correctly copy an empty object
         */
        JntArray(const JntArray& arg);
        ~JntArray();
        /** Resize the array 
         * @warning This causes a dynamic allocation (and potentially 	
         * also a dynamic deallocation). This _will_ negatively affect
         * real-time performance! 
         *
         * @post newSize == rows()
         * @post NULL != data
         * @post all elements in data have 0 value
         */
        void resize(unsigned int newSize);
		
        JntArray& operator = ( const JntArray& arg);
        /**
         * get_item operator for the joint array, if a second value is
         * given it should be zero, since a JntArray resembles a column.
         *
         *
         * @return the joint value at position i, starting from 0
         * @pre 0 != size (ie non-default constructor or resize() called)
         */
        double operator()(unsigned int i,unsigned int j=0)const;
        /**
         * set_item operator, again if a second value is given it
         *should be zero.
         *
         * @return reference to the joint value at position i,starting
         *from zero.
         * @pre 0 != size (ie non-default constructor or resize() called)
         */
        double& operator()(unsigned int i,unsigned int j=0);
        /**
         * Returns the number of rows (size) of the array
         *
         */
        unsigned int rows()const;
        /**
         * Returns the number of columns of the array, always 1.
         */
        unsigned int columns()const;

        friend void Add(const JntArray& src1,const JntArray& src2,JntArray& dest);
        friend void Subtract(const JntArray& src1,const JntArray& src2,JntArray& dest);
        friend void Multiply(const JntArray& src,const double& factor,JntArray& dest);
        friend void Divide(const JntArray& src,const double& factor,JntArray& dest);
        friend void MultiplyJacobian(const Jacobian& jac, const JntArray& src, Twist& dest);
        friend void SetToZero(JntArray& array);
        friend bool Equal(const JntArray& src1,const JntArray& src2,double eps);

        friend bool operator==(const JntArray& src1,const JntArray& src2);
        //friend bool operator!=(const JntArray& src1,const JntArray& src2);
        };

    bool operator==(const JntArray& src1,const JntArray& src2);
    //bool operator!=(const JntArray& src1,const JntArray& src2);

    /**
     * Function to add two joint arrays, all the arguments must
     * have the same size: A + B = C. This function is
     * aliasing-safe, A or B can be the same array as C.
     *
     * @param src1 A
     * @param src2 B
     * @param dest C
     */
    void Add(const JntArray& src1,const JntArray& src2,JntArray& dest);
    /**
     * Function to subtract two joint arrays, all the arguments must
     * have the same size: A - B = C. This function is
     * aliasing-safe, A or B can be the same array as C.
     *
     * @param src1 A
     * @param src2 B
     * @param dest C
     */
    void Subtract(const JntArray& src1,const JntArray& src2,JntArray& dest);
    /**
     * Function to multiply all the array values with a scalar
     * factor: A*b=C. This function is aliasing-safe, A can be the
     * same array as C.
     *
     * @param src A
     * @param factor b
     * @param dest C
     */
    void Multiply(const JntArray& src,const double& factor,JntArray& dest);
    /**
     * Function to divide all the array values with a scalar
     * factor: A/b=C. This function is aliasing-safe, A can be the
     * same array as C.
     *
     * @param src A
     * @param factor b
     * @param dest C
     */
    void Divide(const JntArray& src,const double& factor,JntArray& dest);
    /**
     * Function to multiply a KDL::Jacobian with a KDL::JntArray
     * to get a KDL::Twist, it should not be used to calculate the
     * forward velocity kinematics, the solver classes are built
     * for this purpose.
     * J*q = t
     *
     * @param jac J
     * @param src q
     * @param dest t
     * @post dest==Twist::Zero() if 0==src.rows() (ie src is empty)
     */
    void MultiplyJacobian(const Jacobian& jac, const JntArray& src, Twist& dest);
    /**
     * Function to set all the values of the array to 0
     *
     * @param array
     */
    void SetToZero(JntArray& array);
    /**
     * Function to check if two arrays are the same with a
     *precision of eps
     *
     * @param src1
     * @param src2
     * @param eps default: epsilon
     * @return true if each element of src1 is within eps of the same
     * element in src2, or if both src1 and src2 have no data (ie 0==rows())
     */
    bool Equal(const JntArray& src1,const JntArray& src2,double eps=epsilon);

}

#endif
