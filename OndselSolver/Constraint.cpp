/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <functional>
#include <chrono>
#include <cstdint>

#include "Constraint.h"
#include "FullColumn.h"
#include "enum.h"

using namespace MbD;

Constraint::Constraint() : Item()
{
	aConstant = 0.0;
}

Constraint::Constraint(const std::string& str) : Item(str)
{
}

void Constraint::initialize()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto nanoseconds = now.time_since_epoch().count();
	name = std::to_string(nanoseconds);
}

void Constraint::postInput()
{
	lam = 0.0;
	Item::postInput();
}

void Constraint::prePosIC()
{
	lam = 0.0;
	iG = SIZE_MAX;
	Item::prePosIC();
}

void Constraint::prePosKine()
{
	//"Preserve lam calculated from AccIC for possible use by DynIntegrator if system is not kinematic."
	auto lamOld = lam;
	this->prePosIC();
	lam = lamOld;
}

void Constraint::fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	if (this->type() == essential) {
		essenConstraints->push_back(sptr);
	}
}
void Constraint::fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	if (this->type() == displacement) {
		dispConstraints->push_back(sptr);
	}
}
void Constraint::fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	if (this->type() == perpendicular) {
		perpenConstraints->push_back(sptr);
	}
}

void Constraint::fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	if (this->type() == redundant) {
		redunConstraints->push_back(sptr);
	}
}

void Constraint::fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	allConstraints->push_back(sptr);
}

ConstraintType Constraint::type()
{
	return essential;
}

void Constraint::fillqsulam(FColDsptr col)
{
	col->atiput(iG, lam);
}

void Constraint::setqsulam(FColDsptr col)
{
	lam = col->at(iG);
}

void Constraint::setqsudotlam(FColDsptr col)
{
	lam = col->at(iG);
}

void Constraint::fillPosICError(FColDsptr col)
{
	col->atiplusNumber(iG, aG);
}

void Constraint::removeRedundantConstraints(std::shared_ptr<std::vector<size_t>>)
{
	//My owner should handle this.
	assert(false);
}

void MbD::Constraint::setConstant(double value)
{
	aConstant = value;
}

void Constraint::reactivateRedundantConstraints()
{
	//My owner should handle this.
	assert(false);
}

bool Constraint::isRedundant()
{
	return false;
}

void Constraint::preDyn()
{
	mu = 0.0;
}

void Constraint::fillPosKineError(FColDsptr col)
{
	col->atiplusNumber(iG, aG);
}

void Constraint::fillqsuddotlam(FColDsptr col)
{
	col->atiput(iG, lam);
}

void Constraint::preAccIC()
{
	lam = 0.0;
	Item::preAccIC();
}

void Constraint::fillAccICIterJacob(SpMatDsptr mat)
{
	//"Same as velIC."
	this->fillVelICJacob(mat);
}

void Constraint::setqsuddotlam(FColDsptr col)
{
	lam = col->at(iG);
}

void Constraint::addToJointForceI(FColDsptr)
{
}

void Constraint::addToJointTorqueI(FColDsptr)
{
}

void Constraint::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) {
    Item::fillConstraints(allConstraints);
}

void Constraint::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) {
    Item::fillRedundantConstraints(redunConstraints);
}

void Constraint::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints) {
    Item::fillDispConstraints(dispConstraints);
}

void Constraint::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) {
    Item::fillEssenConstraints(essenConstraints);
}

void Constraint::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints) {
    Item::fillPerpenConstraints(perpenConstraints);
}
