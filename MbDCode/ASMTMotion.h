/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTConstraintSet.h"
#include "ForceTorqueData.h"

namespace MbD {
    class ASMTMotion : public ASMTConstraintSet
    {
        //
    public:
        void readMotionSeries(std::vector<std::string>& lines);
        virtual void initMarkers();

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> motionSeries;

    };
}
