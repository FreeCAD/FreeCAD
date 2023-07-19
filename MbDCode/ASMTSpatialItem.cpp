#include "ASMTSpatialItem.h"

using namespace MbD;

void MbD::ASMTSpatialItem::readPosition3D(std::vector<std::string>& lines)
{
	assert(lines[0].find("Position3D") != std::string::npos);
	lines.erase(lines.begin());
	std::istringstream iss(lines[0]);
	position3D = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		position3D->push_back(d);
	}
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialItem::readRotationMatrix(std::vector<std::string>& lines)
{
	assert(lines[0].find("RotationMatrix") != std::string::npos);
	lines.erase(lines.begin());
	rotationMatrix = std::make_shared<FullMatrix<double>>(3, 0);
	for (int i = 0; i < 3; i++)
	{
		auto& row = rotationMatrix->at(i);
		std::istringstream iss(lines[0]);
		double d;
		while (iss >> d) {
			row->push_back(d);
		}
		lines.erase(lines.begin());
	}
}

void MbD::ASMTSpatialItem::readVelocity3D(std::vector<std::string>& lines)
{
	assert(lines[0].find("Velocity3D") != std::string::npos);
	lines.erase(lines.begin());
	std::istringstream iss(lines[0]);
	velocity3D = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		velocity3D->push_back(d);
	}
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialItem::readOmega3D(std::vector<std::string>& lines)
{
	assert(lines[0].find("Omega3D") != std::string::npos);
	lines.erase(lines.begin());
	std::istringstream iss(lines[0]);
	omega3D = std::make_shared<FullColumn<double>>();
	double d;
	while (iss >> d) {
		omega3D->push_back(d);
	}
	lines.erase(lines.begin());
}
