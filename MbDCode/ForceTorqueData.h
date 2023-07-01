#pragma once

#include "StateData.h"

namespace MbD {
	class ForceTorqueData : public StateData
	{
		//aFIO aTIO
	public:
		std::ostream& printOn(std::ostream& s) const override;

		FColDsptr aFIO, aTIO;
	};
}

