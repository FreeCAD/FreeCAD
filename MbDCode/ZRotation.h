#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class ZRotation : public PrescribedMotion
	{
		//
	public:
		ZRotation();
		ZRotation(const char* str);
		void initializeGlobally() override;
		void initMotions() override;
	};
}

