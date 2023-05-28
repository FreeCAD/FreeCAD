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

		int axis;
		double riIeJeO;
	};
}

