// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AsmtIO.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/GeoFeature.h>
#include <App/PropertyStandard.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>

#include "MbDAssembly.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDParameters.h"
#include "MbDPart.h"

namespace
{

constexpr double lengthScale = 0.001;

std::string safeName(const App::DocumentObject* obj)
{
    if (!obj) {
        return {};
    }
    std::string name = obj->getNameInDocument();
    std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

std::string number(double value)
{
    if (std::abs(value) < 1.0e-14) {
        value = 0.0;
    }
    std::ostringstream out;
    out << std::setprecision(15) << value;
    return out.str();
}

class Writer
{
public:
    void line(int level, const std::string& value)
    {
        lines << std::string(static_cast<std::size_t>(level), '\t') << value << '\n';
    }

    void keyValue(int level, const std::string& key, const std::string& value)
    {
        line(level, key);
        line(level + 1, value);
    }

    void vector(int level, const std::vector<double>& values)
    {
        std::ostringstream row;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                row << '\t';
            }
            row << number(values[i]);
        }
        row << '\t';
        line(level, row.str());
    }

    void matrix(int level, const std::array<std::vector<double>, 3>& rows)
    {
        for (const auto& row : rows) {
            vector(level, row);
        }
    }

    std::string text() const
    {
        return lines.str();
    }

private:
    std::ostringstream lines;
};

Base::Placement placementOf(const App::DocumentObject* obj)
{
    if (const auto* geo = dynamic_cast<const App::GeoFeature*>(obj)) {
        return geo->Placement.getValue();
    }
    return Base::Placement();
}

std::vector<double> positionValues(const Base::Placement& placement)
{
    const auto& pos = placement.getPosition();
    return {pos.x * lengthScale, pos.y * lengthScale, pos.z * lengthScale};
}

std::array<std::vector<double>, 3> rotationRows(const Base::Placement& placement)
{
    const auto matrix = placement.toMatrix();
    return {
        {{matrix[0][0], matrix[0][1], matrix[0][2]},
         {matrix[1][0], matrix[1][1], matrix[1][2]},
         {matrix[2][0], matrix[2][1], matrix[2][2]}}
    };
}

void writePlacement(Writer& writer, int level, const Base::Placement& placement)
{
    writer.line(level, "Position3D");
    writer.vector(level + 1, positionValues(placement));
    writer.line(level, "RotationMatrix");
    writer.matrix(level + 1, rotationRows(placement));
}

void writeSpatialItem(Writer& writer, int level, const App::DocumentObject* obj)
{
    writePlacement(writer, level, placementOf(obj));
}

void writeSpatialKinematics(Writer& writer, int level, const App::DocumentObject* obj)
{
    writeSpatialItem(writer, level, obj);
    writer.line(level, "Velocity3D");
    writer.vector(level + 1, {0.0, 0.0, 0.0});
    writer.line(level, "Omega3D");
    writer.vector(level + 1, {0.0, 0.0, 0.0});
}

void writeAssemblyKinematics(Writer& writer, int level)
{
    writePlacement(writer, level, Base::Placement());
    writer.line(level, "Velocity3D");
    writer.vector(level + 1, {0.0, 0.0, 0.0});
    writer.line(level, "Omega3D");
    writer.vector(level + 1, {0.0, 0.0, 0.0});
}

struct MarkerRef
{
    const MbDFEM::MbDMarker* marker {};
    Base::Placement refPointPlacement;
    std::string name;
};

void writeReferences(Writer& writer, int level, const std::vector<MarkerRef>& markers)
{
    writer.line(level, "RefPoints");
    for (const auto& ref : markers) {
        writer.line(level + 1, "RefPoint");
        writePlacement(writer, level + 2, ref.refPointPlacement);
        writer.line(level + 2, "Markers");
        writer.line(level + 3, "Marker");
        writer.keyValue(level + 4, "Name", ref.name);
        writeSpatialItem(writer, level + 4, ref.marker);
    }
    writer.line(level, "RefCurves");
    writer.line(level, "RefSurfaces");
}

