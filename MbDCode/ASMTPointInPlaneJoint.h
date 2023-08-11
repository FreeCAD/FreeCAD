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
    class ASMTPointInPlaneJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;
        void parseASMT(std::vector<std::string>& lines) override;
        void readOffset(std::vector<std::string>& lines);
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;

        double offset;
    };
}

