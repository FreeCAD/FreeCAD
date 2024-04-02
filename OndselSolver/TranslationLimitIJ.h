/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "LimitIJ.h"

namespace MbD {
	class TranslationLimitIJ : public LimitIJ
	{
		//
	public:
		static std::shared_ptr<TranslationLimitIJ> With();
		void initializeGlobally() override;


	};
}