void writePrincipalMassMarker(Writer& writer, int level)
{
    writer.line(level, "PrincipalMassMarker");
    writer.keyValue(level + 1, "Name", "MassMarker");
    writer.line(level + 1, "Position3D");
    writer.vector(level + 2, {0.0, 0.0, 0.0});
    writer.line(level + 1, "RotationMatrix");
    writer.matrix(level + 2, {{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}});
    writer.keyValue(level + 1, "Mass", "1");
    writer.line(level + 1, "MomentOfInertias");
    writer.vector(level + 2, {1.0, 1.0, 1.0});
    writer.keyValue(level + 1, "Density", "1");
}

std::vector<App::DocumentObject*> uniqueObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<App::DocumentObject*> result;
    for (auto* object : objects) {
        if (object && std::find(result.begin(), result.end(), object) == result.end()) {
            result.push_back(object);
        }
    }
    return result;
}

std::vector<App::DocumentObject*> folderObjects(App::DocumentObjectGroup* folder)
{
    return folder ? folder->Group.getValues() : std::vector<App::DocumentObject*> {};
}

std::vector<MbDFEM::MbDPart*> partsFromObjects(const std::vector<App::DocumentObject*>& objects)
{
    std::vector<MbDFEM::MbDPart*> parts;
    for (auto* object : objects) {
        if (auto* part = freecad_cast<MbDFEM::MbDPart*>(object)) {
            if (std::find(parts.begin(), parts.end(), part) == parts.end()) {
                parts.push_back(part);
            }
        }
    }
    return parts;
}

std::vector<MbDFEM::MbDPart*> movableParts(MbDFEM::MbDAssembly* assembly)
{
    auto objects = folderObjects(assembly->getPartsFolder());
    if (objects.empty()) {
        objects = assembly->parts.getValues();
    }
    return partsFromObjects(uniqueObjects(objects));
}

std::vector<MbDFEM::MbDPart*> fixedParts(MbDFEM::MbDAssembly* assembly)
{
    auto objects = folderObjects(assembly->getFixedPartsFolder());
    auto propertyObjects = assembly->fixedparts.getValues();
    objects.insert(objects.end(), propertyObjects.begin(), propertyObjects.end());
    return partsFromObjects(uniqueObjects(objects));
}

std::vector<MarkerRef> markerRefs(MbDFEM::MbDPart* part)
{
    std::vector<MarkerRef> refs;
    for (auto* object : part->markers.getValues()) {
        if (auto* marker = freecad_cast<MbDFEM::MbDMarker*>(object)) {
            refs.push_back({marker, Base::Placement(), safeName(marker)});
        }
    }
    return refs;
}

void writePart(Writer& writer, int level, MbDFEM::MbDPart* part)
{
    writer.line(level, "Part");
    writer.keyValue(level + 1, "Name", safeName(part));
    writeSpatialKinematics(writer, level + 1, part);
    writer.line(level + 1, "FeatureOrder");
    writePrincipalMassMarker(writer, level + 1);
    writeReferences(writer, level + 1, markerRefs(part));
}

MbDFEM::MbDPart* partContainingMarker(
    const std::vector<MbDFEM::MbDPart*>& parts,
    App::DocumentObject* marker
)
{
    for (auto* part : parts) {
        const auto markers = part->markers.getValues();
        if (std::find(markers.begin(), markers.end(), marker) != markers.end()) {
            return part;
        }
    }
    return nullptr;
}

std::string jointTypeName(const MbDFEM::MbDJoint* joint)
{
    static const std::map<std::string, std::string> mapping {
        {"Fixed", "FixedJoint"},
        {"Revolute", "RevoluteJoint"},
        {"Prismatic", "TranslationalJoint"},
        {"Translational", "TranslationalJoint"},
        {"Cylindrical", "CylindricalJoint"},
        {"Spherical", "SphericalJoint"},
        {"Universal", "UniversalJoint"},
    };

    const auto value = joint->jointType.getValueAsString();
    const auto it = mapping.find(value ? value : "");
    return it == mapping.end() ? "FixedJoint" : it->second;
}

std::string markerPath(
    const std::string& assemblyName,
    const MbDFEM::MbDPart* part,
    const MbDFEM::MbDMarker* marker
)
{
    return "/" + assemblyName + "/" + safeName(part) + "/" + safeName(marker);
}

