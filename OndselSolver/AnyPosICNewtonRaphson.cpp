/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <iostream>
 
#include "AnyPosICNewtonRaphson.h"
#include "SystemSolver.h"
#include "Item.h"
#include "Part.h"
#include "Constraint.h"

using namespace MbD;

void AnyPosICNewtonRaphson::initialize()
{
	NewtonRaphson::initialize();
	nSingularMatrixError = 0;
}

void AnyPosICNewtonRaphson::initializeGlobally()
{
	SystemNewtonRaphson::initializeGlobally();
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) {
		item->fillqsu(qsuOld);
		item->fillqsuWeights(qsuWeights);
		item->fillqsulam(x);
		});
}

void AnyPosICNewtonRaphson::createVectorsAndMatrices()
{
	qsuOld = std::make_shared<FullColumn<double>>(nqsu);
	qsuWeights = std::make_shared<DiagonalMatrix<double>>(nqsu);
	SystemNewtonRaphson::createVectorsAndMatrices();
}

void AnyPosICNewtonRaphson::fillY()
{
	auto newMinusOld = qsuOld->negated();
	newMinusOld->equalSelfPlusFullColumnAt(x, 0);
	y->zeroSelf();
	y->atiminusFullColumn(0, (qsuWeights->timesFullColumn(newMinusOld)));
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) {
		item->fillPosICError(y);
		//std::cout << item->name << *y << std::endl;
		//noop();
		});
	//std::cout << "Final" << *y << std::endl;
}

void AnyPosICNewtonRaphson::fillPyPx()
{
	pypx->zeroSelf();
	pypx->atijminusDiagonalMatrix(0, 0, qsuWeights);
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) {
		item->fillPosICJacob(pypx);
		//std::cout << *(pypx->at(3)) << std::endl;
		});
	//std::cout << *pypx << std::endl;
}

void AnyPosICNewtonRaphson::passRootToSystem()
{
	system->partsJointsMotionsLimitsDo([&](std::shared_ptr<Item> item) { item->setqsulam(x); });
}

void MbD::AnyPosICNewtonRaphson::assignEquationNumbers()
{
	auto parts = system->parts();
	//auto contactEndFrames = system->contactEndFrames();
	//auto uHolders = system->uHolders();
	auto constraints = system->allConstraints();
	size_t eqnNo = 0;
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

bool MbD::AnyPosICNewtonRaphson::isConverged()
{
	return dxNorms->at(iterNo) < dxTol || isConvergedToNumericalLimit();
}
