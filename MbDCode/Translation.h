#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class Translation : public PrescribedMotion
	{
		//
	public:
		Translation();
		Translation(const char* str);
		void initializeGlobally() override;
		void initMotions() override;

	};
}

