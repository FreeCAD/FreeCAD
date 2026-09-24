// SPDX-License-Identifier: LGPL-2.1-or-later
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/VectorPy.h>
#include "Annotation.h"
#include "HatchPattern.h"

using namespace Sketcher;
TYPESYSTEM_SOURCE(Sketcher::PropertyAnnotationList, App::Property)

namespace
{
const char* kindName(Annotation::Kind kind)
{
    switch (kind) {
        case Annotation::Kind::Text:
            return "Text";
        case Annotation::Kind::Hatch:
            return "Hatch";
        case Annotation::Kind::Leader:
            return "Leader";
    }
    throw Base::ValueError("Unknown annotation type");
}
Annotation::Kind parseKind(const std::string& name)
{
    if (name == "Text") {
        return Annotation::Kind::Text;
    }
    if (name == "Hatch") {
        return Annotation::Kind::Hatch;
    }
    if (name == "Leader") {
        return Annotation::Kind::Leader;
    }
    throw Base::ValueError("Annotation Type must be Text, Hatch, or Leader");
}
double numberFromPython(PyObject* value)
{
    const double result = PyFloat_AsDouble(value);
    if (PyErr_Occurred()) {
        throw Py::Exception();
    }
    return result;
}
Base::Vector3d vectorFromPython(PyObject* value)
{
    if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        return Base::Vector3d(
            numberFromPython(PyTuple_GetItem(value, 0)),
            numberFromPython(PyTuple_GetItem(value, 1)),
            numberFromPython(PyTuple_GetItem(value, 2))
        );
    }
    if (!PyObject_TypeCheck(value, &Base::VectorPy::Type)) {
        throw Base::TypeError("Annotation points must be App.Vector values");
    }
    return *static_cast<Base::VectorPy*>(value)->getVectorPtr();
}
bool planar(const Base::Vector3d& p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && std::abs(p.z) <= 1e-9;
}
}  // namespace

namespace
{
constexpr std::string_view subNamePrefix = "Annotation";
bool knownArrowStyle(const std::string& style)
{
    const auto& styles = Annotation::arrowStyles();
    return std::find(styles.begin(), styles.end(), style) != styles.end();
}
}  // namespace

const std::vector<std::string>& Annotation::arrowStyles()
{
    static const std::vector<std::string> styles {
        "Filled arrow",
        "Open arrow",
        "Tick",
        "Dot",
        "Open circle",
        "Fork",
        "Filled triangle",
        "None",
    };
    return styles;
}

long Annotation::idFromSubName(std::string_view name)
{
    if (!name.starts_with(subNamePrefix)) {
        return 0;
    }
    const auto digits = name.substr(subNamePrefix.size());
    long id = 0;
    const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), id);
    if (parsed.ec != std::errc() || parsed.ptr != digits.data() + digits.size() || id <= 0) {
        return 0;
    }
    return id;
}

std::string Annotation::subName(long id)
{
    return std::string(subNamePrefix) + std::to_string(id);
}

PyObject* Annotation::toPython() const
{
    Py::Dict result;
    result.setItem("Id", Py::Long(id));
    result.setItem("Type", Py::String(kindName(kind)));
    result.setItem("Label", Py::String(label));
    result.setItem("Construction", Py::Boolean(construction));
    result.setItem("Position", Py::Object(new Base::VectorPy(position), true));
    result.setItem("Rotation", Py::Float(rotation));
    result.setItem("Html", Py::String(html));
    result.setItem("TextSize", Py::Float(textSize));
    result.setItem("TextWidth", Py::Float(textWidth));
    Py::List refs;
    for (long id : boundary) {
        refs.append(Py::Long(id));
    }
    result.setItem("Boundary", refs);
    result.setItem("Spacing", Py::Float(spacing));
    result.setItem("Pattern", Py::String(pattern));
    Py::List vertices;
    for (const auto& p : points) {
        vertices.append(Py::Object(new Base::VectorPy(p), true));
    }
    result.setItem("Points", vertices);
    result.setItem("ArrowSize", Py::Float(arrowSize));
    result.setItem("ArrowStyle", Py::String(arrowStyle));
    return Py::new_reference_to(result);
}

