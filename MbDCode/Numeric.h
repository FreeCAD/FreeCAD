/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once
#include <vector>

#include "Math.h"

namespace MbD {
    class Numeric : public Math
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
        for (int i = 1; i < vec->size(); i++)
        {
            previous = next;
            next = vec->at(i);
            if (previous > next) return false;
        }
        return true;
    }
}

