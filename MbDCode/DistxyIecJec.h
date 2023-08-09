/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "KinematicIeJe.h"
#include "DispCompIecJecIe.h"

namespace MbD {
	class DistxyIecJec : public KinematicIeJe
	{
		//distxy xIeJeIe yIeJeIe
	public:
        DistxyIecJec();
        DistxyIecJec(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        virtual void init_xyIeJeIe();
        void postInput() override;
        void postPosICIteration() override;
        void preAccIC() override;
        void prePosIC() override;
        void preVelIC() override;
        void simUpdateAll() override;
        double value() override;

        double distxy;
        std::shared_ptr<DispCompIecJecIe> xIeJeIe, yIeJeIe;
    
    };
}