std::string groundMarkerPath(
    const std::string& assemblyName,
    const MbDFEM::MbDPart* part,
    const MbDFEM::MbDMarker* marker
)
{
    return "/" + assemblyName + "/Ground_" + safeName(part) + "_" + safeName(marker);
}

void writeJoint(
    Writer& writer,
    int level,
    const std::string& jointType,
    const std::string& name,
    const std::string& markerI,
    const std::string& markerJ
)
{
    writer.line(level, jointType);
    writer.keyValue(level + 1, "Name", name);
    writer.keyValue(level + 1, "MarkerI", markerI);
    writer.keyValue(level + 1, "MarkerJ", markerJ);
}

void writeConstraintSets(
    Writer& writer,
    MbDFEM::MbDAssembly* assembly,
    const std::string& assemblyName,
    const std::vector<MbDFEM::MbDPart*>& movable,
    const std::vector<MbDFEM::MbDPart*>& fixed
)
{
    std::vector<MbDFEM::MbDPart*> allParts = movable;
    allParts.insert(allParts.end(), fixed.begin(), fixed.end());

    writer.line(1, "ConstraintSets");
    writer.line(2, "Joints");
    for (auto* object : assembly->joints.getValues()) {
        auto* joint = freecad_cast<MbDFEM::MbDJoint*>(object);
        if (!joint) {
            continue;
        }

        auto* markerI = freecad_cast<MbDFEM::MbDMarker*>(joint->markerI.getValue());
        auto* markerJ = freecad_cast<MbDFEM::MbDMarker*>(joint->markerJ.getValue());
        auto* partI = partContainingMarker(allParts, markerI);
        auto* partJ = partContainingMarker(allParts, markerJ);
        if (!markerI || !markerJ || !partI || !partJ) {
            continue;
        }

        const bool fixedI = std::find(fixed.begin(), fixed.end(), partI) != fixed.end();
        const bool fixedJ = std::find(fixed.begin(), fixed.end(), partJ) != fixed.end();
        writeJoint(
            writer,
            3,
            jointTypeName(joint),
            safeName(joint),
            fixedI ? groundMarkerPath(assemblyName, partI, markerI)
                   : markerPath(assemblyName, partI, markerI),
            fixedJ ? groundMarkerPath(assemblyName, partJ, markerJ)
                   : markerPath(assemblyName, partJ, markerJ)
        );
    }
    writer.line(2, "Motions");
    writer.line(2, "GeneralConstraintSets");
}

void writeGravity(Writer& writer, MbDFEM::MbDAssembly* assembly)
{
    writer.line(1, "ConstantGravity");
    if (auto* gravity = assembly->getGravity()) {
        const auto value = gravity->gravity.getValue();
        writer.vector(2, {value.x, value.y, value.z});
    }
    else {
        writer.vector(2, {0.0, 0.0, -9.81});
    }
}

void writeSimulationParameters(Writer& writer, MbDFEM::MbDAssembly* assembly)
{
    auto* parameters = assembly->getSimulationParameters();
    const double start = parameters ? parameters->startTime.getValue() : 0.0;
    const double end = parameters ? parameters->endTime.getValue() : 1.0;
    const double step = parameters ? parameters->outputInterval.getValue() : 0.01;
    const double maxStepSize = parameters ? parameters->maxStepSize.getValue() : 1.0;
    const double minStepSize = parameters ? parameters->minStepSize.getValue() : 1.0e-09;
    const int digits = parameters ? parameters->significantDigits.getValue() : 4;

    writer.line(1, "SimulationParameters");
    writer.keyValue(2, "tstart", number(start));
    writer.keyValue(2, "tend", number(end));
    writer.keyValue(2, "hmin", number(minStepSize));
    writer.keyValue(2, "hmax", number(std::max(maxStepSize, step)));
    writer.keyValue(2, "hout", number(step));
    writer.keyValue(2, "errorTol", number(std::pow(10.0, -(2 * digits))));
}

