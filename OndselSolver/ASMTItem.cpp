/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTItem.h"
#include "CREATE.h"
#include "ASMTSpatialContainer.h"
#include "ASMTAssembly.h"
#include "Constant.h"
#include <algorithm>

using namespace MbD;

ASMTAssembly* MbD::ASMTItem::root()
{
	return owner->root();
}

ASMTSpatialContainer* MbD::ASMTItem::partOrAssembly()
{
	return owner->partOrAssembly();
}

ASMTPart* MbD::ASMTItem::part()
{
	return owner->part();
}

void MbD::ASMTItem::initialize()
{
}

void MbD::ASMTItem::noop()
{
	//No Operations
}

std::string MbD::ASMTItem::classname()
{
	std::string str = typeid(*this).name();
	auto answer = str.substr(11, str.size() - 11);
	return answer;
}

void MbD::ASMTItem::setName(const std::string& str)
{
	name = str;
}

void MbD::ASMTItem::parseASMT(std::vector<std::string>&)
{
	assert(false);
}

std::string MbD::ASMTItem::popOffTop(std::vector<std::string>& args)
{
	std::string str = args.at(0);	//Must copy string
	args.erase(args.begin());
	return str;
}

std::string MbD::ASMTItem::readStringOffTop(std::vector<std::string>& args)
{
	auto iss = std::istringstream(args.at(0));
	args.erase(args.begin());
	std::string str;
	iss >> str;
	return str;
}

FRowDsptr MbD::ASMTItem::readRowOfDoubles(const std::string& line)
{
	std::istringstream iss(line);
	auto readRowOfDoubles = std::make_shared<FullRow<double>>();
	double d;
	while (iss >> d) {
		readRowOfDoubles->push_back(d);
	}
	return readRowOfDoubles;
}

FRowDsptr MbD::ASMTItem::readRowOfDoublesOffTop(std::vector<std::string>& lines)
{
	auto str = popOffTop(lines);
	return readRowOfDoubles(str);
}

FColDsptr MbD::ASMTItem::readColumnOfDoubles(const std::string& line)
{
	std::istringstream iss(line);
	auto readColumnOfDoubles = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		readColumnOfDoubles->push_back(d);
	}
	return readColumnOfDoubles;
}

FColDsptr MbD::ASMTItem::readColumnOfDoublesOffTop(std::vector<std::string>& lines)
{
	auto str = popOffTop(lines);
	return readColumnOfDoubles(str);
}

double MbD::ASMTItem::readDouble(const std::string& line)
{
	std::istringstream iss(line);
	double d;
	iss >> d;
	return d;
}

int MbD::ASMTItem::readInt(const std::string& line)
{
	std::istringstream iss(line);
	int i;
	iss >> i;
	return i;
}

size_t MbD::ASMTItem::readSize_t(const std::string& line)
{
	std::istringstream iss(line);
	size_t i;
	iss >> i;
	return i;
}

bool MbD::ASMTItem::readBool(const std::string& line)
{
	if (line.find("true") != std::string::npos)
	{
		return true;
	}
	else 	if (line.find("false") != std::string::npos)
	{
		return false;
	}
	else {
		assert(false);
		return false;
	}
}

std::string MbD::ASMTItem::readString(const std::string& line)
{
	std::string str = line;
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	return str;
}

void MbD::ASMTItem::readName(std::vector<std::string>& lines)
{
	assert(lines[0].find("Name") != std::string::npos);
	lines.erase(lines.begin());
	name = readString(lines[0]);
	lines.erase(lines.begin());
}

std::string MbD::ASMTItem::fullName(const std::string& partialName)
{
	std::string longerName = "/" + name + partialName;
	if (owner == nullptr) {
		return longerName;
	}
	else {
		return owner->fullName(longerName);
	}
}

void MbD::ASMTItem::readDoublesInto(std::string str, std::string label, FRowDsptr& row)
{
	auto pos = str.find(label);
	assert(pos != std::string::npos);
	str.erase(0, pos + label.length());
	row = readRowOfDoubles(str);
}

void MbD::ASMTItem::deleteMbD()
{
	mbdObject = nullptr;
}

void MbD::ASMTItem::createMbD(std::shared_ptr<System>, std::shared_ptr<Units>)
{
	noop();
	assert(false);
}

void MbD::ASMTItem::updateForFrame(size_t index)
{
    assert(false);
}

void MbD::ASMTItem::updateFromInitiallyAssembledState()
{
    assert(false);
}

void MbD::ASMTItem::updateFromInputState()
{
    assert(false);
}

void MbD::ASMTItem::updateFromMbD()
{
	assert(false);
}

void MbD::ASMTItem::compareResults(AnalysisType)
{
	assert(false);
}

void MbD::ASMTItem::outputResults(AnalysisType)
{
	assert(false);
}

std::shared_ptr<Units> MbD::ASMTItem::mbdUnits()
{
	if (owner) {
		return owner->mbdUnits();
	}
	return static_cast<ASMTAssembly*>(this)->mbdUnits;
}

std::shared_ptr<Constant> MbD::ASMTItem::sptrConstant(double value)
{
	return std::make_shared<Constant>(value);
}

void MbD::ASMTItem::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
}

void MbD::ASMTItem::storeOnLevelTabs(std::ofstream& os, size_t level)
{
	for (size_t i = 0; i < level; i++)
	{
		os << '\t';
	}
}

void MbD::ASMTItem::storeOnLevelString(std::ofstream& os, size_t level, std::string str)
{
	storeOnLevelTabs(os, level);
	os << str << std::endl;
}

void MbD::ASMTItem::storeOnLevelDouble(std::ofstream& os, size_t level, double value)
{
	storeOnLevelTabs(os, level);
	os << value << std::endl;
}

void MbD::ASMTItem::storeOnLevelInt(std::ofstream& os, size_t level, int i)
{
	storeOnLevelTabs(os, level);
	os << i << std::endl;
}

void MbD::ASMTItem::storeOnLevelSize_t(std::ofstream& os, size_t level, size_t i)
{
	storeOnLevelTabs(os, level);
	os << i << std::endl;
}

void MbD::ASMTItem::storeOnLevelBool(std::ofstream& os, size_t level, bool value)
{
	storeOnLevelTabs(os, level);
	if (value) {
		os << "true" << std::endl;
	}
	else {
		os << "false" << std::endl;
	}
}

void MbD::ASMTItem::storeOnLevelArray(std::ofstream& os, size_t level, std::vector<double> array)
{
	storeOnLevelTabs(os, level);
	for (size_t i = 0; i < array.size(); i++)
	{
		os << array[i] << '\t';
	}
	os << std::endl;
}

void MbD::ASMTItem::storeOnLevelName(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "Name");
	storeOnLevelString(os, level + 1, name);
}

void MbD::ASMTItem::storeOnTimeSeries(std::ofstream&)
{
	assert(false);
}

void MbD::ASMTItem::logString(const std::string& str)
{
	std::cout << str << std::endl;
}
