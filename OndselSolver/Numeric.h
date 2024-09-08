/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once
#include <vector>
#include <cstddef>
#include <limits>

// For Windows platforms only, cstddef includes size_t but not ssize_t
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "MbDMath.h"

namespace MbD {
    class Numeric : public MbDMath
    {
        //
    public:
        static double arcTan0to2piYoverX(double y, double x);
        static bool equaltol(double x, double xx, double tol);
        template <typename T>
        static bool isIncreasingVector(std::vector<T>* vec);

    };
    template<typename T>
    inline bool Numeric::isIncreasingVector(std::vector<T>* vec)
    {
        T previous, next;
        next = vec->at(0);
        for (size_t i = 1; i < vec->size(); i++)
        {
            previous = next;
            next = vec->at(i);
            if (previous > next) return false;
        }
        return true;
    }
}

