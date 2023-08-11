/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>
#include "enum.h"
#include <string>

//#include "CADSystem.h"
//#include "ASMTAssembly.h"

namespace MbD {
	class CADSystem;
	class ASMTAssembly;
	class System;

	class ExternalSystem
	{
		//
	public:
		void preMbDrun(std::shared_ptr<System> mbdSys);
		void outputFor(AnalysisType type);
		void logString(std::string& str);
		void logString(double value);
		void runOndselPiston();
		void runPiston();
		void postMbDrun();


		CADSystem* cadSystem;
		ASMTAssembly* asmtAssembly;

	};
}

