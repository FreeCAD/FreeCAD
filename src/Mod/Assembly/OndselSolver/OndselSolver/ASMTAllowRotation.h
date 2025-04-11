/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ASMTMotion.h"

namespace MbD {
    class ASMTAllowRotation : public ASMTMotion
    {
        //
    public:
        static std::shared_ptr<ASMTAllowRotation> With();
        void parseASMT(std::vector<std::string>& lines) override;
        void readMotionJoint(std::vector<std::string>& lines);
        void initMarkers() override;
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        void setMotionJoint(const std::string& motionJoint);
        void storeOnLevel(std::ofstream& os, size_t level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::string motionJoint, rotationZ;
    };
}
