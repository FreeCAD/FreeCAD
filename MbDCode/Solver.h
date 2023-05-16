#pragma once
namespace MbD {
	class Solver
	{
	public:
		virtual void initializeLocally() = 0;
		virtual void initializeGlobally() = 0;
	};
}

