#pragma once

#include "PrescribedMotion.h"
#include "Symbolic.h"
#include "EulerAngles.h"

namespace MbD {

	class FullMotion : public PrescribedMotion
	{
		//frIJI fangIJJ 
	public:
		FullMotion();
		FullMotion(const char* str);
		void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ) override;
		void initializeGlobally() override;
		void initMotions() override;

		std::shared_ptr<FullColumn<Symsptr>> frIJI;
		std::shared_ptr<EulerAngles<Symsptr>> fangIJJ;

	};
}

