/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QDateTime>
#include <boost/random.hpp>
#include <algorithm>
#include <cmath>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>
#endif

#include <fmt/ranges.h>

#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include "Constraint.h"
#include "ConstraintPy.h"


using namespace Sketcher;
using namespace Base;


TYPESYSTEM_SOURCE(Sketcher::Constraint, Base::Persistence)

Constraint::Constraint()
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    // The random number generator is not threadsafe so we guard it.  See
    // https://www.boost.org/doc/libs/1_62_0/libs/uuid/uuid.html#Design%20notes
    static boost::mt19937 ran;
    static bool seeded = false;
    static boost::mutex random_number_mutex;

    boost::lock_guard<boost::mutex> guard(random_number_mutex);

    if (!seeded) {
        ran.seed(QDateTime::currentMSecsSinceEpoch() & 0xffffffff);
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);

    tag = gen();
}

Constraint* Constraint::clone() const
{
    return new Constraint(*this);
}

Constraint* Constraint::copy() const
{
    Constraint* temp = new Constraint();
    temp->Value = this->Value;
    temp->Type = this->Type;
    temp->AlignmentType = this->AlignmentType;
    temp->Name = this->Name;
    temp->LabelDistance = this->LabelDistance;
    temp->LabelPosition = this->LabelPosition;
    temp->isDriving = this->isDriving;
    temp->InternalAlignmentIndex = this->InternalAlignmentIndex;
    temp->isInVirtualSpace = this->isInVirtualSpace;
    temp->isActive = this->isActive;
    temp->elements = this->elements;
    // Do not copy tag, otherwise it is considered a clone, and a "rename" by the expression engine.

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    temp->First = this->First;
    temp->FirstPos = this->FirstPos;
    temp->Second = this->Second;
    temp->SecondPos = this->SecondPos;
    temp->Third = this->Third;
    temp->ThirdPos = this->ThirdPos;
#endif

    return temp;
}

PyObject* Constraint::getPyObject()
{
    return new ConstraintPy(new Constraint(*this));
}

Quantity Constraint::getPresentationValue() const
{
    Quantity quantity;
    switch (Type) {
        case Distance:
        case Radius:
        case Diameter:
        case DistanceX:
        case DistanceY:
            quantity.setValue(Value);
            quantity.setUnit(Unit::Length);
            break;
        case Angle:
            quantity.setValue(toDegrees<double>(Value));
            quantity.setUnit(Unit::Angle);
            break;
        case SnellsLaw:
        case Weight:
            quantity.setValue(Value);
            break;
        default:
            quantity.setValue(Value);
            break;
    }

    QuantityFormat format = quantity.getFormat();
    format.option = QuantityFormat::None;
    format.format = QuantityFormat::Default;
    format.precision = 6;  // QString's default
    quantity.setFormat(format);
    return quantity;
}

unsigned int Constraint::getMemSize() const
{
    return 0;
}

void Constraint::Save(Writer& writer) const
{
    std::string encodeName = encodeAttribute(Name);
    writer.Stream() << writer.ind() << "<Constrain "
                    << "Name=\"" << encodeName << "\" "
                    << "Type=\"" << (int)Type << "\" ";
    if (this->Type == InternalAlignment) {
        writer.Stream() << "InternalAlignmentType=\"" << (int)AlignmentType << "\" "
                        << "InternalAlignmentIndex=\"" << InternalAlignmentIndex << "\" ";
    }
    writer.Stream() << "Value=\"" << Value << "\" "
                    << "LabelDistance=\"" << LabelDistance << "\" "
                    << "LabelPosition=\"" << LabelPosition << "\" "
                    << "IsDriving=\"" << (int)isDriving << "\" "
                    << "IsInVirtualSpace=\"" << (int)isInVirtualSpace << "\" "
                    << "IsActive=\"" << (int)isActive << "\" ";

    // Save elements
    {
        // Ensure backwards compatibility with old versions
        writer.Stream() << "First=\"" << getElement(0).GeoId << "\" "
                        << "FirstPos=\"" << getElement(0).posIdAsInt() << "\" "
                        << "Second=\"" << getElement(1).GeoId << "\" "
                        << "SecondPos=\"" << getElement(1).posIdAsInt() << "\" "
                        << "Third=\"" << getElement(2).GeoId << "\" "
                        << "ThirdPos=\"" << getElement(2).posIdAsInt() << "\" ";
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
        auto elements = std::views::iota(size_t {0}, this->elements.size())
            | std::views::transform([&](size_t i) {
                            return getElement(i);
                        });
#endif
        auto geoIds = elements | std::views::transform([](const GeoElementId& e) {
                          return e.GeoId;
                      });
        auto posIds = elements | std::views::transform([](const GeoElementId& e) {
                          return e.posIdAsInt();
                      });

        const std::string ids = fmt::format("{}", fmt::join(geoIds, " "));
        const std::string positions = fmt::format("{}", fmt::join(posIds, " "));

        writer.Stream() << "ElementIds=\"" << ids << "\" "
                        << "ElementPositions=\"" << positions << "\" ";
    }

    writer.Stream() << "/>\n";
}

