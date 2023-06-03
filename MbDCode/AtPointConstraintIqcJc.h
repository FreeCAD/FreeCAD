#pragma once

#include "AtPointConstraintIJ.h"

namespace MbD {
	class AtPointConstraintIqcJc : public AtPointConstraintIJ
	{
		//pGpEI ppGpEIpEI iqXIminusOnePlusAxis iqEI 
	public:
		AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi);
		void initializeGlobally() override;
		void initriIeJeO() override;
		void calcPostDynCorrectorIteration() override;
		void useEquationNumbers() override;

		FRowDsptr pGpEI;
		FMatDsptr ppGpEIpEI;
		size_t iqXIminusOnePlusAxis = -1;
		size_t iqEI = -1;
	};
}

