// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <cmath>

namespace Base
{

// The methods are copied from OCC's Precision class
class Precision
{
public:
    /*!
     * \brief Angular
     * Returns the recommended precision value when checking the equality of two angles (given in
     * radians). \return
     */
    static double Angular()
    {
        return 1.e-12;
    }

    /*!
     * \brief Confusion
     * Returns the recommended precision value when checking coincidence of two points in real
     * space. \return
     */
    static double Confusion()
    {
        return 1.e-7;
    }

    /*!
     * \brief SquareConfusion
     * Returns square of \ref Confusion.
     * \return
     */
    static double SquareConfusion()
    {
        return Confusion() * Confusion();
    }

    /*!
     * \brief Intersection
     * Returns the precision value in real space, frequently
     * used by intersection algorithms to decide that a solution is reached.
     * \return
     */
    static double Intersection()
    {
        return Confusion() * 0.01;
    }

    /*!
     * \brief Approximation
     * Returns the precision value in real space, frequently used
     * by approximation algorithms.
     * \return
     */
    static double Approximation()
    {
        return Confusion() * 10.0;
    }

    /*!
     * \brief Parametric
     * Convert a real  space precision  to  a  parametric space precision.
     * \param P
     * \param T
     * \return
     */
    static double Parametric(const double P, const double T)
    {
        return P / T;
    }

    /*!
     * \brief PConfusion
     * Returns a precision value in parametric space.
     * \param T
     * \return
     */
    static double PConfusion(const double T)
    {
        return Parametric(Confusion(), T);
    }

    /*!
     * \brief PConfusion
     * Used  to test distances  in parametric  space on a default curve.
     * \return
     */
    static double PConfusion()
    {
        return Parametric(Confusion());
    }

    /*!
     * \brief SquarePConfusion
     * Returns square of \ref PConfusion.
     * \return
     */
    static double SquarePConfusion()
    {
        return PConfusion() * PConfusion();
    }

    /*!
     * \brief PIntersection
     * Returns a precision value in parametric space, which may be used by intersection algorithms,
     * to decide that a solution is reached.
     * \param T
     * \return
     */
    static double PIntersection(const double T)
    {
        return Parametric(Intersection(), T);
    }

    /*!
     * \brief PApproximation
     *  Returns a precision value in parametric space, which may be used by approximation
     * algorithms. \param T \return
     */
    static double PApproximation(const double T)
    {
        return Parametric(Approximation(), T);
    }

    /*!
     * \brief Parametric
     * Convert a real  space precision  to  a  parametric space precision on a default curve.
     * \param P
     * \return
     */
    static double Parametric(const double P)
    {
        return Parametric(P, 100.0);
    }

    /*!
     * \brief PIntersection
     * Used for Intersections  in parametric  space  on a default curve.
     * \return
     */
    static double PIntersection()
    {
        return Parametric(Intersection());
    }

    /*!
     * \brief PApproximation
     * Used for  Approximations  in parametric space on a default curve.
     * \return
     */
    static double PApproximation()
    {
        return Parametric(Approximation());
    }

    /*!
     * \brief IsInfinite
     * Returns True if R may be considered as an infinite number. Currently Abs(R) > 1e100
     * \param R
     * \return
     */
    static bool IsInfinite(const double R)
    {
        return std::fabs(R) >= (0.5 * Precision::Infinite());
    }

    /*!
     * \brief IsPositiveInfinite
     * Returns True if R may be considered as  a positive infinite number. Currently R > 1e100
     * \param R
     * \return
     */
    static bool IsPositiveInfinite(const double R)
    {
        return R >= (0.5 * Precision::Infinite());
    }

    /*!
     * \brief IsNegativeInfinite
     * Returns True if R may  be considered as a negative infinite number. Currently R < -1e100
     * \param R
     * \return
     */
    static bool IsNegativeInfinite(const double R)
    {
        return R <= -(0.5 * Precision::Infinite());
    }

    /*!
     * \brief Infinite
     * Returns a  big number that  can  be  considered as infinite. Use -Infinite() for a negative
     * big number. \return
     */
    static double Infinite()
    {
        return 2.e+100;
    }
};

}  // namespace Base
