#include "AnyPosICNewtonRaphson.h"
#include "SystemSolver.h"
#include "Item.h"

using namespace MbD;

void MbD::AnyPosICNewtonRaphson::initialize()
{
	NewtonRaphson::initialize();
	nSingularMatrixError = 0;
}

void MbD::AnyPosICNewtonRaphson::initializeGlobally()
{
	SystemNewtonRaphson::initializeGlobally();
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {
		item->fillqsu(qsuOld);
		item->fillqsuWeights(qsuWeights);
		item->fillqsulam(x);
		});
}

void MbD::AnyPosICNewtonRaphson::createVectorsAndMatrices()
{
	qsuOld = std::make_shared<FullColumn<double>>(nqsu);
	qsuWeights = std::make_shared<DiagonalMatrix<double>>(nqsu);
	SystemNewtonRaphson::createVectorsAndMatrices();
}

void MbD::AnyPosICNewtonRaphson::fillY()
{
	auto newMinusOld = qsuOld->negated();
	newMinusOld->equalSelfPlusFullColumnAt(x, 0);
	y->zeroSelf();
	y->atiminusFullColumn(0, (qsuWeights->timesFullColumn(newMinusOld)));
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->fillPosICError(y); });
}

void MbD::AnyPosICNewtonRaphson::fillPyPx()
{
	pypx->zeroSelf();
	pypx->atijminusDiagonalMatrix(0, 0, qsuWeights);
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {item->fillPosICJacob(pypx); });
}

void MbD::AnyPosICNewtonRaphson::passRootToSystem()
{
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) {item->setqsulam(x); });
}
