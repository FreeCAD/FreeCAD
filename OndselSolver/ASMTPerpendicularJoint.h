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
    class ASMTPerpendicularJoint : public ASMTJoint
    {
        //
    public:
        static std::shared_ptr<ASMTPerpendicularJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;

    };
}
