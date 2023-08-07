#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class ZTranslation : public PrescribedMotion
	{
		//
	public:
		ZTranslation();
		ZTranslation(const char* str);
		void initializeGlobally() override;

	};
}

