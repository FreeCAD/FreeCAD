/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "KinematicIeJe.h"
#include "DirectionCosineIeqcJec.h"

namespace MbD {
    class AngleZIecJec : public KinematicIeJe
    {
        //thez aA00IeJe aA10IeJe cosOverSSq sinOverSSq twoCosSinOverSSqSq dSqOverSSqSq 
    public:
        AngleZIecJec();
        AngleZIecJec(EndFrmsptr frmi, EndFrmsptr frmj);
        
        void calcPostDynCorrectorIteration() override;
        virtual void init_aAijIeJe() = 0;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;
        double value() override;

        double thez = std::numeric_limits<double>::min();
        double cosOverSSq = 0.0, sinOverSSq = 0.0, twoCosSinOverSSqSq = 0.0, dSqOverSSqSq = 0.0;
        std::shared_ptr<DirectionCosineIeqcJec> aA00IeJe, aA10IeJe;
    };
}

