/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AtPointJoint.h"

namespace MbD {
    class RevoluteJoint : public AtPointJoint
    {
        //
    public:
        RevoluteJoint();
        RevoluteJoint(const std::string& str);
        void initializeGlobally() override;
    };
}