void writeAnimationParameters(Writer& writer, MbDFEM::MbDAssembly* assembly)
{
    auto* parameters = assembly->getAnimationParameters();
    const int frameRate = parameters ? parameters->frameRate.getValue() : 30;

    writer.line(1, "AnimationParameters");
    writer.keyValue(2, "nframe", "1000000");
    writer.keyValue(2, "icurrent", "1");
    writer.keyValue(2, "istart", "1");
    writer.keyValue(2, "iend", "1000000");
    writer.keyValue(2, "isForward", "true");
    writer.keyValue(2, "framesPerSecond", std::to_string(frameRate));
}

std::vector<std::string> fields(const std::string& line)
{
    std::istringstream stream(line);
    std::vector<std::string> result;
    std::string value;
    while (stream >> value) {
        result.push_back(value);
    }
    return result;
}

double seriesValue(const std::string& value)
{
    if (value == "Input") {
        return -std::numeric_limits<double>::max();
    }
    try {
        return std::stod(value);
    }
    catch (const std::exception&) {
        throw Base::ValueError("Invalid ASMT numeric value: " + value);
    }
}

std::vector<double> seriesValues(const std::vector<std::string>& tokens)
{
    std::vector<double> values;
    values.reserve(tokens.size() > 0 ? tokens.size() - 1 : 0);
    for (std::size_t i = 1; i < tokens.size(); ++i) {
        values.push_back(seriesValue(tokens[i]));
    }
    return values;
}

std::string objectNameFromPath(const std::string& path)
{
    auto end = path.find_last_not_of('/');
    if (end == std::string::npos) {
        return {};
    }
    auto start = path.find_last_of('/', end);
    return path.substr(start == std::string::npos ? 0 : start + 1, end - start);
}

App::DocumentObject* targetObject(
    MbDFEM::MbDAssembly* assembly,
    const std::string& seriesType,
    const std::string& path
)
{
    if (seriesType == "AssemblySeries") {
        return assembly;
    }
    if (seriesType != "PartSeries" && seriesType != "FixedJointSeries"
        && seriesType != "RevoluteJointSeries") {
        return nullptr;
    }

    auto* document = assembly->getDocument();
    const auto name = objectNameFromPath(path);
    auto* object = document && !name.empty() ? document->getObject(name.c_str()) : nullptr;
    if (seriesType == "PartSeries") {
        return object && object->isDerivedFrom(MbDFEM::MbDPart::getClassTypeId()) ? object : nullptr;
    }
    return object && object->isDerivedFrom(MbDFEM::MbDJoint::getClassTypeId()) ? object : nullptr;
}

App::PropertyFloatList* resultProperty(App::DocumentObject* object, const std::string& keyword)
{
    static const std::map<std::string, const char*> properties {
        {"X", "xs"},          {"Y", "ys"},          {"Z", "zs"},         {"Bryantx", "bryxs"},
        {"Bryanty", "bryys"}, {"Bryantz", "bryzs"}, {"VX", "vxs"},       {"VY", "vys"},
        {"VZ", "vzs"},        {"OmegaX", "omexs"},  {"OmegaY", "omeys"}, {"OmegaZ", "omezs"},
        {"AX", "axs"},        {"AY", "ays"},        {"AZ", "azs"},       {"AlphaX", "alpxs"},
        {"AlphaY", "alpys"},  {"AlphaZ", "alpzs"},  {"FXonI", "fxs"},    {"FYonI", "fys"},
        {"FZonI", "fzs"},     {"TXonI", "txs"},     {"TYonI", "tys"},    {"TZonI", "tzs"},
    };

    const auto it = properties.find(keyword);
    if (it == properties.end() || !object) {
        return nullptr;
    }
    return freecad_cast<App::PropertyFloatList*>(object->getPropertyByName(it->second));
}

struct SeriesAssignment
{
    App::DocumentObject* object {};
    App::PropertyFloatList* property {};
    std::string propertyName;
    std::vector<double> values;
};

void validateSeriesLengths(
    const std::vector<double>* timeValues,
    const std::vector<SeriesAssignment>& assignments
)
{
    std::size_t expected = timeValues ? timeValues->size() : 0;
    if (!timeValues && !assignments.empty()) {
        expected = assignments.front().values.size();
    }

    for (const auto& assignment : assignments) {
        if (assignment.values.size() != expected) {
            std::ostringstream message;
            message << safeName(assignment.object) << "." << assignment.propertyName << " has "
                    << assignment.values.size() << " values; expected " << expected;
            throw Base::ValueError(message.str());
        }
    }
}

}  // namespace

