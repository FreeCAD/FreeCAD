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
			mbdSystem->initialize();
			mbdSystem->externalSystem->cadSystem = this;
		}

		void outputFor(AnalysisType type);
		void logString(std::string& str);
		void logString(double value);
		void runOndselPiston();
		void runPiston();
		void preMbDrun(std::shared_ptr<System> mbdSys);
		void postMbDrun();
		void updateFromMbD();

		std::shared_ptr<System> mbdSystem = std::make_shared<System>();

	};
}

