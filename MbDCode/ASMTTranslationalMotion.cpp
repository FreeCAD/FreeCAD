#include "ASMTTranslationalMotion.h"
#include "ASMTAssembly.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"

using namespace MbD;

void MbD::ASMTTranslationalMotion::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readMotionJoint(lines);
	readTranslationZ(lines);

	//size_t pos = lines[0].find_first_not_of("\t");
	//auto leadingTabs = lines[0].substr(0, pos);
	//assert(lines[0] == (leadingTabs + "Name"));
	//lines.erase(lines.begin());
	//name = lines[0];
	//lines.erase(lines.begin());
	//assert(lines[0] == (leadingTabs + "MotionJoint"));
	//lines.erase(lines.begin());
	//motionJoint = lines[0];
	//lines.erase(lines.begin());
	//assert(lines[0] == (leadingTabs + "TranslationZ"));
	//lines.erase(lines.begin());
	//translationZ = lines[0];
	//lines.erase(lines.begin());
}

void MbD::ASMTTranslationalMotion::initMarkers()
{
	auto jt = root()->jointAt(motionJoint);
	markerI = jt->markerI;
	markerJ = jt->markerJ;
}

void MbD::ASMTTranslationalMotion::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTMotion::createMbD(mbdSys, mbdUnits);
	auto parser = CREATE<SymbolicParser>::With();
	parser->owner = this;
	auto geoTime = owner->root()->geoTime();
	parser->variables->insert(std::make_pair("time", geoTime));
	auto userFunc = CREATE<BasicUserFunction>::With(translationZ, 1.0);
	parser->parseUserFunction(userFunc);
	auto zIJ = parser->stack->top();
	zIJ = Symbolic::times(zIJ, std::make_shared<Constant>(1.0 / mbdUnits->length));
	zIJ->createMbD(mbdSys, mbdUnits);
	std::static_pointer_cast<ZTranslation>(mbdObject)->zBlk = zIJ->simplified(zIJ);
}

std::shared_ptr<Joint> MbD::ASMTTranslationalMotion::mbdClassNew()
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

void MbD::ASMTTranslationalMotion::readTranslationZ(std::vector<std::string>& lines)
{
	assert(lines[0].find("TranslationZ") != std::string::npos);
	lines.erase(lines.begin());
	translationZ = readString(lines[0]);
	lines.erase(lines.begin());
}
