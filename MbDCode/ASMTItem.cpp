#include "ASMTItem.h"
#include "CREATE.h"

using namespace MbD;

void MbD::ASMTItem::initialize()
{
}

void MbD::ASMTItem::parseASMT(std::vector<std::string>& lines)
{
	assert(false);
}

FRowDsptr MbD::ASMTItem::readRowOfDoubles(std::string line)
{
	std::istringstream iss(line);
	auto readRowOfDoubles = std::make_shared<FullRow<double>>();
	double d;
	while (iss >> d) {
		readRowOfDoubles->push_back(d);
	}
	return readRowOfDoubles;
}

FColDsptr MbD::ASMTItem::readColumnOfDoubles(std::string line)
{
	std::istringstream iss(line);
	auto readColumnOfDoubles = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		readColumnOfDoubles->push_back(d);
	}
	return readColumnOfDoubles;
}

double MbD::ASMTItem::readDouble(std::string line)
{
	std::istringstream iss(line);
	double d;
	iss >> d;
	return d;
}

int MbD::ASMTItem::readInt(std::string line)
{
	std::istringstream iss(line);
	int i;
	iss >> i;
	return i;
}

bool MbD::ASMTItem::readBool(std::string line)
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
	}
}
