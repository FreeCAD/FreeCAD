// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_map>

#include <Base/Exception.h>

#include "ToolpathStore.h"


namespace Path::ToolpathStore
{

const char* const FileExtension = ".ncb";

namespace
{

constexpr std::string_view Magic = "FCTP";
constexpr double Scale = 1.0 / Resolution;
// A value below this limit, multiplied by Scale, fits a 64-bit integer with margin.
constexpr double QuantLimit = 1e12;

constexpr std::uint8_t ShapeHasToolAxis = 0x01;

enum class Encoding : std::uint8_t
{
    QuantDelta = 0,
    RawDouble = 1
};

[[noreturn]] void fail(const char* what)
{
    throw Base::BadFormatError(std::string("Binary toolpath store: ") + what);
}

// ---------------------------------------------------------------------------------------------
// Writer

class Writer
{
public:
    explicit Writer(std::size_t reserve)
    {
        out.reserve(reserve);
    }

    void byte(std::uint8_t v)
    {
        out.push_back(static_cast<char>(v));
    }

    void bytes(std::string_view v)
    {
        out.append(v);
    }

    void varint(std::uint64_t v)
    {
        while (v >= 0x80) {
            out.push_back(static_cast<char>(v | 0x80));
            v >>= 7;
        }
        out.push_back(static_cast<char>(v));
    }

    void zigzag(std::int64_t v)
    {
        varint((static_cast<std::uint64_t>(v) << 1) ^ static_cast<std::uint64_t>(v >> 63));
    }

    void real(double v)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &v, sizeof(bits));
        for (int i = 0; i < 8; ++i) {
            out.push_back(static_cast<char>(bits >> (8 * i)));
        }
    }

    void column(const std::vector<double>& values)
    {
        varint(values.size());
        bool quant = true;
        for (double v : values) {
            if (!std::isfinite(v) || std::fabs(v) >= QuantLimit) {
                quant = false;
                break;
            }
        }
        if (!quant) {
            byte(static_cast<std::uint8_t>(Encoding::RawDouble));
            for (double v : values) {
                real(v);
            }
            return;
        }
        byte(static_cast<std::uint8_t>(Encoding::QuantDelta));
        std::int64_t prev = 0;
        for (double v : values) {
            const std::int64_t q = std::llround(v * Scale);
            zigzag(q - prev);
            prev = q;
        }
    }

    std::string out;
};

class StringTable
{
public:
    std::uint32_t id(const std::string& s)
    {
        const auto [it, added] = ids.try_emplace(s, static_cast<std::uint32_t>(items.size()));
        if (added) {
            items.push_back(&it->first);
        }
        return it->second;
    }

    void write(Writer& w) const
    {
        w.varint(items.size());
        for (const std::string* s : items) {
            w.varint(s->size());
            w.bytes(*s);
        }
    }

private:
    std::unordered_map<std::string, std::uint32_t> ids;
    std::vector<const std::string*> items;
};

struct Shape
{
    std::uint32_t name = 0;
    std::uint8_t flags = 0;
    std::vector<std::uint32_t> params;       // key string ids, in map order
    std::vector<std::uint32_t> annotations;  // key string ids of numeric annotations, in map order

    auto operator<=>(const Shape&) const = default;
};

// ---------------------------------------------------------------------------------------------
// Reader

class Reader
{
public:
    explicit Reader(std::string_view data)
        : p(data.data())
        , e(data.data() + data.size())
    {}

    std::size_t remaining() const
    {
        return static_cast<std::size_t>(e - p);
    }

    std::uint8_t byte()
    {
        if (p >= e) {
            fail("unexpected end of data");
        }
        return static_cast<std::uint8_t>(*p++);
    }

    std::string_view bytes(std::size_t n)
    {
        if (n > remaining()) {
            fail("unexpected end of data");
        }
        const std::string_view v(p, n);
        p += n;
        return v;
    }

