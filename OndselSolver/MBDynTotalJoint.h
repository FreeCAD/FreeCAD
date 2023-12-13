/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynJoint.h"

namespace MbD {
    class MBDynTotalJoint : public MBDynJoint
    {
    public:
        void parseMBDyn(std::string line) override;
        void readMarkerI(std::vector<std::string>& args) override;
        void readMarkerJ(std::vector<std::string>& args) override;
        void readPositionConstraints(std::vector<std::string>& args);
        void readOrientationConstraints(std::vector<std::string>& args);
        void readPositionFormulas(std::vector<std::string>& args);
        void readOrientationFormulas(std::vector<std::string>& args);
        void createASMT() override;

        std::vector<std::string> positionConstraints = std::vector<std::string>(3);
        std::vector<std::string> orientationConstraints = std::vector<std::string>(3);
        std::vector<std::string> positionFormulas = std::vector<std::string>(3);
        std::vector<std::string> orientationFormulas = std::vector<std::string>(3);
    };
}
