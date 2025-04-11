/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Numeric.h"

namespace MbD {
    class Units : public Numeric
    {
        //time mass aJ length angle velocity omega acceleration alpha force torque 
    public:
        Units();
        Units(double unitTime, double unitMass, double unitLength, double unitAngle);
        void initialize();
        void calc();

        double time = 1.0, mass = 1.0, aJ = 1.0, length = 1.0, angle = 1.0;
        double velocity = 1.0, omega = 1.0, acceleration = 1.0, alpha = 1.0;
        double force = 1.0, torque = 1.0;


    };
}