    std::uint64_t varint()
    {
        std::uint64_t v = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            const std::uint8_t b = byte();
            v |= static_cast<std::uint64_t>(b & 0x7F) << shift;
            if (!(b & 0x80)) {
                return v;
            }
        }
        fail("number is too long");
    }

    // A count of items that each need at least \a itemSize bytes of the remaining data.
    std::size_t count(std::size_t itemSize = 1)
    {
        const std::uint64_t n = varint();
        if (n > remaining() / itemSize) {
            fail("count is larger than the data");
        }
        return static_cast<std::size_t>(n);
    }

    std::int64_t zigzag()
    {
        const std::uint64_t z = varint();
        return static_cast<std::int64_t>(z >> 1) ^ -static_cast<std::int64_t>(z & 1);
    }

    double real()
    {
        const std::string_view raw = bytes(8);
        std::uint64_t bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(raw[i])) << (8 * i);
        }
        double v = 0;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    std::vector<double> column()
    {
        const std::uint64_t n = varint();
        const auto encoding = static_cast<Encoding>(byte());
        if (encoding == Encoding::RawDouble) {
            if (n > remaining() / 8) {
                fail("column is larger than the data");
            }
            std::vector<double> values(static_cast<std::size_t>(n));
            for (double& v : values) {
                v = real();
            }
            return values;
        }
        if (encoding != Encoding::QuantDelta) {
            fail("unknown column encoding");
        }
        if (n > remaining()) {
            fail("column is larger than the data");
        }
        std::vector<double> values(static_cast<std::size_t>(n));
        // Unsigned arithmetic: a damaged file must not cause signed overflow.
        std::uint64_t prev = 0;
        for (double& v : values) {
            prev += static_cast<std::uint64_t>(zigzag());
            v = static_cast<double>(static_cast<std::int64_t>(prev)) * Resolution;
        }
        return values;
    }

private:
    const char* p;
    const char* e;
};

struct Column
{
    std::vector<double> values;
    std::size_t pos = 0;

    double next()
    {
        if (pos >= values.size()) {
            fail("column is too short");
        }
        return values[pos++];
    }
};

struct ResolvedShape
{
    const std::string* name = nullptr;
    bool hasToolAxis = false;
    std::vector<std::pair<const std::string*, Column*>> params;
    std::vector<std::pair<const std::string*, Column*>> annotations;
};

}  // namespace

// ---------------------------------------------------------------------------------------------

bool isBinary(std::string_view data)
{
    return data.substr(0, Magic.size()) == Magic;
}

