#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class FullMotion : public PrescribedMotion
	{
		//frIJI fangIJJ 
	public:
		FullMotion();
		FullMotion(const char* str);
		void initializeGlobally() override;
		void initMotions() override;

	};
}

