#include "ASMTRefPoint.h"
#include "CREATE.h"

using namespace MbD;

void MbD::ASMTRefPoint::parseASMT(std::vector<std::string>& lines)
{
	readPosition3D(lines);
	readRotationMatrix(lines);
	readMarkers(lines);
}

void MbD::ASMTRefPoint::readMarkers(std::vector<std::string>& lines)
{
	assert(lines[0].find("Markers") != std::string::npos);
	lines.erase(lines.begin());
	markers = std::make_shared<std::vector<std::shared_ptr<ASMTMarker>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefPoint") != std::string::npos;
		});
	std::vector<std::string> markersLines(lines.begin(), it);
	while (!markersLines.empty()) {
		readMarker(markersLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTRefPoint::readMarker(std::vector<std::string>& lines)
{
	assert(lines[0].find("Marker") != std::string::npos);
	lines.erase(lines.begin());
	auto marker = CREATE<ASMTMarker>::With();
	marker->parseASMT(lines);
	markers->push_back(marker);
	marker->owner = this;
}
