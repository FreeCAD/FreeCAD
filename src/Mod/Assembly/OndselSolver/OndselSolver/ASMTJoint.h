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
        static std::shared_ptr<ASMTJoint> With();
        void readJointSeries(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, size_t level) override;
        void storeOnTimeSeries(std::ofstream& os) override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

        std::shared_ptr<std::vector<std::shared_ptr<ForceTorqueData>>> jointSeries;

    };
}
