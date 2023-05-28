#pragma once
#include "PrescribedMotion.h"

namespace MbD {
	class ZRotation : public PrescribedMotion
	{
		//
	public:
		static std::shared_ptr<ZRotation> Create(const char* name);
		ZRotation();
		ZRotation(const char* str);
		void initializeGlobally() override;
		void initMotions();
		void addConstraint(std::shared_ptr<Constraint> con);
	};
}

