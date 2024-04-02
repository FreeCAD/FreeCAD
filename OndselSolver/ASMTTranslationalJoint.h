/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ASMTInLineJoint.h"

namespace MbD {
    class ASMTTranslationalJoint : public ASMTInLineJoint
    {
        //
    public:
        static std::shared_ptr<ASMTTranslationalJoint> With();
        std::shared_ptr<ItemIJ> mbdClassNew() override;
        void storeOnTimeSeries(std::ofstream& os) override;


    };
}

