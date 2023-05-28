#include <limits>

#include "NewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void NewtonRaphson::initialize()
{
	dxNorms = std::shared_ptr<std::vector<double>>();
	dxTol = 4 * std::numeric_limits<double>::epsilon();
	yNorms = std::shared_ptr<std::vector<double>>();
	yNormTol = 1.0e-30;
	iterMax = 100;
	twoAlp = 2.0e-4;
}

void NewtonRaphson::initializeLocally()
{
	iterNo = 0;
	nDivergence = 0;
	nBackTracking = 0;
	dxNorms->clear();
	yNorms->clear();
	yNormOld = std::numeric_limits<double>::max();
}

void MbD::NewtonRaphson::run()
{
		//self preRun.
		//self initializeLocally.
		//self initializeGlobally.
		//self iterate.
		//self finalize.
		//self reportStats.
		//self postRun.
}

void MbD::NewtonRaphson::setSystem(SystemSolver* sys)
{
	system = sys;
}
