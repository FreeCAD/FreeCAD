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
        static std::shared_ptr<ASMTRevRevJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;

    };
}