Annotation Annotation::fromPython(PyObject* values, Annotation a)
{
    if (!PyDict_Check(values)) {
        throw Base::TypeError("Annotation data must be a dictionary");
    }
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(values, &pos, &key, &value)) {
        const std::string name = Py::String(key).as_string();
        if (name == "Id") {
            // A new annotation gets its ID from the sketch, so a copied snapshot may carry
            // any ID; an existing one keeps its own.
            if (a.id != 0 && Py::Long(value).as_long() != a.id) {
                throw Base::ValueError("Annotation IDs are read-only");
            }
        }
        else if (name == "Type") {
            a.kind = parseKind(Py::String(value).as_string());
        }
        else if (name == "Label") {
            a.label = Py::String(value).as_string();
        }
        else if (name == "Construction") {
            a.construction = Py::Boolean(value).isTrue();
        }
        else if (name == "Position") {
            a.position = vectorFromPython(value);
        }
        else if (name == "Rotation") {
            a.rotation = numberFromPython(value);
        }
        else if (name == "Html") {
            a.html = Py::String(value).as_string();
        }
        else if (name == "TextSize") {
            a.textSize = numberFromPython(value);
        }
        else if (name == "TextWidth") {
            a.textWidth = numberFromPython(value);
        }
        else if (name == "Spacing") {
            a.spacing = numberFromPython(value);
        }
        else if (name == "Pattern") {
            a.pattern = Py::String(value).as_string();
        }
        else if (name == "ArrowSize") {
            a.arrowSize = numberFromPython(value);
        }
        else if (name == "ArrowStyle") {
            a.arrowStyle = Py::String(value).as_string();
        }
        else if (name == "Boundary") {
            a.boundary.clear();
            for (auto item : Py::Sequence(value)) {
                a.boundary.push_back(Py::Long(item).as_long());
            }
        }
        else if (name == "Points") {
            a.points.clear();
            for (auto item : Py::Sequence(value)) {
                a.points.push_back(vectorFromPython(item.ptr()));
            }
        }
        else {
            throw Base::ValueError("Unknown annotation field: " + name);
        }
    }
    a.validate();
    return a;
}

void Annotation::validate() const
{
    if (!planar(position) || !std::isfinite(rotation) || !std::isfinite(textSize)
        || textSize <= 0 || !std::isfinite(textWidth) || textWidth < 0 || !std::isfinite(spacing)
        || spacing < 0.01 || !std::isfinite(arrowSize) || arrowSize <= 0) {
        throw Base::ValueError("Invalid annotation coordinates or size");
    }
    // Control characters cannot be written to the document XML and would break loading it.
    auto printable = [](const std::string& text) {
        return std::none_of(text.begin(), text.end(), [](char c) {
            const auto code = static_cast<unsigned char>(c);
            return code < 0x20 && c != '\t' && c != '\n' && c != '\r';
        });
    };
    if (!printable(label) || !printable(html)) {
        throw Base::ValueError("Annotation text contains control characters");
    }
    if (kind == Kind::Text && html.empty()) {
        throw Base::ValueError("Annotation text cannot be empty");
    }
    if (kind == Kind::Leader) {
        if (!knownArrowStyle(arrowStyle)) {
            throw Base::ValueError("Unknown arrowhead style: " + arrowStyle);
        }
        if (points.size() < 2) {
            throw Base::ValueError("A leader needs at least two points");
        }
        for (size_t i = 0; i < points.size(); ++i) {
            if (!planar(points[i]) || (i && (points[i] - points[i - 1]).Length() < 1e-9)) {
                throw Base::ValueError(
                    "Leader points must be distinct consecutive points in the sketch plane"
                );
            }
        }
    }
    if (kind == Kind::Hatch) {
        if (!findHatchPattern(pattern)) {
            throw Base::ValueError("Unknown hatch pattern: " + pattern);
        }
        if (boundary.empty()) {
            throw Base::ValueError("A hatch needs boundary geometry");
        }
        std::set<long> unique;
        for (long id : boundary) {
            if (id <= 0 || !unique.insert(id).second) {
                throw Base::ValueError("Invalid hatch boundary IDs");
            }
        }
    }
}

