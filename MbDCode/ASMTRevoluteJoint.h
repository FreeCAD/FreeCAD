/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTJoint.h"
#include "RevoluteJoint.h"

namespace MbD {
    class ASMTRevoluteJoint : public ASMTJoint
    {
        //
    public:
        virtual std::shared_ptr<Joint> mbdClassNew() override;

    };
}

