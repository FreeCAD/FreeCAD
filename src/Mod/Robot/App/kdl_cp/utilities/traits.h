// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "utility.h"

 
// forwards declarations :
namespace KDL {
   class Frame;
    class Rotation;
    class Vector;
    class Twist;
    class Wrench;
	class FrameVel;
	class RotationVel;
	class VectorVel;
	class TwistVel;
}


/**
 * @brief Traits are traits classes to determine the type of a derivative of another type.
 *
 * For geometric objects the "geometric" derivative is chosen.  For example the derivative of a Rotation
 * matrix is NOT a 3x3 matrix containing the derivative of the elements of a rotation matrix.  The derivative
 * of the rotation matrix is a Vector corresponding the rotational velocity.  Mostly used in template classes
 * and routines to derive a correct type when needed.
 * 
 * You can see this as a compile-time lookuptable to find the type of the derivative.
 *
 * Example
 * \verbatim
	Rotation R;
    Traits<Rotation> dR;
   \endverbatim
 */
template <typename T>
struct Traits {
	typedef T valueType;
	typedef T derivType;
};

template <>
struct Traits<KDL::Frame> {
	typedef KDL::Frame valueType;
	typedef KDL::Twist derivType;
};
template <>
struct Traits<KDL::Twist> {
	typedef KDL::Twist valueType;
	typedef KDL::Twist derivType;
};
template <>
struct Traits<KDL::Wrench> {
	typedef KDL::Wrench valueType;
	typedef KDL::Wrench derivType;
};

template <>
struct Traits<KDL::Rotation> {
	typedef KDL::Rotation valueType;
	typedef KDL::Vector derivType;
};

template <>
struct Traits<KDL::Vector> {
	typedef KDL::Vector valueType;
	typedef KDL::Vector derivType;
};

template <>
struct Traits<double> {
	typedef double valueType;
	typedef double derivType;
};

template <>
struct Traits<float> {
	typedef float valueType;
	typedef float derivType;
};

template <>
struct Traits<KDL::FrameVel> {
	typedef KDL::Frame valueType;
	typedef KDL::TwistVel derivType;
};
template <>
struct Traits<KDL::TwistVel> {
	typedef KDL::Twist valueType;
	typedef KDL::TwistVel derivType;
};

template <>
struct Traits<KDL::RotationVel> {
	typedef KDL::Rotation valueType;
	typedef KDL::VectorVel derivType;
};

template <>
struct Traits<KDL::VectorVel> {
	typedef KDL::Vector valueType;
	typedef KDL::VectorVel derivType;
};