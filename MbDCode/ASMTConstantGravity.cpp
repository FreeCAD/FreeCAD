#include "ASMTConstantGravity.h"
#include "ASMTAssembly.h"
#include "Units.h"
#include "ConstantGravity.h"
#include "System.h"
#include "Part.h"

using namespace MbD;

void MbD::ASMTConstantGravity::parseASMT(std::vector<std::string>& lines)
{
	g = readColumnOfDoubles(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTConstantGravity::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	auto mbdGravity = CREATE<ConstantGravity>::With();
	mbdObject = mbdGravity;
	mbdGravity->gXYZ = g->times(1.0 / mbdUnits->acceleration);
	mbdSys->addForceTorque(mbdGravity);
}
