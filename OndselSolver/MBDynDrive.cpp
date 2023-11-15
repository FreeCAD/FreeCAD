#include <regex>

#include "MBDynDrive.h"

using namespace MbD;

void MbD::MBDynDrive::parseMBDyn(std::string line)
{
	driveString = line;
	size_t previousPos = 0;
	auto pos = line.find(":");
	auto front = line.substr(previousPos, pos - previousPos);
	assert(front.find("drive caller") != std::string::npos);
	auto arguments = std::vector<std::string>();
	std::string argument;
	while (true) {
		previousPos = pos;
		pos = line.find(",", pos + 1);
		if (pos != std::string::npos) {
			argument = line.substr(previousPos + 1, pos - previousPos - 1);
			arguments.push_back(argument);
		}
		else {
			argument = line.substr(previousPos + 1);
			arguments.push_back(argument);
			break;
		}
	}
	auto iss = std::istringstream(arguments.at(0));
	iss >> name;
	arguments.erase(arguments.begin());
	assert(arguments.at(0).find("name") != std::string::npos);
	arguments.erase(arguments.begin());
	iss = std::istringstream(arguments.at(0));
	iss >> driveName;
	driveName = std::regex_replace(driveName, std::regex("\""), "");
	arguments.erase(arguments.begin());
	readFunction(arguments);
}

void MbD::MBDynDrive::readFunction(std::vector<std::string>& args)
{
	if (args.empty()) return;
	std::string str;
	auto iss = std::istringstream(args.at(0));
	iss >> str;
	if (str.find("ramp") != std::string::npos) {
		args.erase(args.begin());
		std::string slope, initValue, initTime, finalTime;
		slope = popOffTop(args);
		initTime = popOffTop(args);
		finalTime = popOffTop(args);
		initValue = popOffTop(args);
		slope.erase(remove_if(slope.begin(), slope.end(), isspace), slope.end());
		initTime.erase(remove_if(initTime.begin(), initTime.end(), isspace), initTime.end());
		finalTime.erase(remove_if(finalTime.begin(), finalTime.end(), isspace), finalTime.end());
		initValue.erase(remove_if(initValue.begin(), initValue.end(), isspace), initValue.end());

		//f = slope*(time - t0) + f0
		//rampstep(time, t0, f0, t1, f1)
		//t0 = initTime
		//f0 = initValue
		//t1 = finalTime
		//f1 = initValue + slope * (finalTime - initTime)
		auto ss = std::stringstream();
		ss << "rampstep(time, " << initTime << ", " << initValue << ", " << finalTime << ", "
			<< initValue << " + " << slope << "*(" << finalTime << " - " << initTime << "))";
		formula = ss.str();
	}
	else {
		assert(false);
	}
}

void MbD::MBDynDrive::createASMT()
{
	assert(false);
}
