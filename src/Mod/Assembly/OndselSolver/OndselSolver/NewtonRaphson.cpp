/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <iostream>
#include <limits>
#include <cassert>
#include <cstdint>

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
	iterNo = SIZE_MAX;
	nDivergence = SIZE_MAX;
	nBackTracking = SIZE_MAX;
	dxNorms->clear();
	yNorms->clear();
	yNormOld = std::numeric_limits<double>::max();
}

void NewtonRaphson::run()
{
	assert(false);
	//self preRun.
	//self initializeLocally.
	//self initializeGlobally.
	//self iterate.
	//self finalize.
	//self reportStats.
	//self postRun.
}

void NewtonRaphson::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}

void NewtonRaphson::iterate()
{
	//"
	//	Do not skip matrix solution even when yNorm is very small.
	//	This avoids unexpected behaviors when convergence is still
	//	possible.

	//	Do not skip redundant constraint removal even when yNorm is
	//	zero.
	//	"

	iterNo = SIZE_MAX;
	this->fillY();
	this->calcyNorm();
	yNorms->push_back(yNorm);

	while (true) {
		this->incrementIterNo();
		this->fillPyPx();
		this->solveEquations();
		this->calcDXNormImproveRootCalcYNorm();
		if (this->isConverged()) {
			//std::cout << "iterNo = " << iterNo << std::endl;
			break;
		}
	}
}

void NewtonRaphson::incrementIterNo()
{
	iterNo++;
	if (iterNo > iterMax) {
		this->reportStats();
		throw MaximumIterationError("");
	}
}

bool NewtonRaphson::isConverged()
{
	return this->isConvergedToNumericalLimit();
}

void NewtonRaphson::askSystemToUpdate()
{
	assert(false);
}

bool NewtonRaphson::isConvergedToNumericalLimit()
{
	//"worthIterating is less stringent with IterNo."
	//"nDivergenceMax is the number of small divergences allowed."

	auto tooLargeTol = 1.0e-2;
	constexpr auto smallEnoughTol = std::numeric_limits<double>::epsilon();
	auto nDecade = log10(tooLargeTol / smallEnoughTol);
	size_t nDivergenceMax = 3;
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
			if (!farTooLargeError) {
				nDivergence++;
			}
			stillConverging = nDivergence < nDivergenceMax;
		}
		return !(farTooLargeError || (worthIterating && stillConverging));
	}
	else {
		auto worthIterating = dxNormIterNo > smallEnoughTol;
		return !worthIterating;
	}
}

void NewtonRaphson::calcDXNormImproveRootCalcYNorm()
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

void NewtonRaphson::postRun()
{
	system->postNewtonRaphson();
}
