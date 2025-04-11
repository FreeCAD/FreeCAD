/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "AtPointConstraintIJ.h"

namespace MbD {
	class AtPointConstraintIqcJc : public AtPointConstraintIJ
	{
		//pGpEI ppGpEIpEI iqXIminusOnePlusAxis iqEI 
	public:
		AtPointConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi);

		void addToJointForceI(FColDsptr col) override;
		void addToJointTorqueI(FColDsptr col) override;
		void calcPostDynCorrectorIteration() override;
		void fillAccICIterError(FColDsptr col) override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void initializeGlobally() override;
		void initriIeJeO() override;
		void useEquationNumbers() override;

		FRowDsptr pGpEI;
		FMatDsptr ppGpEIpEI;
		size_t iqXIminusOnePlusAxis = SIZE_MAX;
		size_t iqEI = SIZE_MAX;
	};
}

