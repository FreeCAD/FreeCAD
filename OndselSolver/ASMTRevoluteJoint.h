/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTAtPointJoint.h"

namespace MbD {
    class ASMTRevoluteJoint : public ASMTAtPointJoint
    {
        //
    public:
        static std::shared_ptr<ASMTRevoluteJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        //void storeOnTimeSeries(std::ofstream& os) override;

    };
}

