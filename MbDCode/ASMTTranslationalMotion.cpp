#include "ASMTTranslationalMotion.h"
#include "ASMTAssembly.h"

using namespace MbD;

void MbD::ASMTTranslationalMotion::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "Name"));
	lines.erase(lines.begin());
	name = lines[0];
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "MotionJoint"));
	lines.erase(lines.begin());
	motionJoint = lines[0];
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "TranslationZ"));
	lines.erase(lines.begin());
	translationZ = lines[0];
	lines.erase(lines.begin());
}

void MbD::ASMTTranslationalMotion::initMarkers()
{
	auto jt = root()->jointAt(motionJoint);
	markerI = jt->markerI;
	markerJ = jt->markerJ;
}

void MbD::ASMTTranslationalMotion::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	//ASMTMotion::createMbD(mbdSys, mbdUnits);
	//	zFunc : = zIJ isNil
	//	ifTrue : [StMConstant with : 0]
	//	ifFalse : [zIJ isUserFunction
	//	ifTrue :
	//[parser:= self functionParser.
	//	stack : = parser
	//	parseUserFunction : zIJ
	//	notifying : nil
	//	ifFail : nil.
	//	func : = stack last.
	//	func]
	//ifFalse: [zIJ] ] .
	//	zFunc : = (zFunc / self mbdUnits length) createMbD simplified.
	//	mbdZTranslation : = self mbdObject.
	//	mbdZTranslation zBlk : zFunc
}

std::shared_ptr<Joint> MbD::ASMTTranslationalMotion::mbdClassNew()
{
	return CREATE<ZTranslation>::With();
}
