#include "ASMTLimit.h"
#include "ASMTAssembly.h"
#include "ASMTJoint.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"

using namespace MbD;

void MbD::ASMTLimit::initMarkers()
{
	if (motionJoint == "") {
		assert(markerI != "");
		assert(markerJ != "");
	}
	else {
		auto jt = root()->jointAt(motionJoint);
		markerI = jt->markerI;
		markerJ = jt->markerJ;
	}
}

void MbD::ASMTLimit::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTItemIJ::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "MotionJoint");
	storeOnLevelString(os, level + 2, motionJoint);
	storeOnLevelString(os, level + 1, "Limit");
	storeOnLevelString(os, level + 2, limit);
	storeOnLevelString(os, level + 1, "Type");
	storeOnLevelString(os, level + 2, type);
	storeOnLevelString(os, level + 1, "Tol");
	storeOnLevelString(os, level + 2, tol);
}

void MbD::ASMTLimit::readMotionJoint(std::vector<std::string>& lines)
{
	assert(readStringOffTop(lines) == "MotionJoint");
	motionJoint = readStringOffTop(lines);
}

void MbD::ASMTLimit::readLimit(std::vector<std::string>& lines)
{
	assert(readStringOffTop(lines) == "Limit");
	limit = readStringOffTop(lines);
}

void MbD::ASMTLimit::readType(std::vector<std::string>& lines)
{
	assert(readStringOffTop(lines) == "Type");
	type = readStringOffTop(lines);
}

void MbD::ASMTLimit::readTol(std::vector<std::string>& lines)
{
	assert(readStringOffTop(lines) == "Tol");
	tol = readStringOffTop(lines);
}

void MbD::ASMTLimit::parseASMT(std::vector<std::string>& lines)
{
	ASMTConstraintSet::parseASMT(lines);
	readMotionJoint(lines);
	readLimit(lines);
	readType(lines);
	readTol(lines);
}

void MbD::ASMTLimit::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTConstraintSet::createMbD(mbdSys, mbdUnits);
	auto limitIJ = std::static_pointer_cast<LimitIJ>(mbdObject);
	mbdSys->addLimit(limitIJ);
	//
	auto parser = std::make_shared<SymbolicParser>();
	parser->owner = this;
	std::shared_ptr<BasicUserFunction> userFunc;
	//
	userFunc = std::make_shared<BasicUserFunction>(limit, 1.0);
	parser->parseUserFunction(userFunc);
	auto& geolimit = parser->stack->top();
	geolimit = Symbolic::times(geolimit, sptrConstant(1.0 / mbdUnits->angle));
	geolimit->createMbD(mbdSys, mbdUnits);
	geolimit = geolimit->simplified(geolimit);
	limitIJ->limit = geolimit->getValue();
	//
	limitIJ->type = type;
	//
	userFunc = std::make_shared<BasicUserFunction>(tol, 1.0);
	parser->parseUserFunction(userFunc);
	auto& geotol = parser->stack->top();
	geotol = Symbolic::times(geotol, sptrConstant(1.0 / mbdUnits->angle));
	geotol->createMbD(mbdSys, mbdUnits);
	geotol = geotol->simplified(geotol);
	limitIJ->tol = geotol->getValue();
}

void MbD::ASMTLimit::setmotionJoint(const std::string& _motionJoint)
{
	motionJoint = _motionJoint;
}

void MbD::ASMTLimit::settype(const std::string& _type)
{
	type = _type;
}

void MbD::ASMTLimit::setlimit(const std::string& _limit)
{
	limit = _limit;
}

void MbD::ASMTLimit::settol(const std::string& _tol)
{
	tol = _tol;
}
