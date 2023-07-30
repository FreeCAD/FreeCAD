#include "ASMTItem.h"
#include "CREATE.h"
#include "ASMTSpatialContainer.h"
#include "ASMTAssembly.h"

using namespace MbD;

ASMTAssembly* MbD::ASMTItem::root()
{
	return owner->root();
}

std::shared_ptr<ASMTSpatialContainer> MbD::ASMTItem::part()
{
	return owner->part();
}

void MbD::ASMTItem::initialize()
{
}

void MbD::ASMTItem::parseASMT(std::vector<std::string>& lines)
{
	assert(false);
}

FRowDsptr MbD::ASMTItem::readRowOfDoubles(std::string& line)
{
	std::istringstream iss(line);
	auto readRowOfDoubles = std::make_shared<FullRow<double>>();
	double d;
	while (iss >> d) {
		readRowOfDoubles->push_back(d);
	}
	return readRowOfDoubles;
}

FColDsptr MbD::ASMTItem::readColumnOfDoubles(std::string& line)
{
	std::istringstream iss(line);
	auto readColumnOfDoubles = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		readColumnOfDoubles->push_back(d);
	}
	return readColumnOfDoubles;
}

double MbD::ASMTItem::readDouble(std::string& line)
{
	std::istringstream iss(line);
	double d;
	iss >> d;
	return d;
}

int MbD::ASMTItem::readInt(std::string& line)
{
	std::istringstream iss(line);
	int i;
	iss >> i;
	return i;
}

bool MbD::ASMTItem::readBool(std::string& line)
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

std::string MbD::ASMTItem::readString(std::string& line)
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

std::string MbD::ASMTItem::fullName(std::string partialName)
{
	std::string longerName = "/" + name + partialName;
	if (owner == nullptr) {
		return longerName;
	}
	else {
		return owner->fullName(longerName);
	}
}

void MbD::ASMTItem::readDoublesInto(std::string& str, std::string label, FRowDsptr& row)
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

void MbD::ASMTItem::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	assert(false);
}

void MbD::ASMTItem::updateFromMbD()
{
	assert(false);
}

void MbD::ASMTItem::compareResults(AnalysisType type)
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
