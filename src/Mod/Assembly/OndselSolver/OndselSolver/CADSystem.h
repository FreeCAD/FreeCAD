/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>

#include "ExternalSystem.h"
#include "System.h"

namespace MbD {

	class CADSystem
	{
		//
	public:
		CADSystem() {
			mbdSystem->initialize();
			mbdSystem->externalSystem->cadSystem = this;
		}

		void outputFor(AnalysisType type);
		void logString(const std::string& str);
		void logString(double value);
		void runOndselSinglePendulum();
		void runOndselDoublePendulum();
		void runOndselPiston();
		void runPiston();
		void preMbDrun(std::shared_ptr<System> mbdSys);
		void postMbDrun();
		void updateFromMbD();

		std::shared_ptr<System> mbdSystem = std::make_shared<System>();

	};
}

