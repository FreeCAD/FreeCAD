/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "ConstraintIJ.h"
#include "AngleZIecJec.h"

namespace MbD {
    class AngleZConstraintIJ : public ConstraintIJ
    {
        //thezIeJe 
    public:
        AngleZConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj);

        static std::shared_ptr<AngleZConstraintIJ> With(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void initthezIeJe();
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;
        ConstraintType type() override;

        std::shared_ptr<AngleZIecJec> thezIeJe;
    };
}
