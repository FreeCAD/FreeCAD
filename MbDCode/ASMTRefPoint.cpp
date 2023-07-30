#include "ASMTRefPoint.h"
#include "ASMTMarker.h"
#include "CREATE.h"

using namespace MbD;

void MbD::ASMTRefPoint::parseASMT(std::vector<std::string>& lines)
{
	readPosition3D(lines);
	readRotationMatrix(lines);
	readMarkers(lines);
}

std::string MbD::ASMTRefPoint::fullName(std::string partialName)
{
	return owner->fullName(partialName);
}

void MbD::ASMTRefPoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	for (auto& marker : *markers) {
		marker->createMbD(mbdSys, mbdUnits);
	}
}