std::string encode(const std::vector<Command>& commands, const AnnotationMap& annotations)
{
    StringTable strings;
    std::map<Shape, std::uint32_t> shapeIds;
    std::vector<const Shape*> shapes;
    std::vector<std::uint32_t> commandShape;
    commandShape.reserve(commands.size());
    std::map<std::uint32_t, std::vector<double>> paramColumns;
    std::map<std::uint32_t, std::vector<double>> annotationColumns;
    std::array<std::vector<double>, 3> toolAxisColumns;

    struct StringAnnotation
    {
        std::size_t command;
        std::uint32_t key;
        std::uint32_t value;
    };
    std::vector<StringAnnotation> stringAnnotations;

    Shape shape;
    for (std::size_t i = 0; i < commands.size(); ++i) {
        const Command& cmd = commands[i];
        shape.name = strings.id(cmd.Name);
        shape.flags = 0;
        if (cmd.ToolAxis) {
            shape.flags |= ShapeHasToolAxis;
            toolAxisColumns[0].push_back(cmd.ToolAxis->x);
            toolAxisColumns[1].push_back(cmd.ToolAxis->y);
            toolAxisColumns[2].push_back(cmd.ToolAxis->z);
        }
        shape.params.clear();
        shape.annotations.clear();
        for (const auto& [key, value] : cmd.Parameters) {
            const std::uint32_t id = strings.id(key);
            shape.params.push_back(id);
            paramColumns[id].push_back(value);
        }
        for (const auto& [key, value] : cmd.Annotations) {
            const std::uint32_t id = strings.id(key);
            if (const double* number = std::get_if<double>(&value)) {
                shape.annotations.push_back(id);
                annotationColumns[id].push_back(*number);
            }
            else {
                stringAnnotations.push_back({i, id, strings.id(std::get<std::string>(value))});
            }
        }
        const auto [it, added] = shapeIds.try_emplace(shape, static_cast<std::uint32_t>(shapes.size()));
        if (added) {
            shapes.push_back(&it->first);
        }
        commandShape.push_back(it->second);
    }

    // Register the strings of the Toolpath annotations before the table is written.
    for (const auto& [key, value] : annotations) {
        strings.id(key);
        if (const std::string* text = std::get_if<std::string>(&value)) {
            strings.id(*text);
        }
    }

    Writer w(commands.size() * 12 + 64);
    w.bytes(Magic);
    w.byte(FormatVersion);
    w.varint(commands.size());
    strings.write(w);

    w.varint(shapes.size());
    for (const Shape* s : shapes) {
        w.varint(s->name);
        w.byte(s->flags);
        w.varint(s->params.size());
        for (std::uint32_t id : s->params) {
            w.varint(id);
        }
        w.varint(s->annotations.size());
        for (std::uint32_t id : s->annotations) {
            w.varint(id);
        }
    }
    for (std::uint32_t id : commandShape) {
        w.varint(id);
    }

    w.varint(paramColumns.size());
    for (const auto& [id, values] : paramColumns) {
        w.varint(id);
        w.column(values);
    }
    w.varint(annotationColumns.size());
    for (const auto& [id, values] : annotationColumns) {
        w.varint(id);
        w.column(values);
    }

    const bool hasToolAxis = !toolAxisColumns[0].empty();
    w.byte(hasToolAxis ? 1 : 0);
    if (hasToolAxis) {
        for (const auto& values : toolAxisColumns) {
            w.column(values);
        }
    }

    w.varint(stringAnnotations.size());
    std::size_t prev = 0;
    for (const StringAnnotation& a : stringAnnotations) {
        w.varint(a.command - prev);
        prev = a.command;
        w.varint(a.key);
        w.varint(a.value);
    }

    w.varint(annotations.size());
    for (const auto& [key, value] : annotations) {
        w.varint(strings.id(key));
        if (const std::string* text = std::get_if<std::string>(&value)) {
            w.byte(0);
            w.varint(strings.id(*text));
        }
        else {
            w.byte(1);
            w.real(std::get<double>(value));
        }
    }
    return std::move(w.out);
}