void Constraint::Restore(XMLReader& reader)
{
    reader.readElement("Constrain");
    Name = reader.getAttribute<const char*>("Name");
    Type = reader.getAttribute<ConstraintType>("Type");
    Value = reader.getAttribute<double>("Value");

    if (this->Type == InternalAlignment) {
        AlignmentType = reader.getAttribute<InternalAlignmentType>("InternalAlignmentType");

        if (reader.hasAttribute("InternalAlignmentIndex")) {
            InternalAlignmentIndex = reader.getAttribute<long>("InternalAlignmentIndex");
        }
    }
    else {
        AlignmentType = Undef;
    }

    // Read the distance a constraint label has been moved
    if (reader.hasAttribute("LabelDistance")) {
        LabelDistance = (float)reader.getAttribute<double>("LabelDistance");
    }

    if (reader.hasAttribute("LabelPosition")) {
        LabelPosition = (float)reader.getAttribute<double>("LabelPosition");
    }

    if (reader.hasAttribute("IsDriving")) {
        isDriving = reader.getAttribute<bool>("IsDriving");
    }

    if (reader.hasAttribute("IsInVirtualSpace")) {
        isInVirtualSpace = reader.getAttribute<bool>("IsInVirtualSpace");
    }

    if (reader.hasAttribute("IsActive")) {
        isActive = reader.getAttribute<bool>("IsActive");
    }

    if (reader.hasAttribute("ElementIds") && reader.hasAttribute("ElementPositions")) {
        auto splitAndClean = [](std::string_view input) {
            const char delimiter = ' ';

            auto tokens = input | std::views::split(delimiter)
                | std::views::transform([](auto&& subrange) {
                              // workaround due to lack of std::ranges::to in c++20
                              std::string token;
                              auto size = std::ranges::distance(subrange);
                              token.reserve(size);
                              for (char c : subrange) {
                                  token.push_back(c);
                              }
                              return token;
                          })
                | std::views::filter([](const std::string& s) {
                              return !s.empty();
                          });

            return std::vector<std::string>(tokens.begin(), tokens.end());
        };

        const std::string elementIds = reader.getAttribute<const char*>("ElementIds");
        const std::string elementPositions = reader.getAttribute<const char*>("ElementPositions");

        const auto ids = splitAndClean(elementIds);
        const auto positions = splitAndClean(elementPositions);

        if (ids.size() != positions.size()) {
            throw Base::ParserError(fmt::format("ElementIds and ElementPositions do not match in "
                                                "size. Got {} ids and {} positions.",
                                                ids.size(),
                                                positions.size()));
        }

        elements.clear();
        for (size_t i = 0; i < std::min(ids.size(), positions.size()); ++i) {
            const int geoId {std::stoi(ids[i])};
            const PointPos pos {static_cast<PointPos>(std::stoi(positions[i]))};
            addElement(GeoElementId(geoId, pos));
        }
    }

    // Ensure we have at least 3 elements
    while (getElementsSize() < 3) {
        addElement(GeoElementId(GeoEnum::GeoUndef, PointPos::none));
    }

    // Load deprecated First, Second, Third elements
    // These take precedence over the new elements
    // Even though these are deprecated, we still need to read them
    // for compatibility with old files.
    {
        constexpr std::array<const char*, 3> names = {"First", "Second", "Third"};
        constexpr std::array<const char*, 3> posNames = {"FirstPos", "SecondPos", "ThirdPos"};
        static_assert(names.size() == posNames.size());

        for (size_t i = 0; i < names.size(); ++i) {
            if (reader.hasAttribute(names[i])) {
                const int geoId {reader.getAttribute<int>(names[i])};
                const PointPos pos {reader.getAttribute<PointPos>(posNames[i])};
                setElement(i, GeoElementId(geoId, pos));
            }
        }
    }
}

