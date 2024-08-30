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
#include "Part.h"

namespace Assembly
{
	class AssemblyObject;
}

namespace MbD {
	class CADSystem;
	class ASMTAssembly;
    class System;

	class ExternalSystem
	{
		//
	public:
		void preMbDrun(std::shared_ptr<System> mbdSys);
		void preMbDrunDragStep(std::shared_ptr<System> mbdSys, std::shared_ptr<std::vector<std::shared_ptr<Part>>> dragParts);
		void updateFromMbD();
		void outputFor(AnalysisType type);
		void logString(const std::string& str);
		void logString(double value);
		void runOndselPiston();
		void runPiston();
		void postMbDrun();


		CADSystem* cadSystem;
        ASMTAssembly* asmtAssembly;
        Assembly::AssemblyObject* freecadAssemblyObject;

	};
}

