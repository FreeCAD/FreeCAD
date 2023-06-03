#include <limits>

#include "NewtonRaphson.h"
#include "SystemSolver.h"
#include "MaximumIterationError.h"

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

	iterNo = 0;
	this->fillY();
	this->calcyNorm();
	yNorms->push_back(yNorm);

	while (true) {
		this->incrementIterNo();
		this->fillPyPx();
		this->solveEquations();
		this->calcDXNormImproveRootCalcYNorm();
		if (this->isConverged()) break;
	}
}

void MbD::NewtonRaphson::incrementIterNo()
{
	iterNo++;
	if (iterNo > iterMax) {
		this->reportStats();
		throw MaximumIterationError("");
	}
}

bool MbD::NewtonRaphson::isConverged()
{
	return this->isConvergedToNumericalLimit();
}

void MbD::NewtonRaphson::askSystemToUpdate()
{
}

bool MbD::NewtonRaphson::isConvergedToNumericalLimit()
{
	//"worthIterating is less stringent with IterNo."
	//"nDivergenceMax is the number of small divergences allowed."

	auto tooLargeTol = 1.0e-2;
	constexpr auto smallEnoughTol = std::numeric_limits<double>::epsilon();
	auto nDecade = log(tooLargeTol / smallEnoughTol);
	auto nDivergenceMax = 3;
	auto dxNormIterNo = dxNorms->at(iterNo);
	if (iterNo > 0) {
		auto dxNormIterNoOld = dxNorms->at(iterNo);
		auto farTooLargeError = dxNormIterNo > tooLargeTol;
		auto worthIterating = dxNormIterNo > (smallEnoughTol * pow(10.0, (iterNo / iterMax) * nDecade));
		bool stillConverging;
		if (dxNormIterNo < (0.5 * dxNormIterNoOld)) {
			stillConverging = true;
		}
		else {
			if (!farTooLargeError) nDivergence++;
			stillConverging = nDivergence < nDivergenceMax;
		}
		return !(farTooLargeError || (worthIterating && stillConverging));
	}
	else {
		auto worthIterating = dxNormIterNo > smallEnoughTol;
		return !worthIterating;
	}
}

void MbD::NewtonRaphson::calcDXNormImproveRootCalcYNorm()
{
	this->calcdxNorm();
	dxNorms->push_back(dxNorm);
	this->updatexold();
	this->xEqualxoldPlusdx();
	this->passRootToSystem();
	this->askSystemToUpdate();
	this->fillY();
	this->calcyNorm();
	yNorms->push_back(yNorm);
	yNormOld = yNorm;
}
