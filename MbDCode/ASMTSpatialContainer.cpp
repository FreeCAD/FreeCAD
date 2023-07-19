#include "ASMTSpatialContainer.h"

void MbD::ASMTSpatialContainer::readRefPoints(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefPoints") != std::string::npos);
	lines.erase(lines.begin());
	refPoints = std::make_shared<std::vector<std::shared_ptr<ASMTRefPoint>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefCurves") != std::string::npos;
		});
	std::vector<std::string> refPointsLines(lines.begin(), it);
	while (!refPointsLines.empty()) {
		readRefPoint(refPointsLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefPoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefPoint") != std::string::npos);
	lines.erase(lines.begin());
	auto refPoint = CREATE<ASMTRefPoint>::With();
	refPoint->parseASMT(lines);
	refPoints->push_back(refPoint);
	refPoint->owner = this;
}

void MbD::ASMTSpatialContainer::readRefCurves(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefCurves") != std::string::npos);
	lines.erase(lines.begin());
	refCurves = std::make_shared<std::vector<std::shared_ptr<ASMTRefCurve>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("RefSurfaces") != std::string::npos;
		});
	std::vector<std::string> refCurvesLines(lines.begin(), it);
	while (!refCurvesLines.empty()) {
		readRefCurve(refCurvesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefCurve(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::ASMTSpatialContainer::readRefSurfaces(std::vector<std::string>& lines)
{
	assert(lines[0].find("RefSurfaces") != std::string::npos);
	lines.erase(lines.begin());
	refSurfaces = std::make_shared<std::vector<std::shared_ptr<ASMTRefSurface>>>();
	auto it = std::find_if(lines.begin(), lines.end(), [](const std::string& s) {
		return s.find("Part") != std::string::npos;
		});
	std::vector<std::string> refSurfacesLines(lines.begin(), it);
	while (!refSurfacesLines.empty()) {
		readRefSurface(refSurfacesLines);
	}
	lines.erase(lines.begin(), it);
}

void MbD::ASMTSpatialContainer::readRefSurface(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::ASMTSpatialContainer::readXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "X", xs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Y", ys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Z", zs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantxs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryantx", bryxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantys(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryanty", bryys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readBryantzs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "Bryantz", bryzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VX", vxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VY", vys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readVZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "VZ", vzs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaX", omexs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaY", omeys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readOmegaZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "OmegaZ", omezs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AX", axs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AY", ays);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AZ", azs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaXs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaX", alpxs);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaYs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaY", alpys);
	lines.erase(lines.begin());
}

void MbD::ASMTSpatialContainer::readAlphaZs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "AlphaZ", alpzs);
	lines.erase(lines.begin());
}
