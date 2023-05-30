#include <limits>

#include "NewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void NewtonRaphson::initialize()
{
	dxNorms = std::make_shared<std::vector<double>>();
	dxTol = 4 * std::numeric_limits<double>::epsilon();
	yNorms = std::make_shared<std::vector<double>>();
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

void MbD::NewtonRaphson::iterate()
{
	//"
	//	Do not skip matrix solution even when yNorm is very small.
	//	This avoids unexpected behaviors when convergence is still
	//	possible.

	//	Do not skip redundant constraint removal even when yNorm is
	//	zero.
	//	"

	//	iterNo : = 0.
	//	self fillY.
	//	self calcyNorm.
	//	yNorms add : yNorm.

	//	[self incrementIterNo.
	//	self fillPyPx.
	//	self solveEquations.
	//	self calcDXNormImproveRootCalcYNorm.
	//	self isConverged] whileFalse
}
