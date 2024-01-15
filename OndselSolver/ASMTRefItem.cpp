/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTRefItem.h"
#include "CREATE.h"
#include <algorithm>

using namespace MbD;

void MbD::ASMTRefItem::addMarker(std::shared_ptr<ASMTMarker> marker)
{
	markers->push_back(marker);
	marker->owner = this;
}

void MbD::ASMTRefItem::readMarkers(std::vector<std::string>& lines)
{
	assert(lines[0].find("Markers") != std::string::npos);
	lines.erase(lines.begin());
	markers->clear();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefPoint") != std::string::npos;
		});
	std::vector<std::string> markersLines(lines.begin(), it);
	while (!markersLines.empty()) {
		readMarker(markersLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTRefItem::readMarker(std::vector<std::string>& lines)
{
	assert(lines[0].find("Marker") != std::string::npos);
	lines.erase(lines.begin());
	auto marker = CREATE<ASMTMarker>::With();
	marker->parseASMT(lines);
	markers->push_back(marker);
	marker->owner = this;
}

void MbD::ASMTRefItem::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RefPoints");
	ASMTSpatialItem::storeOnLevel(os, level+1);
	for (auto& marker : *markers) {
		marker->storeOnLevel(os, level);
	}
}
