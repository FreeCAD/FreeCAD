#include <string>
#include <cassert>
#include <fstream>	
#include <algorithm>
#include <numeric>

#include "MBDynSystem.h"
#include "CREATE.h"
#include "FullColumn.h"
#include "MBDynInitialValue.h"
#include "MBDynData.h"
#include "MBDynControlData.h"
#include "MBDynLabels.h"
#include "MBDynVariables.h"
#include "MBDynReferences.h"
#include "MBDynNodes.h"

using namespace MbD;

void MbD::MBDynSystem::runFile(const char* filename)
{
	std::ifstream stream(filename);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(stream, line)) {
		lines.push_back(line);
	}
	eraseComments(lines);
	auto statements = collectStatements(lines);

	auto system = CREATE<MBDynSystem>::With();
	system->setFilename(filename);
	system->parseMBDyn(statements);
	system->runKINEMATIC();
}

void MbD::MBDynSystem::parseMBDyn(std::vector<std::string>& lines)
{
	readDataBlock(lines);
	readInitialValueBlock(lines);
	readControlDataBlock(lines);
	readLabels(lines);
	readVariables(lines);
	readReferences(lines);
	readNodesBlock(lines);
	readElementsBlock(lines);

}

std::shared_ptr<MBDynVariables> MbD::MBDynSystem::mbdynVariables()
{
	return variables;
}

void MbD::MBDynSystem::runKINEMATIC()
{
	assert(false);
}

void MbD::MBDynSystem::setFilename(std::string str)
{
	filename = str;
}

void MbD::MBDynSystem::readDataBlock(std::vector<std::string>& lines)
{
	std::vector<std::string> tokens{"begin:", "data;"};
	auto beginit = findLineWith(lines, tokens);
	std::vector<std::string> tokens1{"end:", "data;"};
	auto endit = findLineWith(lines, tokens1);
	std::vector<std::string> blocklines = { beginit, endit + 1 };
	dataBlk = CREATE<MBDynData>::With();
	dataBlk->owner = this;
	dataBlk->parseMBDyn(blocklines);
	lines.erase(beginit, endit + 1);
}

void MbD::MBDynSystem::readInitialValueBlock(std::vector<std::string>& lines)
{
	std::vector<std::string> tokens{"begin:", "initial", "value"};
	auto beginit = findLineWith(lines, tokens);
	std::vector<std::string> tokens1{"end:", "initial", "value"};
	auto endit = findLineWith(lines, tokens1);
	std::vector<std::string> blocklines = { beginit, endit + 1 };
	initialValueBlk = CREATE<MBDynInitialValue>::With();
	initialValueBlk->owner = this;
	initialValueBlk->parseMBDyn(blocklines);
	lines.erase(beginit, endit + 1);
}

void MbD::MBDynSystem::readControlDataBlock(std::vector<std::string>& lines)
{
	std::vector<std::string> tokens{"begin:", "control", "data"};
	auto beginit = findLineWith(lines, tokens);
	std::vector<std::string> tokens1{"end:", "control", "data"};
	auto endit = findLineWith(lines, tokens1);
	std::vector<std::string> blocklines = { beginit, endit + 1 };
	controlDataBlk = CREATE<MBDynControlData>::With();
	controlDataBlk->owner = this;
	controlDataBlk->parseMBDyn(blocklines);
	lines.erase(beginit, endit + 1);
}

void MbD::MBDynSystem::readLabels(std::vector<std::string>& lines)
{
	labels = CREATE<MBDynLabels>::With();
	labels->owner = this;
	labels->parseMBDyn(lines);
}

void MbD::MBDynSystem::readVariables(std::vector<std::string>& lines)
{
	variables = CREATE<MBDynVariables>::With();
	variables->owner = this;
	variables->parseMBDyn(lines);
}

void MbD::MBDynSystem::readReferences(std::vector<std::string>& lines)
{
	references = CREATE<MBDynReferences>::With();
	references->owner = this;
	references->parseMBDyn(lines);
}

void MbD::MBDynSystem::readNodesBlock(std::vector<std::string>& lines)
{
	std::vector<std::string> tokens{ "begin:", "nodes;" };
	auto beginit = findLineWith(lines, tokens);
	std::vector<std::string> tokens1{ "end:", "nodes;" };
	auto endit = findLineWith(lines, tokens1);
	std::vector<std::string> blocklines = { beginit, endit + 1 };
	nodesBlk = CREATE<MBDynNodes>::With();
	nodesBlk->owner = this;
	nodesBlk->parseMBDyn(blocklines);
	lines.erase(beginit, endit + 1);
}

void MbD::MBDynSystem::readElementsBlock(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::MBDynSystem::eraseComments(std::vector<std::string>& lines)
{
	for (int i = 0; i < lines.size(); i++)
	{
		auto line = lines[i];
		auto it = line.find('#');
		if (it != std::string::npos) {
			lines[i] = line.substr(0, it);
		}
	}
	for (int i = lines.size() - 1; i >= 0; i--) {
		auto& line = lines[i];
		auto it = std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); });
		if (it == line.end()) lines.erase(lines.begin() + i);
	}
}

std::vector<std::string> MbD::MBDynSystem::collectStatements(std::vector<std::string>& lines)
{
	auto statements = std::vector<std::string>();
	while (!lines.empty()) {
		std::stringstream ss;
		while (!lines.empty()) {
			auto line = lines[0];
			lines.erase(lines.begin());
			auto i = line.find(';');
			if (i != std::string::npos) {
				ss << line.substr(0, i + 1);
				if (line.size() > i + 1) {
					auto remainder = line.substr(i + 1);
					auto it = std::find_if(remainder.begin(), remainder.end(), [](unsigned char ch) { return !std::isspace(ch); });
					if (it != remainder.end()) lines.insert(lines.begin(), remainder);
				}
				break;
			}
			else {
				ss << line;
			}
		}
		statements.push_back(ss.str());
	}
	return statements;
}

void MbD::MBDynSystem::initialize()
{
}
