/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "InLineJoint.h"

namespace MbD {
    class TranslationalJoint : public InLineJoint
    {
        //
    public:
        TranslationalJoint();
        TranslationalJoint(const std::string& str);
        void initializeGlobally() override;

    };
}

