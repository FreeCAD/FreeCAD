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
    class ASMTJoint : public ASMTConstraintSet
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readJointSeries(std::vector<std::string>& lines);

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> jointSeries;

    };
}
