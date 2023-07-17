#include "ASMTRefPoint.h"
#include "CREATE.h"

using namespace MbD;

void MbD::ASMTRefPoint::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	while (!lines.empty()) {
		if (lines[0] == (leadingTabs + "Position3D")) {
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
		else if (lines[0] == (leadingTabs + "Markers")) {
			lines.erase(lines.begin());
			markers = std::make_shared<std::vector<std::shared_ptr<ASMTMarker>>>();
			auto it = std::find(lines.begin(), lines.end(), (leadingTabs.substr(0, leadingTabs.size() - 1) + "RefPoint"));
			std::vector<std::string> markersLines(lines.begin(), it);
			while (!markersLines.empty()) {
				if (markersLines[0] == (leadingTabs + "\tMarker")) {
					markersLines.erase(markersLines.begin());
					auto marker = CREATE<ASMTMarker>::With();
					marker->parseASMT(markersLines);
					markers->push_back(marker);
					marker->owner = this;
				}
				else {
					return;
				}
			}
			lines.erase(lines.begin(), it);
		}
		else {
			return;
		}
	}
}
