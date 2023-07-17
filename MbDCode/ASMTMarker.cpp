#include "ASMTMarker.h"

using namespace MbD;

void MbD::ASMTMarker::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	while (!lines.empty()) {
		if (lines[0] == (leadingTabs + "Name")) {
			lines.erase(lines.begin());
			name = lines[0];
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "Position3D")) {
			lines.erase(lines.begin());
			std::istringstream iss(lines[0]);
			position3D = std::make_shared<FullColumn<double>>();
			double d;
			while (iss >> d) {
				position3D->push_back(d);
			}
			lines.erase(lines.begin());
		}
		else if (lines[0] == (leadingTabs + "RotationMatrix")) {
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
		else {
			assert(false);
		}
	}
}