void decode(std::string_view data, std::vector<Command>& commands, AnnotationMap& annotations)
{
    if (!isBinary(data)) {
        fail("magic bytes not found");
    }
    Reader r(data.substr(Magic.size()));
    const std::uint8_t version = r.byte();
    if (version != FormatVersion) {
        fail("unsupported format version");
    }
    const std::size_t commandCount = r.count();

    std::vector<std::string> strings(r.count());
    for (std::string& s : strings) {
        s = std::string(r.bytes(r.count()));
    }
    const auto stringAt = [&strings](std::uint64_t id) -> const std::string* {
        if (id >= strings.size()) {
            fail("string id out of range");
        }
        return &strings[static_cast<std::size_t>(id)];
    };

    // Shapes name their columns by key string id. The columns follow the shapes
    // in the data, so read the shapes with ids first and resolve them after.
    struct RawShape
    {
        std::uint64_t name;
        std::uint8_t flags;
        std::vector<std::uint64_t> params;
        std::vector<std::uint64_t> annotations;
    };
    std::vector<RawShape> rawShapes(r.count(4));
    for (RawShape& s : rawShapes) {
        s.name = r.varint();
        s.flags = r.byte();
        if (s.flags & ~ShapeHasToolAxis) {
            fail("unknown shape flags");
        }
        s.params.resize(r.count());
        for (auto& id : s.params) {
            id = r.varint();
        }
        s.annotations.resize(r.count());
        for (auto& id : s.annotations) {
            id = r.varint();
        }
    }

    std::vector<std::uint32_t> commandShape(commandCount);
    for (auto& id : commandShape) {
        const std::uint64_t v = r.varint();
        if (v >= rawShapes.size()) {
            fail("shape id out of range");
        }
        id = static_cast<std::uint32_t>(v);
    }

    const auto readColumns = [&r, &strings]() {
        std::map<std::uint64_t, Column> columns;
        const std::size_t n = r.count(2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint64_t id = r.varint();
            if (id >= strings.size()) {
                fail("string id out of range");
            }
            if (!columns.try_emplace(id, Column {r.column(), 0}).second) {
                fail("duplicate column");
            }
        }
        return columns;
    };
    std::map<std::uint64_t, Column> paramColumns = readColumns();
    std::map<std::uint64_t, Column> annotationColumns = readColumns();

    std::array<Column, 3> toolAxisColumns;
    switch (r.byte()) {
        case 0:
            break;
        case 1:
            for (Column& column : toolAxisColumns) {
                column.values = r.column();
            }
            break;
        default:
            fail("unknown tool axis flag");
    }

    const auto columnOf = [](std::map<std::uint64_t, Column>& columns, std::uint64_t id) {
        const auto it = columns.find(id);
        if (it == columns.end()) {
            fail("shape names a column that does not exist");
        }
        return &it->second;
    };
    std::vector<ResolvedShape> shapes(rawShapes.size());
    for (std::size_t i = 0; i < rawShapes.size(); ++i) {
        shapes[i].name = stringAt(rawShapes[i].name);
        shapes[i].hasToolAxis = (rawShapes[i].flags & ShapeHasToolAxis) != 0;
        for (std::uint64_t id : rawShapes[i].params) {
            shapes[i].params.emplace_back(stringAt(id), columnOf(paramColumns, id));
        }
        for (std::uint64_t id : rawShapes[i].annotations) {
            shapes[i].annotations.emplace_back(stringAt(id), columnOf(annotationColumns, id));
        }
    }

    std::vector<Command> result(commandCount);
    for (std::size_t i = 0; i < commandCount; ++i) {
        Command& cmd = result[i];
        const ResolvedShape& shape = shapes[commandShape[i]];
        cmd.Name = *shape.name;
        for (const auto& [key, column] : shape.params) {
            cmd.Parameters.emplace_hint(cmd.Parameters.end(), *key, column->next());
        }
        for (const auto& [key, column] : shape.annotations) {
            cmd.Annotations.emplace_hint(cmd.Annotations.end(), *key, column->next());
        }
        if (shape.hasToolAxis) {
            const double x = toolAxisColumns[0].next();
            const double y = toolAxisColumns[1].next();
            const double z = toolAxisColumns[2].next();
            cmd.ToolAxis = Base::Vector3d(x, y, z);
        }
    }

    const std::size_t stringAnnotationCount = r.count(3);
    std::uint64_t index = 0;
    for (std::size_t i = 0; i < stringAnnotationCount; ++i) {
        index += r.varint();
        if (index >= commandCount) {
            fail("annotation names a command that does not exist");
        }
        const std::string* key = stringAt(r.varint());
        const std::string* value = stringAt(r.varint());
        result[static_cast<std::size_t>(index)].Annotations[*key] = *value;
    }

    AnnotationMap toolpathAnnotations;
    const std::size_t annotationCount = r.count(3);
    for (std::size_t i = 0; i < annotationCount; ++i) {
        const std::string* key = stringAt(r.varint());
        switch (r.byte()) {
            case 0:
                toolpathAnnotations[*key] = *stringAt(r.varint());
                break;
            case 1:
                toolpathAnnotations[*key] = r.real();
                break;
            default:
                fail("unknown annotation type");
        }
    }

    commands = std::move(result);
    annotations = std::move(toolpathAnnotations);
}

void decode(std::string_view data, std::vector<Command>& commands)
{
    AnnotationMap ignored;
    decode(data, commands, ignored);
}

}  // namespace Path::ToolpathStore
