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
        std::shared_ptr<Joint> mbdClassNew() override;

    };
}

