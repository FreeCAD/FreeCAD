/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ForceTorqueItem.h"

namespace MbD {
    class ConstantGravity : public ForceTorqueItem
    {
        //
    public:
        void fillAccICIterError(FColDsptr col) override;

        FColDsptr gXYZ;
    };
}

