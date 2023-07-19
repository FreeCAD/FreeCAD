#pragma once

#include <memory>
#include "enum.h"
#include <string>

//#include "CADSystem.h"
//#include "ASMTAssembly.h"

namespace MbD {
	class CADSystem;
	class ASMTAssembly;

	class ExternalSystem
	{
		//
	public:
		void preMbDrun();
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

