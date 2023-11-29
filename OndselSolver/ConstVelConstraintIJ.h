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
    class DirectionCosineIecJec;

    class ConstVelConstraintIJ : public ConstraintIJ
    {
        //aA01IeJe aA10IeJe 
    public:
        ConstVelConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj);

        static std::shared_ptr<ConstVelConstraintIJ> With(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void initA01IeJe();
        virtual void initA10IeJe();
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;

        std::shared_ptr<DirectionCosineIecJec> aA01IeJe, aA10IeJe;
    };
}

