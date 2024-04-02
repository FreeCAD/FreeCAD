/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ASMTJoint.h"

namespace MbD {
    class ASMTGearJoint : public ASMTJoint
    {
        //
    public:
        static std::shared_ptr<ASMTGearJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        void parseASMT(std::vector<std::string>& lines) override;
        void readRadiusI(std::vector<std::string>& lines);
        void readRadiusJ(std::vector<std::string>& lines);
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        void storeOnLevel(std::ofstream& os, size_t level) override;

        double radiusI = 0.0, radiusJ = 0.0, aConstant = 0.0;
    };
}

