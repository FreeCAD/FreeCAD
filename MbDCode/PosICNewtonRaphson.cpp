#include <assert.h>
#include <exception>

#include "PosICNewtonRaphson.h"
#include "SingularMatrixError.h"
#include "SystemSolver.h"

void MbD::PosICNewtonRaphson::run()
{
	while (true) {
		try {
			VectorNewtonRaphson::run();
			break;
		}
		catch (SingularMatrixError ex) {
			auto redundantEqnNos = ex.getRedundantEqnNos();
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->removeRedundantConstraints(redundantEqnNos); });
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->constraintsReport(); });
			system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->setqsu(qsuOld); });
		}
	}
}
