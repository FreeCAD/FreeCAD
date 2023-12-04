/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ASMTInPlaneJoint.h"

namespace MbD {
    class ASMTPlanarJoint : public ASMTInPlaneJoint
    {
        //
    public:
        std::shared_ptr<Joint> mbdClassNew() override;

    };
}