std::string MbDFEM::exportAssemblyAsmt(MbDAssembly* assembly, const std::string& filename)
{
    if (!assembly) {
        throw Base::ValueError("exportAssemblyAsmt expects an MbDFEM::MbDAssembly");
    }

    const std::filesystem::path path = std::filesystem::u8path(filename);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    const std::string assemblyName = safeName(assembly);
    const auto fixed = fixedParts(assembly);
    auto movable = movableParts(assembly);
    movable.erase(
        std::remove_if(
            movable.begin(),
            movable.end(),
            [&fixed](MbDPart* part) {
                return std::find(fixed.begin(), fixed.end(), part) != fixed.end();
            }
        ),
        movable.end()
    );

    Writer writer;
    writer.line(0, "FreeCADMbD");
    writer.line(0, "Assembly");
    writer.keyValue(1, "Notes", "(Text string: '' runs: (Core.RunArray new))");
    writer.keyValue(1, "Name", assemblyName);
    writeAssemblyKinematics(writer, 1);

    std::vector<MarkerRef> groundMarkers;
    for (auto* part : fixed) {
        for (auto* object : part->markers.getValues()) {
            if (auto* marker = freecad_cast<MbDMarker*>(object)) {
                groundMarkers.push_back(
                    {marker, placementOf(part), "Ground_" + safeName(part) + "_" + safeName(marker)}
                );
            }
        }
    }
    writeReferences(writer, 1, groundMarkers);

    writer.line(1, "Parts");
    for (auto* part : movable) {
        writePart(writer, 2, part);
    }

    writer.line(1, "KinematicIJs");
    writeConstraintSets(writer, assembly, assemblyName, movable, fixed);
    writer.line(1, "ForceTorques");
    writeGravity(writer, assembly);
    writeSimulationParameters(writer, assembly);
    writeAnimationParameters(writer, assembly);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw Base::FileException("Cannot open ASMT file for writing", filename.c_str());
    }
    file << writer.text();
    if (!file) {
        throw Base::FileException("Cannot write ASMT file", filename.c_str());
    }
    return path.string();
}

std::vector<App::DocumentObject*> MbDFEM::importSolvedAsmt(
    MbDAssembly* assembly,
    const std::string& filename
)
{
    if (!assembly) {
        throw Base::ValueError("importSolvedAsmt expects an MbDFEM::MbDAssembly");
    }

    std::ifstream file(std::filesystem::u8path(filename), std::ios::binary);
    if (!file) {
        throw Base::FileException("Cannot open ASMT file for reading", filename.c_str());
    }

    App::DocumentObject* currentObject = nullptr;
    std::vector<App::DocumentObject*> importedObjects;
    std::vector<double> timeValues;
    bool hasTimeValues = false;
    std::vector<SeriesAssignment> assignments;

    std::string line;
    while (std::getline(file, line)) {
        const auto tokens = fields(line);
        if (tokens.empty()) {
            continue;
        }

        const auto& keyword = tokens.front();
        if (keyword == "Time") {
            timeValues = seriesValues(tokens);
            hasTimeValues = true;
            continue;
        }

        if (keyword == "AssemblySeries" || keyword == "PartSeries" || keyword == "FixedJointSeries"
            || keyword == "RevoluteJointSeries") {
            currentObject = targetObject(assembly, keyword, tokens.size() > 1 ? tokens[1] : "");
            if (currentObject
                && std::find(importedObjects.begin(), importedObjects.end(), currentObject)
                    == importedObjects.end()) {
                importedObjects.push_back(currentObject);
            }
            continue;
        }

        auto* property = resultProperty(currentObject, keyword);
        if (property) {
            assignments.push_back({currentObject, property, property->getName(), seriesValues(tokens)});
        }
    }

    validateSeriesLengths(hasTimeValues ? &timeValues : nullptr, assignments);

    if (hasTimeValues) {
        assembly->times.setValues(timeValues);
    }
    for (const auto& assignment : assignments) {
        assignment.property->setValues(assignment.values);
    }

    if (auto* document = assembly->getDocument()) {
        document->recompute();
    }
    return importedObjects;
}
