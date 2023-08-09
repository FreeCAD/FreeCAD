/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTMotion.h"
#include "ZRotation.h"

namespace MbD {
    class ASMTRotationalMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void readMotionJoint(std::vector<std::string>& lines);
        void readRotationZ(std::vector<std::string>& lines);
        void initMarkers() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        std::shared_ptr<Joint> mbdClassNew() override;

        std::string motionJoint, rotationZ;
    };
}

