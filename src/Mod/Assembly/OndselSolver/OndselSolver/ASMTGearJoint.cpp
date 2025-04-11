/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTGearJoint.h"
#include "GearJoint.h"
#include "SimulationStoppingError.h"

using namespace MbD;

std::shared_ptr<ASMTGearJoint> MbD::ASMTGearJoint::With()
{
	auto asmt = std::make_shared<ASMTGearJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTGearJoint::mbdClassNew()
{
    return CREATE<GearJoint>::With();
}

void MbD::ASMTGearJoint::parseASMT(std::vector<std::string>& lines)
{
	ASMTJoint::parseASMT(lines);
	readRadiusI(lines);
	readRadiusJ(lines);
}

void MbD::ASMTGearJoint::readRadiusI(std::vector<std::string>& lines)
{
	if (lines[0].find("radiusI") == std::string::npos) {
		radiusI = 0.0;
	}
	else {
		lines.erase(lines.begin());
		radiusI = readDouble(lines[0]);
		lines.erase(lines.begin());
	}
}

void MbD::ASMTGearJoint::readRadiusJ(std::vector<std::string>& lines)
{
	if (lines[0].find("radiusJ") == std::string::npos) {
		radiusJ = 0.0;
	}
	else {
		lines.erase(lines.begin());
		radiusJ = readDouble(lines[0]);
		lines.erase(lines.begin());
	}
}

void MbD::ASMTGearJoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	if (radiusI == 0.0 || radiusJ == 0.0) throw SimulationStoppingError("Gear radius is zero.");
	ASMTJoint::createMbD(mbdSys, mbdUnits);
	auto gearJoint = std::static_pointer_cast<GearJoint>(mbdObject);
	gearJoint->radiusI = radiusI;
	gearJoint->radiusJ = radiusJ;
}

void MbD::ASMTGearJoint::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTJoint::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "radiusI");
	storeOnLevelDouble(os, level + 2, radiusI);
	storeOnLevelString(os, level + 1, "radiusJ");
	storeOnLevelDouble(os, level + 2, radiusJ);
	//storeOnLevelString(os, level + 1, "constant");
	//storeOnLevelDouble(os, level + 2, aConstant);
}
