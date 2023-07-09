#pragma once

#include "DispCompIeqcJeqcIe.h"

namespace MbD {
	class DispCompIeqctJeqcIe : public DispCompIeqcJeqcIe
	{
		//priIeJeIept ppriIeJeIepXIpt ppriIeJeIepEIpt ppriIeJeIepXJpt ppriIeJeIepEJpt ppriIeJeIeptpt 
	public:
		DispCompIeqctJeqcIe();
		DispCompIeqctJeqcIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

		double priIeJeIept, ppriIeJeIeptpt;
		FRowDsptr ppriIeJeIepXIpt, ppriIeJeIepEIpt, ppriIeJeIepXJpt, ppriIeJeIepEJpt;
	};
}

