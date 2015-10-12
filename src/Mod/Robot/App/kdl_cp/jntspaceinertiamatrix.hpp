// Copyright  (C)  2009  Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>

// Version: 1.0
// Author: Dominick Vanthienen <dominick dot vanthienen at mech dot kuleuven dot be>
// Maintainer: Dominick Vanthienen <ruben dot smits at mech dot kuleuven dot be>
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

#ifndef KDL_JNTSPACEINERTIAMATRIX_HPP
#define KDL_JNTSPACEINERTIAMATRIX_HPP

#include "frames.hpp"
#include "jacobian.hpp"
#include "jntarray.hpp"

#include <Eigen/Core>

namespace KDL
{
    /**
     * @brief This class represents an fixed size matrix containing
     * the Joint-Space Inertia Matrix of a KDL::Chain.
     *
     * \warning An object constructed with the default constructor provides
     * a valid, but inert, object. Many of the member functions will do
     * the correct thing and have no affect on this object, but some 
     * member functions can _NOT_ deal with an inert/empty object. These 
     * functions will assert() and exit the program instead. The intended use 
     * case for the default constructor (in an RTT/OCL setting) is outlined in
     * code below - the default constructor plus the resize() function allow
     * use of JntSpaceInertiaMatrix objects whose size is set within a configureHook() call
     * (typically based on a size determined from a property).
	 
\code
class MyTask : public RTT::TaskContext
{
   JntSpaceInertiaMatrix		j;
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
/endcode

     */

    class JntSpaceInertiaMatrix
    {
    public:
        Eigen::MatrixXd data;

        /** Construct with _no_ data array
         * @post NULL == data
         * @post 0 == rows()
         * @warning use of an object constructed like this, without
         * a resize() first, may result in program exit! See class
         * documentation.
         */
        JntSpaceInertiaMatrix();
        /**
         * Constructor of the Joint-Space Inertia Matrix
         *
         * @param size of the matrix, this cannot be changed
         * afterwards. Size rows and size columns.
         * @pre 0 < size
         * @post NULL != data
         * @post 0 < rows()
         * @post all elements in data have 0 value
         */
        explicit JntSpaceInertiaMatrix(int size);
        /** Copy constructor 
         * @note Will correctly copy an empty object
         */
        JntSpaceInertiaMatrix(const JntSpaceInertiaMatrix& arg);
        ~JntSpaceInertiaMatrix();
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
		
        JntSpaceInertiaMatrix& operator = ( const JntSpaceInertiaMatrix& arg);
        /**
         * get_item operator for the joint matrix
         *
         *
         * @return the joint value at position i, starting from 0
         * @pre 0 != size (ie non-default constructor or resize() called)
         */
        double operator()(unsigned int i,unsigned int j)const;
        /**
         * set_item operator
         *
         * @return reference to the joint value at position i,starting
         *from zero.
         * @pre 0 != size (ie non-default constructor or resize() called)
         */
        double& operator()(unsigned int i,unsigned int j);
        /**
         * Returns the number of rows and columns of the matrix
         *
         */
        unsigned int rows()const;
        /**
         * Returns the number of columns of the matrix.
         */
        unsigned int columns()const;

        friend void Add(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest);
        friend void Subtract(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest);
        friend void Multiply(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest);
        friend void Divide(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest);
        friend void Multiply(const JntSpaceInertiaMatrix& src, const JntArray& vec, JntArray& dest);
        friend void SetToZero(JntSpaceInertiaMatrix& matrix);
        friend bool Equal(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,double eps);
        friend bool operator==(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2);
        //friend bool operator!=(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2);
    };

    bool operator==(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2);
    //bool operator!=(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2);

    /**
     * Function to add two joint matrix, all the arguments must
     * have the same size: A + B = C. This function is
     * aliasing-safe, A or B can be the same array as C.
     *
     * @param src1 A
     * @param src2 B
     * @param dest C
     */
    void Add(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest);

    /**
     * Function to subtract two joint matrix, all the arguments must
     * have the same size: A - B = C. This function is
     * aliasing-safe, A or B can be the same array as C.
     *
     * @param src1 A
     * @param src2 B
     * @param dest C
     */
    void Subtract(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,JntSpaceInertiaMatrix& dest);

    /**
     * Function to multiply all the array values with a scalar
     * factor: A*b=C. This function is aliasing-safe, A can be the
     * same array as C.
     *
     * @param src A
     * @param factor b
     * @param dest C
     */
    void Multiply(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest);

    /**
     * Function to divide all the array values with a scalar
     * factor: A/b=C. This function is aliasing-safe, A can be the
     * same array as C.
     *
     * @param src A
     * @param factor b
     * @param dest C
     */
    void Divide(const JntSpaceInertiaMatrix& src,const double& factor,JntSpaceInertiaMatrix& dest);

    /**
     * Function to multiply a KDL::Jacobian with a KDL::JntSpaceInertiaMatrix
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
    void Multiply(const JntSpaceInertiaMatrix& src, const JntArray& vec, JntArray& dest);

    /**
     * Function to set all the values of the array to 0
     *
     * @param array
     */
    void SetToZero(JntSpaceInertiaMatrix& matrix);

    /**
     * Function to check if two matrices are the same with a
     *precision of eps
     *
     * @param src1
     * @param src2
     * @param eps default: epsilon
     * @return true if each element of src1 is within eps of the same
     * element in src2, or if both src1 and src2 have no data (ie 0==rows())
     */
    bool Equal(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2,double eps=epsilon);

    bool operator==(const JntSpaceInertiaMatrix& src1,const JntSpaceInertiaMatrix& src2);

}

#endif
