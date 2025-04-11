/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "PrescribedMotion.h"
#include "Symbolic.h"
#include "EulerAngles.h"

namespace MbD {

	class FullMotion : public PrescribedMotion
	{
		//frIJI fangIJJ 
	public:
		FullMotion();
		FullMotion(const std::string& str);
		void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ) override;
		void initializeGlobally() override;
		void initMotions() override;

		std::shared_ptr<FullColumn<Symsptr>> frIJI;
		std::shared_ptr<EulerAngles<Symsptr>> fangIJJ;

	};
}

