#pragma once
namespace MbD {
	class Solver
	{
	public:
		virtual void initialize();
		virtual void initializeLocally();
		virtual void initializeGlobally();
		virtual void run();
	};
}