void Constraint::substituteIndex(int fromGeoId, int toGeoId)
{
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    for (size_t i = 0; i < elements.size(); ++i) {
        const GeoElementId element = getElement(i);
        if (element.GeoId == fromGeoId) {
            setElement(i, GeoElementId(toGeoId, element.Pos));
        }
    }
#else
    for (auto& element : elements) {
        if (element.GeoId == fromGeoId) {
            element = GeoElementId(toGeoId, element.Pos);
        }
    }
#endif
}

void Constraint::substituteIndexAndPos(int fromGeoId,
                                       PointPos fromPosId,
                                       int toGeoId,
                                       PointPos toPosId)
{
    const GeoElementId from {fromGeoId, fromPosId};

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    for (size_t i = 0; i < elements.size(); ++i) {
        const GeoElementId element = getElement(i);
        if (element == from) {
            setElement(i, GeoElementId(toGeoId, toPosId));
        }
    }
#else
    for (auto& element : elements) {
        if (element == from) {
            element = GeoElementId(toGeoId, toPosId);
        }
    }
#endif
}

std::string Constraint::typeToString(ConstraintType type)
{
    return type2str[type];
}

std::string Constraint::internalAlignmentTypeToString(InternalAlignmentType alignment)
{
    return internalAlignmentType2str[alignment];
}

bool Constraint::involvesGeoId(int geoId) const
{
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    auto elements =
        std::views::iota(size_t {0}, this->elements.size()) | std::views::transform([&](size_t i) {
            return getElement(i);
        });
#endif
    return std::ranges::any_of(elements, [geoId](const auto& element) {
        return element.GeoId == geoId;
    });
}
/// utility function to check if (`geoId`, `posId`) is one of the points/curves
bool Constraint::involvesGeoIdAndPosId(int geoId, PointPos posId) const
{
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    auto elements =
        std::views::iota(size_t {0}, this->elements.size()) | std::views::transform([&](size_t i) {
            return getElement(i);
        });
#endif
    return std::ranges::find(elements, GeoElementId(geoId, posId)) != elements.end();
}

GeoElementId Constraint::getElement(size_t index) const
{
    if (index >= elements.size()) {
        throw Base::IndexError("Constraint::getElement index out of range");
    }

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    if (index < 3) {
        switch (index) {
            case 0:
                return GeoElementId(First, FirstPos);
            case 1:
                return GeoElementId(Second, SecondPos);
            case 2:
                return GeoElementId(Third, ThirdPos);
        }
    }
#endif
    return elements[index];
}
void Constraint::setElement(size_t index, GeoElementId element)
{
    if (index >= elements.size()) {
        throw Base::IndexError("Constraint::getElement index out of range");
    }

    elements[index] = element;

#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    if (index < 3) {
        switch (index) {
            case 0:
                First = element.GeoId;
                FirstPos = element.Pos;
                break;
            case 1:
                Second = element.GeoId;
                SecondPos = element.Pos;
                break;
            case 2:
                Third = element.GeoId;
                ThirdPos = element.Pos;
                break;
        }
    }
#endif
}

size_t Constraint::getElementsSize() const
{
    return elements.size();
}

void Constraint::addElement(GeoElementId element)
{
#if SKETCHER_CONSTRAINT_USE_LEGACY_ELEMENTS
    int i = elements.size();
    elements.resize(i + 1);
    setElement(i, element);
#else
    elements.push_back(element);
#endif
}
