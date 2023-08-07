#pragma once

#include "FunctionFromData.h"

namespace MbD {
	class AnyGeneralSpline : public FunctionFromData
	{
		//derivs degree index delta 
	public:
		AnyGeneralSpline() = default;
		AnyGeneralSpline(Symsptr arg);
	};
}
