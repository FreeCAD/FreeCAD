/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ConstraintIJ.h"

namespace MbD {
    class DispCompIecJecO;

    class AtPointConstraintIJ : public ConstraintIJ
    {
        //axis riIeJeO 
    public:
        AtPointConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        virtual void initriIeJeO();
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        ConstraintType type() override;
        

        size_t axis;
        std::shared_ptr<DispCompIecJecO> riIeJeO;
    };
}

