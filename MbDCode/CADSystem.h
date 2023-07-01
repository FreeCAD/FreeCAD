#pragma once
#include<memory>

#include "System.h"

//using namespace MbD;

//namespace CAD {
namespace MbD {
	//class System;

	class CADSystem
	{
		//
	public:
		CADSystem() {
			mbdSystem->externalSystem = this;
		}

		void outputFor(AnalysisType type);
		void logString(std::string& str);
		void logString(double value);
		void runOndselPiston();
		void runPiston();
		void postMbDrun();

		std::shared_ptr<System> mbdSystem = std::make_shared<System>();

	};
}

