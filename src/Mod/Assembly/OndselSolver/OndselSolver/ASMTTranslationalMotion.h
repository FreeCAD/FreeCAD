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
    class ASMTTranslationalMotion : public ASMTMotion
    {
        //
    public:
        void parseASMT(std::vector<std::string>& lines) override;
        void initMarkers() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        void readMotionJoint(std::vector<std::string>& lines);
        void setTranslationZ(std::string tranZ);
        void readTranslationZ(std::vector<std::string>& lines);
        void storeOnLevel(std::ofstream& os, size_t level) override;
        void storeOnTimeSeries(std::ofstream& os) override;

        std::string motionJoint, translationZ;

    };
}

