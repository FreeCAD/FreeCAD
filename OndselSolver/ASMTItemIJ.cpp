/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTItemIJ.h"
#include "ASMTAssembly.h"
#include "ASMTMarker.h"
#include "ItemIJ.h"

using namespace MbD;

MbD::ASMTItemIJ::ASMTItemIJ()
{
	//Use initialize to be consistent with Smalltalk Twin
	//fxs = std::make_shared<FullRow<double>>();
	//fys = std::make_shared<FullRow<double>>();
	//fzs = std::make_shared<FullRow<double>>();
	//txs = std::make_shared<FullRow<double>>();
	//tys = std::make_shared<FullRow<double>>();
	//tzs = std::make_shared<FullRow<double>>();
}

void MbD::ASMTItemIJ::initialize()
{
	fxs = std::make_shared<FullRow<double>>();
	fys = std::make_shared<FullRow<double>>();
	fzs = std::make_shared<FullRow<double>>();
	txs = std::make_shared<FullRow<double>>();
	tys = std::make_shared<FullRow<double>>();
	tzs = std::make_shared<FullRow<double>>();
}

void MbD::ASMTItemIJ::setMarkerI(const std::string& mkrI)
{
	markerI = mkrI;
}

void MbD::ASMTItemIJ::setMarkerJ(const std::string& mkrJ)
{
	markerJ = mkrJ;
}

void MbD::ASMTItemIJ::readMarkerI(std::vector<std::string>& lines)
{
	assert(lines[0].find("MarkerI") != std::string::npos);
	lines.erase(lines.begin());
	markerI = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readMarkerJ(std::vector<std::string>& lines)
{
	assert(lines[0].find("MarkerJ") != std::string::npos);
	lines.erase(lines.begin());
	markerJ = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readFXonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FXonI", infxs);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readFYonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FYonI", infys);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readFZonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FZonI", infzs);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readTXonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TXonI", intxs);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readTYonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TYonI", intys);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::readTZonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TZonI", intzs);
	lines.erase(lines.begin());
}

void MbD::ASMTItemIJ::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTItem::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "MarkerI");
	storeOnLevelString(os, level + 2, markerI);
	storeOnLevelString(os, level + 1, "MarkerJ");
	storeOnLevelString(os, level + 2, markerJ);
}

void MbD::ASMTItemIJ::storeOnTimeSeries(std::ofstream& os)
{
	os << "FXonI\t";
	for (size_t i = 0; i < fxs->size(); i++)
	{
		os << fxs->at(i) << '\t';
	}
	os << std::endl;
	os << "FYonI\t";
	for (size_t i = 0; i < fys->size(); i++)
	{
		os << fys->at(i) << '\t';
	}
	os << std::endl;
	os << "FZonI\t";
	for (size_t i = 0; i < fzs->size(); i++)
	{
		os << fzs->at(i) << '\t';
	}
	os << std::endl;
	os << "TXonI\t";
	for (size_t i = 0; i < txs->size(); i++)
	{
		os << txs->at(i) << '\t';
	}
	os << std::endl;
	os << "TYonI\t";
	for (size_t i = 0; i < tys->size(); i++)
	{
		os << tys->at(i) << '\t';
	}
	os << std::endl;
	os << "TZonI\t";
	for (size_t i = 0; i < tzs->size(); i++)
	{
		os << tzs->at(i) << '\t';
	}
	os << std::endl;
}

void MbD::ASMTItemIJ::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readMarkerI(lines);
	readMarkerJ(lines);
}

void MbD::ASMTItemIJ::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
    (void) mbdSys;
    (void) mbdUnits;
	//self dataSeries : OrderedCollection new.
	//self discontinuities : OrderedCollection new.
	auto mbdJt = this->mbdClassNew();
	mbdObject = mbdJt;
	mbdJt->name = fullName("");
	auto mrkI = std::static_pointer_cast<EndFramec>(root()->markerAt(markerI)->mbdObject);
	auto mrkJ = std::static_pointer_cast<EndFramec>(root()->markerAt(markerJ)->mbdObject);
	mbdJt->connectsItoJ(mrkI, mrkJ);
}

std::shared_ptr<ItemIJ> MbD::ASMTItemIJ::mbdClassNew()
{
	assert(false);
	return std::shared_ptr<ItemIJ>();
}
