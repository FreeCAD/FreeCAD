/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

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

std::string MbD::ASMTRefPoint::fullName(const std::string& partialName)
{
	return owner->fullName(partialName);
}

void MbD::ASMTRefPoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	for (auto& marker : *markers) {
		marker->createMbD(mbdSys, mbdUnits);
	}
}

void MbD::ASMTRefPoint::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RefPoint");
	ASMTSpatialItem::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "Markers");
	for (auto& marker : *markers) {
		marker->storeOnLevel(os, level + 2);
	}
}
