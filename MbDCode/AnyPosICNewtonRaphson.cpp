#include "AnyPosICNewtonRaphson.h"
#include "SystemSolver.h"
#include "Item.h"
#include <iostream>

using namespace MbD;

void AnyPosICNewtonRaphson::initialize()
{
	NewtonRaphson::initialize();
	nSingularMatrixError = 0;
}

void AnyPosICNewtonRaphson::initializeGlobally()
{
	SystemNewtonRaphson::initializeGlobally();
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {
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
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {
		item->fillPosICError(y);
		//std::cout << *y << std::endl;
		});
	//std::cout << *y << std::endl;
}

void AnyPosICNewtonRaphson::fillPyPx()
{
	pypx->zeroSelf();
	pypx->atijminusDiagonalMatrix(0, 0, qsuWeights);
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {
		item->fillPosICJacob(pypx);
		//std::cout << *(pypx->at(3)) << std::endl;
		});
	//std::cout << *pypx << std::endl;
}

void AnyPosICNewtonRaphson::passRootToSystem()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->setqsulam(x); });
}
