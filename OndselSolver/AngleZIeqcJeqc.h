/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AngleZIeqcJec.h"

namespace MbD {
    class AngleZIeqcJeqc : public AngleZIeqcJec
    {
        //pthezpEJ ppthezpEIpEJ ppthezpEJpEJ 
    public:
        AngleZIeqcJeqc();
        AngleZIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj);
        static std::shared_ptr<AngleZIeqcJeqc> With(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        void init_aAijIeJe() override;
        void initialize() override;
        FMatDsptr ppvaluepEIpEJ() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;

        FRowDsptr pthezpEJ;
        FMatDsptr ppthezpEIpEJ, ppthezpEJpEJ;
    };
}

