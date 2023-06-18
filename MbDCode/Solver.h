#pragma once

#include <string>

namespace MbD {
	class Solver
	{
		//statistics
	public:
		virtual void initialize();
		virtual void initializeLocally();
		virtual void initializeGlobally();
		virtual void assignEquationNumbers();
		virtual void run();
		virtual void preRun();
		virtual void finalize();
		virtual void reportStats();
		virtual void postRun();
		virtual void logString(std::string& str);
		virtual void setSystem(Solver* sys) = 0;

	};
}

