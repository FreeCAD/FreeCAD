#pragma once

#include "AtPointConstraintIJ.h"

namespace MbD {
	class AtPointConstraintIqcJc : public AtPointConstraintIJ
	{
		//pGpEI ppGpEIpEI iqXIminusOnePlusAxis iqEI 
	public:
		AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
		void initializeGlobally() override;
		void initriIeJeO() override;
		void calcPostDynCorrectorIteration() override;
		void useEquationNumbers() override;
		void fillPosICError(FColDsptr col) override;
		void fillPosICJacob(SpMatDsptr mat) override;
		void fillPosKineJacob(SpMatDsptr mat) override;
		void fillVelICJacob(SpMatDsptr mat) override;
		void fillAccICIterError(FColDsptr col) override;

		FRowDsptr pGpEI;
		FMatDsptr ppGpEIpEI;
		int iqXIminusOnePlusAxis = -1;
		int iqEI = -1;
	};
}

