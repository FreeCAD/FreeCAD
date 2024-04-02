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
	for (size_t i = 0; i < qsuWeights->size(); i++)
	{
		qsuWeights->at(i) = 1.0e3;	//minimum weight
	}
	for (auto& part : *dragParts) {
		auto iqX = part->iqX();
		for (size_t i = 0; i < 3; i++)
		{
			qsuWeights->at((size_t)iqX + i) = 1.0e6;	//maximum weight
		}
	}
}

void MbD::PosICDragNewtonRaphson::setdragParts(std::shared_ptr<std::vector<std::shared_ptr<Part>>> _dragParts)
{
	dragParts = _dragParts;
}
