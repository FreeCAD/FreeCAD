#include <functional>
#include <chrono>

#include "Constraint.h"
#include "FullColumn.h"
#include "enum.h"

using namespace MbD;

Constraint::Constraint()
{
}

Constraint::Constraint(const char* str) : Item(str)
{
}

void Constraint::initialize()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto nanoseconds = now.time_since_epoch().count();
	name = std::to_string(nanoseconds);
}

void MbD::Constraint::postInput()
{
	lam = 0.0;
	Item::postInput();
}

void Constraint::setOwner(Item* x)
{
	owner = x;
}

Item* Constraint::getOwner()
{
	return owner;
}

void MbD::Constraint::prePosIC()
{
	lam = 0.0;
	iG = -1;
	Item::prePosIC();
}

void MbD::Constraint::prePosKine()
{
	//"Preserve lam calculated from AccIC for possible use by DynIntegrator if system is not kinematic."
	auto lamOld = lam;
	this->prePosIC();
	lam = lamOld;
}

void MbD::Constraint::fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	if (this->type() == MbD::essential) {
		essenConstraints->push_back(sptr);
	}
}
void MbD::Constraint::fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	if (this->type() == MbD::displacement) {
		dispConstraints->push_back(sptr);
	}
}
void MbD::Constraint::fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	if (this->type() == MbD::perpendicular) {
		perpenConstraints->push_back(sptr);
	}
}

void MbD::Constraint::fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	if (this->type() == MbD::redundant) {
		redunConstraints->push_back(sptr);
	}
}

void MbD::Constraint::fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	allConstraints->push_back(sptr);
}

MbD::ConstraintType MbD::Constraint::type()
{
	return MbD::essential;
}

void MbD::Constraint::fillqsulam(FColDsptr col)
{
	col->at(iG) = lam;
}

void MbD::Constraint::setqsulam(FColDsptr col)
{
	lam = col->at(iG);
}

void MbD::Constraint::setqsudotlam(FColDsptr col)
{
	lam = col->at(iG);
}

void MbD::Constraint::fillPosICError(FColDsptr col)
{
	col->at(iG) += aG;
}

void MbD::Constraint::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
	//My owner should handle this.
	assert(false);
}

void MbD::Constraint::reactivateRedundantConstraints()
{
	//My owner should handle this.
	assert(false);
}

bool MbD::Constraint::isRedundant()
{
	return false;
}

void MbD::Constraint::outputStates()
{
	Item::outputStates();
	std::stringstream ss;
	ss << "iG = " << iG << std::endl;
	ss << "aG = " << aG << std::endl;
	ss << "lam = " << lam << std::endl;
	auto str = ss.str();
	this->logString(str);
}

void MbD::Constraint::preDyn()
{
	mu = 0.0;
}

void MbD::Constraint::fillPosKineError(FColDsptr col)
{
	col->atiplusNumber(iG, aG);
}

void MbD::Constraint::preAccIC()
{
	lam = 0.0;
	Item::preAccIC();
}

void MbD::Constraint::fillAccICIterJacob(SpMatDsptr mat)
{
	//"Same as velIC."
	this->fillVelICJacob(mat);
}

void MbD::Constraint::setqsuddotlam(FColDsptr qsudotlam)
{
	lam = qsudotlam->at(iG);
}
