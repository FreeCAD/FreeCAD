/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTConstraintSet.h"

namespace MbD {
    class ForceTorqueData;

    class ASMTJoint : public ASMTConstraintSet
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readJointSeries(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, int level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> jointSeries;

    };
}
