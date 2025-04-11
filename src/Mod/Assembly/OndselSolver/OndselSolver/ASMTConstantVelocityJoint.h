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
    class ASMTConstantVelocityJoint : public ASMTAtPointJoint
    {
        //
    public:
        static std::shared_ptr<ASMTConstantVelocityJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;

    };
}

