#include "ASMTMarker.h"

using namespace MbD;

void MbD::ASMTMarker::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readPosition3D(lines);
	readRotationMatrix(lines);
}
