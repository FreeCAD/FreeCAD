#include <algorithm>
#include <memory>
#include <typeinfo>

#include "ConstraintSet.h"
#include "Constraint.h"

using namespace MbD;

MbD::ConstraintSet::ConstraintSet()
{
}

MbD::ConstraintSet::ConstraintSet(const std::string& str) : ItemIJ(str)
{
}

void MbD::ConstraintSet::constraintsDo(const std::function <void(std::shared_ptr<Constraint>)>& f)
{
	std::for_each(constraints->begin(), constraints->end(), f);
}

void MbD::ConstraintSet::initialize()
{
	constraints = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
}

void MbD::ConstraintSet::initializeGlobally()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->initializeGlobally(); });
}

void MbD::ConstraintSet::initializeLocally()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->initializeLocally(); });
}

void MbD::ConstraintSet::addConstraint(std::shared_ptr<Constraint> con)
{
	con->owner = this;
	constraints->push_back(con);
}
void MbD::ConstraintSet::postInput()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postInput(); });
}

void MbD::ConstraintSet::prePosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosIC(); });
}

void MbD::ConstraintSet::prePosKine()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->prePosKine(); });
}

void MbD::ConstraintSet::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillEssenConstraints(con, essenConstraints); });
}

void MbD::ConstraintSet::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillDispConstraints(con, dispConstraints); });
}

void MbD::ConstraintSet::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPerpenConstraints(con, perpenConstraints); });
}

void MbD::ConstraintSet::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillConstraints(con, allConstraints); });
}

void MbD::ConstraintSet::fillqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsulam(col); });
}

void MbD::ConstraintSet::fillqsudot(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillqsudot(col); });
}

void MbD::ConstraintSet::fillqsudotWeights(DiagMatDsptr)
{
}

void MbD::ConstraintSet::useEquationNumbers()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->useEquationNumbers(); });
}

void MbD::ConstraintSet::setqsulam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsulam(col); });
}

void MbD::ConstraintSet::setqsudotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsudotlam(col); });
}

void MbD::ConstraintSet::postPosICIteration()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosICIteration(); });
}

void MbD::ConstraintSet::fillPosICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICError(col); });
}

void MbD::ConstraintSet::fillPosICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICJacob(mat); });
}

void MbD::ConstraintSet::postPosIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postPosIC(); });
}

void MbD::ConstraintSet::preDyn()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preDyn(); });
}

void MbD::ConstraintSet::fillPosKineError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillPosKineError(col); });
}

void MbD::ConstraintSet::fillPosKineJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillPosKineJacob(mat); });
}

void MbD::ConstraintSet::fillqsuddotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillqsuddotlam(col); });
}

void MbD::ConstraintSet::preVelIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preVelIC(); });
}

void MbD::ConstraintSet::fillVelICError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillVelICError(col); });
}

void MbD::ConstraintSet::fillVelICJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> constraint) { constraint->fillVelICJacob(mat); });
}

void MbD::ConstraintSet::preAccIC()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->preAccIC(); });
}

void MbD::ConstraintSet::fillAccICIterError(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterError(col); });
}

void MbD::ConstraintSet::fillAccICIterJacob(SpMatDsptr mat)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->fillAccICIterJacob(mat); });
}

void MbD::ConstraintSet::setqsuddotlam(FColDsptr col)
{
	constraintsDo([&](std::shared_ptr<Constraint> con) { con->setqsuddotlam(col); });
}

void MbD::ConstraintSet::postDynStep()
{
	constraintsDo([](std::shared_ptr<Constraint> constraint) { constraint->postDynStep(); });
}
