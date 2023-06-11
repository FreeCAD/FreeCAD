#pragma once

#include "KinematicIeJe.h"

namespace MbD {
	class DispCompIecJecO : public KinematicIeJe
	{
		//axis riIeJeO 
	public:
		DispCompIecJecO();
		DispCompIecJecO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);
		void calcPostDynCorrectorIteration() override;
		FRowDsptr pvaluepXJ() override;
		FRowDsptr pvaluepEJ() override;
		FMatDsptr ppvaluepXJpEK() override;
		FMatDsptr ppvaluepEJpEK() override;
		FMatDsptr ppvaluepEJpEJ() override;

		int axis = -1;
		double riIeJeO;
	};
}

