/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Joint.h"

namespace MbD {
    class AngleJoint : public Joint
    {
        //theIzJz
    public:
        AngleJoint();
        AngleJoint(const std::string& str);
        void initializeGlobally() override;

        double theIzJz = 0.0;
    };
}

