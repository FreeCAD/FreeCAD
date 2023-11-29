/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "ConstraintIJ.h"
#include "DispCompIecJecIe.h"
#include "AngleZIecJec.h"

namespace MbD {
    class ScrewConstraintIJ : public ConstraintIJ
    {
        //zIeJeIe thezIeJe pitch 
    public:
        ScrewConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj);

        static std::shared_ptr<ScrewConstraintIJ> With(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        virtual void init_zthez();
        virtual void initzIeJeIe();
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

        std::shared_ptr<DispCompIecJecIe> zIeJeIe;
        std::shared_ptr<AngleZIecJec> thezIeJe;
        double pitch;


    };
}

