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
    class ASMTCylSphJoint : public ASMTCompoundJoint
    {
        //
    public:
        static std::shared_ptr<ASMTCylSphJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;

    };
}