void PropertyAnnotationList::setValues(std::vector<Annotation> annotations)
{
    aboutToSetValue();
    values = std::move(annotations);
    for (const auto& annotation : values) {
        highestId = std::max(highestId, annotation.id);
    }
    hasSetValue();
}
PyObject* PropertyAnnotationList::getPyObject()
{
    Py::List result;
    for (const auto& a : values) {
        result.append(Py::Object(a.toPython(), true));
    }
    return Py::new_reference_to(result);
}
void PropertyAnnotationList::setPyObject(PyObject*)
{
    throw Base::AttributeError("Use addAnnotation, updateAnnotation, or delAnnotations");
}
App::Property* PropertyAnnotationList::Copy() const
{
    auto* copy = new PropertyAnnotationList;
    copy->values = values;
    copy->highestId = highestId;
    return copy;
}
void PropertyAnnotationList::Paste(const App::Property& from)
{
    const auto& source = static_cast<const PropertyAnnotationList&>(from);
    highestId = std::max(highestId, source.highestId);
    setValues(source.values);
}
unsigned int PropertyAnnotationList::getMemSize() const
{
    size_t size = sizeof(*this);
    for (const auto& a : values) {
        size += sizeof(a) + a.html.size() + a.label.size() + a.pattern.size() + a.arrowStyle.size() + a.boundary.size() * sizeof(long)
            + a.points.size() * sizeof(Base::Vector3d);
    }
    return static_cast<unsigned int>(size);
}
void PropertyAnnotationList::Save(Base::Writer& writer) const
{
    auto& out = writer.Stream();
    out << writer.ind() << "<Annotations version=\"1\" count=\"" << values.size()
        << "\" highestId=\"" << highestId << "\">\n";
    for (const auto& a : values) {
        out << writer.ind() << "<Annotation id=\"" << a.id << "\" type=\"" << kindName(a.kind)
            << "\" label=\"" << encodeAttribute(a.label)
            << "\" construction=\"" << a.construction << "\" x=\"" << a.position.x << "\" y=\""
            << a.position.y << "\" rotation=\"" << a.rotation << "\" html=\""
            << encodeAttribute(a.html) << "\" textSize=\"" << a.textSize << "\" textWidth=\""
            << a.textWidth << "\" spacing=\"" << a.spacing << "\" pattern=\"" << encodeAttribute(a.pattern)
            << "\" arrowSize=\"" << a.arrowSize << "\" arrowStyle=\"" << encodeAttribute(a.arrowStyle) << "\" boundaries=\"" << a.boundary.size()
            << "\" points=\"" << a.points.size() << "\">\n";
        for (long id : a.boundary) {
            out << "<Boundary id=\"" << id << "\"/>\n";
        }
        for (const auto& p : a.points) {
            out << "<Point x=\"" << p.x << "\" y=\"" << p.y << "\"/>\n";
        }
        out << "</Annotation>\n";
    }
    out << writer.ind() << "</Annotations>\n";
}
void PropertyAnnotationList::Restore(Base::XMLReader& reader)
{
    reader.readElement("Annotations");
    if (reader.getAttribute<int>("version") != 1) {
        throw Base::ValueError("Unsupported annotation format version");
    }
    if (reader.hasAttribute("highestId")) {
        highestId = std::max(highestId, reader.getAttribute<long>("highestId"));
    }
    const int count = reader.getAttribute<int>("count");
    if (count < 0 || count > 1000000) {
        throw Base::ValueError("Invalid annotation count");
    }
    std::vector<Annotation> restored;
    std::set<long> ids;
    for (int i = 0; i < count; ++i) {
        reader.readElement("Annotation");
        Annotation a;
        a.id = reader.getAttribute<long>("id");
        // A type from a newer version is skipped, not allowed to drop every annotation.
        const std::string type = reader.getAttribute<const char*>("type");
        bool knownType = true;
        try {
            a.kind = parseKind(type);
        }
        catch (const Base::Exception&) {
            knownType = false;
        }
        a.label = reader.getAttribute<const char*>("label");
        a.construction = reader.getAttribute<int>("construction") != 0;
        a.position
            = Base::Vector3d(reader.getAttribute<double>("x"), reader.getAttribute<double>("y"), 0);
        a.rotation = reader.getAttribute<double>("rotation");
        a.html = reader.getAttribute<const char*>("html");
        a.textSize = reader.getAttribute<double>("textSize");
        a.textWidth = reader.getAttribute<double>("textWidth");
        a.spacing = reader.getAttribute<double>("spacing");
        if (reader.hasAttribute("pattern")) {
            a.pattern = reader.getAttribute<const char*>("pattern");
        }
        else if (a.kind == Annotation::Kind::Hatch) {
            // Before named patterns a hatch was lines at Rotation, optionally crossed at
            // Rotation + 90. The 45° ANSI families reproduce both exactly.
            a.pattern = reader.getAttribute<int>("crosshatch") != 0 ? "ANSI37" : "ANSI31";
            a.rotation -= 45;
        }
        if (!findHatchPattern(a.pattern)) {
            // A pattern from a newer version must not make the whole document unreadable.
            a.pattern = defaultHatchPattern;
        }
        a.arrowSize = reader.getAttribute<double>("arrowSize");
        if (reader.hasAttribute("arrowStyle")) {
            a.arrowStyle = reader.getAttribute<const char*>("arrowStyle");
        }
        if (!knownArrowStyle(a.arrowStyle)) {
            a.arrowStyle = "Open arrow";
        }
        const int boundaries = reader.getAttribute<int>("boundaries");
        const int points = reader.getAttribute<int>("points");
        if (boundaries < 0 || points < 0 || boundaries > 1000000 || points > 1000000) {
            throw Base::ValueError("Invalid annotation payload size");
        }
        for (int j = 0; j < boundaries; ++j) {
            reader.readElement("Boundary");
            a.boundary.push_back(reader.getAttribute<long>("id"));
        }
        for (int j = 0; j < points; ++j) {
            reader.readElement("Point");
            a.points.emplace_back(reader.getAttribute<double>("x"), reader.getAttribute<double>("y"), 0);
        }
        reader.readEndElement("Annotation");
        if (!knownType) {
            Base::Console().warning(
                "Skipped sketch annotation %ld of unknown type '%s'\n",
                a.id,
                type.c_str()
            );
            continue;
        }
        try {
            a.validate();
        }
        catch (const Base::Exception& error) {
            Base::Console().warning("Skipped invalid sketch annotation %ld: %s\n", a.id, error.what());
            continue;
        }
        if (a.id <= 0 || !ids.insert(a.id).second) {
            Base::Console().warning("Skipped sketch annotation with invalid ID %ld\n", a.id);
            continue;
        }
        restored.push_back(std::move(a));
    }
    reader.readEndElement("Annotations");
    setValues(std::move(restored));
}
