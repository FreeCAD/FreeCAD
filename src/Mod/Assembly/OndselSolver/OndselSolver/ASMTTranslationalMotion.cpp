/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTTranslationalMotion.h"
#include "ASMTAssembly.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"
#include "ASMTJoint.h"
#include "ZTranslation.h"
#include "ASMTTime.h"

using namespace MbD;

void MbD::ASMTTranslationalMotion::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readMotionJoint(lines);
	readTranslationZ(lines);
}

void MbD::ASMTTranslationalMotion::initMarkers()
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

void MbD::ASMTTranslationalMotion::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTMotion::createMbD(mbdSys, mbdUnits);
	auto parser = std::make_shared<SymbolicParser>();
	parser->owner = this;
	auto geoTime = owner->root()->geoTime();
	parser->variables->insert(std::make_pair("time", geoTime));
	auto userFunc = std::make_shared<BasicUserFunction>(translationZ, 1.0);
	parser->parseUserFunction(userFunc);
	auto& zIJ = parser->stack->top();
	zIJ = Symbolic::times(zIJ, sptrConstant(1.0 / mbdUnits->length));
	zIJ->createMbD(mbdSys, mbdUnits);
	std::static_pointer_cast<ZTranslation>(mbdObject)->zBlk = zIJ->simplified(zIJ);
}

std::shared_ptr<ItemIJ> MbD::ASMTTranslationalMotion::mbdClassNew()
{
	return CREATE<ZTranslation>::With();
}

void MbD::ASMTTranslationalMotion::readMotionJoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("MotionJoint") != std::string::npos);
	lines.erase(lines.begin());
	motionJoint = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTTranslationalMotion::setTranslationZ(std::string tranZ)
{
    translationZ = tranZ;
}

void MbD::ASMTTranslationalMotion::readTranslationZ(std::vector<std::string>& lines)
{
	assert(lines[0].find("TranslationZ") != std::string::npos);
	lines.erase(lines.begin());
	translationZ = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTTranslationalMotion::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "TranslationalMotion");
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTTranslationalMotion::storeOnTimeSeries(std::ofstream& os)
{
	os << "TranslationalMotionSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
