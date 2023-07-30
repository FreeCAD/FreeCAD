#pragma once

#include "Function.h"

namespace MbD {
	class FunctionX : public Function
	{
		//
	public:
		FunctionX() = default;
		FunctionX(Symsptr arg);
		Symsptr differentiateWRT(Symsptr var) override;
		virtual Symsptr differentiateWRTx();

		Symsptr xx;



	};
}

