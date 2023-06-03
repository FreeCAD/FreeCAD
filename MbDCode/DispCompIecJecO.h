#pragma once

#include "KinematicIeJe.h"

namespace MbD {
	class DispCompIecJecO : public KinematicIeJe
	{
		//axis riIeJeO 
	public:
		DispCompIecJecO();
		DispCompIecJecO(EndFrmcptr frmi, EndFrmcptr frmj, size_t axis);
		void calcPostDynCorrectorIteration() override;

		size_t axis = -1;
		double riIeJeO;
	};
}

