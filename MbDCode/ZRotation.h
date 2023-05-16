#pragma once
#include "PrescribedMotion.h"

namespace MbD {
	class ZRotation : public PrescribedMotion
	{
		//
	public:
		ZRotation();
		ZRotation(const char* str);
		void initialize();
		void initializeGlobally() override;
		void initMotions();
		void addConstraint(std::shared_ptr<Constraint> con);
	};
}

