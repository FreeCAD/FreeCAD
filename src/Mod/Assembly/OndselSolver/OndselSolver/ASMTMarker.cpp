/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTMarker.h"
#include "FullMatrix.h"
#include "ASMTRefItem.h"
#include "ASMTPart.h"
#include "Part.h"
#include "PartFrame.h"
#include "MarkerFrame.h"
#include "ASMTPrincipalMassMarker.h"

using namespace MbD;

std::shared_ptr<ASMTMarker> MbD::ASMTMarker::With()
{
	auto asmt = std::make_shared<ASMTMarker>();
	asmt->initialize();
	return asmt;
}

void ASMTMarker::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readPosition3D(lines);
	readRotationMatrix(lines);
}

FColDsptr ASMTMarker::rpmp()
{
	//p is cm
	auto refItem = static_cast<ASMTRefItem*>(owner);
	auto& rPrefP = refItem->position3D;
	auto& aAPref = refItem->rotationMatrix;
	auto& rrefmref = position3D;
	auto rPmP = rPrefP->plusFullColumn(aAPref->timesFullColumn(rrefmref));
	auto& principalMassMarker = static_cast<ASMTPart*>(refItem->owner)->principalMassMarker;
	auto& rPcmP = principalMassMarker->position3D;
	auto& aAPcm = principalMassMarker->rotationMatrix;
	auto rpmp = aAPcm->transposeTimesFullColumn(rPmP->minusFullColumn(rPcmP));
	return rpmp;
}

FMatDsptr ASMTMarker::aApm()
{
	//p is cm
	auto refItem = static_cast<ASMTRefItem*>(owner);
	auto& aAPref = refItem->rotationMatrix;
	auto& aArefm = rotationMatrix;
	auto& principalMassMarker = static_cast<ASMTPart*>(refItem->owner)->principalMassMarker;
	auto& aAPcm = principalMassMarker->rotationMatrix;
	auto aApm = aAPcm->transposeTimesFullMatrix(aAPref->timesFullMatrix(aArefm));
	return aApm;
}

void ASMTMarker::createMbD(std::shared_ptr<System>, std::shared_ptr<Units> mbdUnits)
{
	auto mkr = CREATE<MarkerFrame>::With(name.c_str());
	auto prt = std::static_pointer_cast<Part>(partOrAssembly()->mbdObject);
	prt->partFrame->addMarkerFrame(mkr);

	mkr->rpmp = rpmp()->times(1.0 / mbdUnits->length);
	mkr->aApm = aApm();
	mbdObject = mkr->endFrames->at(0);
}

void ASMTMarker::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "Marker");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTSpatialItem::storeOnLevel(os, level);
}
