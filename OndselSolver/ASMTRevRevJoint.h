/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ASMTCompoundJoint.h"

namespace MbD {
    class ASMTRevRevJoint : public ASMTCompoundJoint
    {
        //
    public:
        std::shared_ptr<Joint> mbdClassNew() override;

    };
}

