/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "PosICDragNewtonRaphson.h"
#include "SystemSolver.h"
#include "Part.h"
#include "Constraint.h"

using namespace MbD;

std::shared_ptr<PosICDragNewtonRaphson> MbD::PosICDragNewtonRaphson::With()
{
	auto newtonRaphson = std::make_shared<PosICDragNewtonRaphson>();
	newtonRaphson->initialize();
	return newtonRaphson;
}

void MbD::PosICDragNewtonRaphson::initializeGlobally()
{
	AnyPosICNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxPosKine;
	dxTol = system->errorTolPosKine;
	for (int i = 0; i < qsuWeights->size(); i++)
	{
		qsuWeights->at(i) = 1.0e3;	//minimum weight
	}
	for (auto& part : *dragParts) {
		auto iqX = part->iqX();
		for (int i = 0; i < 3; i++)
		{
			qsuWeights->at((size_t)iqX + i) = 1.0e6;	//maximum weight
		}
	}
}

void MbD::PosICDragNewtonRaphson::assignEquationNumbers()
{
	auto parts = system->parts();
	//auto contactEndFrames = system->contactEndFrames();
	//auto uHolders = system->uHolders();
	auto constraints = system->allConstraints();
	int eqnNo = 0;
	for (auto& part : *parts) {
		part->iqX(eqnNo);
		eqnNo = eqnNo + 3;
		part->iqE(eqnNo);
		eqnNo = eqnNo + 4;
	}
	//for (auto& endFrm : *contactEndFrames) {
	//	endFrm->is(eqnNo);
	//	eqnNo = eqnNo + endFrm->sSize();
	//}
	//for (auto& uHolder : *uHolders) {
	//	uHolder->iu(eqnNo);
	//	eqnNo += 1;
	//}
	auto nEqns = eqnNo;	//C++ uses index 0.
	nqsu = nEqns;
	for (auto& con : *constraints) {
		con->iG = eqnNo;
		eqnNo += 1;
	}
	//auto lastEqnNo = eqnNo - 1;
	nEqns = eqnNo;	//C++ uses index 0.
	n = nEqns;
}

bool MbD::PosICDragNewtonRaphson::isConverged()
{
	return dxNorms->at(iterNo) < dxTol || isConvergedToNumericalLimit();
}

void MbD::PosICDragNewtonRaphson::setdragParts(std::shared_ptr<std::vector<std::shared_ptr<Part>>> _dragParts)
{
	dragParts = _dragParts;
}
