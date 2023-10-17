/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynElement.h"
#include "MBDynMarker.h"

namespace MbD {
    class MBDynJoint : public MBDynElement
    {
    public:
        void initialize() override;
        void parseMBDyn(std::string line);
        void readMarkerI(std::vector<std::string>& args);
        void readMarkerJ(std::vector<std::string>& args);
        void readFunction(std::vector<std::string>& args);
        void createASMT() override;

        std::string jointString, joint_type, node_1_label, node_2_label;
        std::shared_ptr<MBDynMarker> mkr1, mkr2;
        std::string formula;
    };
}
