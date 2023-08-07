#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class Orientation : public PrescribedMotion
	{
		//
	public:
		Orientation();
		Orientation(const char* str);
		void initializeGlobally() override;

	};
}

