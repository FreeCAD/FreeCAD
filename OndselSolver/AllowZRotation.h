/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class AllowZRotation : public PrescribedMotion
	{
		//
	public:
		AllowZRotation();
		AllowZRotation(const std::string& str);
		static std::shared_ptr<AllowZRotation> With();
		void initializeGlobally() override;
		void postPosIC() override;
	};
}

