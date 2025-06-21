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
    temp->First = this->First;
    temp->FirstPos = this->FirstPos;
    temp->Second = this->Second;
    temp->SecondPos = this->SecondPos;
    temp->Third = this->Third;
    temp->ThirdPos = this->ThirdPos;
    temp->LabelDistance = this->LabelDistance;
    temp->LabelPosition = this->LabelPosition;
    temp->isDriving = this->isDriving;
    temp->InternalAlignmentIndex = this->InternalAlignmentIndex;
    temp->isInVirtualSpace = this->isInVirtualSpace;
    temp->isActive = this->isActive;
    temp->elements = this->elements;
    // Do not copy tag, otherwise it is considered a clone, and a "rename" by the expression engine.
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
        writer.Stream() << "First=\"" << First << "\" "
                        << "FirstPos=\"" << (int)FirstPos << "\" "
                        << "Second=\"" << Second << "\" "
                        << "SecondPos=\"" << (int)SecondPos << "\" "
                        << "Third=\"" << Third << "\" "
                        << "ThirdPos=\"" << (int)ThirdPos << "\" ";


        std::stringstream ssIds;
        std::stringstream ssPositions;
        for (size_t i = 0; i < elements.size(); ++i) {
            GeoElementId element = elements[i];
            // Old way takes precedence for the first three elements.
            switch (i) {
                case 0:
                    element = GeoElementId(First, FirstPos);
                    break;
                case 1:
                    element = GeoElementId(Second, SecondPos);
                    break;
                case 2:
                    element = GeoElementId(Third, ThirdPos);
                    break;
            }

            ssIds << element.GeoId << " ";
            ssPositions << element.posIdAsInt() << " ";
        }
        std::string ids = ssIds.str();
        std::string positions = ssPositions.str();

        // Remove trailing spaces
        if (!ids.empty() && ids.back() == ' ') {
            ids.pop_back();
            positions.pop_back();
        }

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
        auto splitAndClean = [](const std::string& input) {
            const char delimiter = ' ';
            auto tokens =
                input | std::views::split(delimiter) | std::views::transform([](auto&& subrange) {
                    return std::string(std::ranges::begin(subrange), std::ranges::end(subrange));
                })
                | std::views::filter([](const std::string& s) {
                      return !s.empty();
                  });

            // C++20 does not have std::ranges::to, so we need to convert the view to a vector
            // manually. This is a workaround until C++23 is supported.
            std::vector<std::string> result;
            for (const auto& token : tokens) {
                result.push_back(token);
            }
            return result;
        };

        const std::string elementIds = reader.getAttribute<const char*>("ElementIds");
        const std::string elementPositions = reader.getAttribute<const char*>("ElementPositions");

        const auto ids = splitAndClean(elementIds);
        const auto positions = splitAndClean(elementPositions);

        if (ids.size() != positions.size()) {
            throw Base::ParserError("ElementIds and ElementPositions do not match in size. Got "
                                    + std::to_string(ids.size()) + " ids and "
                                    + std::to_string(positions.size()) + " positions.");
        }

        elements.clear();

        std::ranges::transform(ids,
                               positions,
                               std::back_inserter(elements),
                               [](const std::string& id, const std::string& pos) {
                                   return GeoElementId(std::stoi(id),
                                                       static_cast<PointPos>(std::stoi(pos)));
                               });

        while (elements.size() < 3) {
            // Ensure we have at least 3 elements
            elements.emplace_back(GeoEnum::GeoUndef, PointPos::none);
        }
    }
    else {
        // Fallback: default to 3 empty elements
        elements = {GeoElementId(), GeoElementId(), GeoElementId()};
    }

    // Load deprecated First, Second, Third elements
    // These take precedence over the new elements
    // Even though these are deprecated, we still need to read them
    // for compatibility with old files.
    {
        if (reader.hasAttribute("First")) {
            setElement(0,
                       GeoElementId(reader.getAttribute<long>("First"),
                                    reader.getAttribute<PointPos>("FirstPos")));
        }
        else {
            const auto& element = elements[0];
            First = element.GeoId;
            FirstPos = element.Pos;
        }
        if (reader.hasAttribute("Second")) {
            setElement(1,
                       GeoElementId(reader.getAttribute<long>("Second"),
                                    reader.getAttribute<PointPos>("SecondPos")));
        }
        else {
            const auto& element = elements[1];
            Second = element.GeoId;
            SecondPos = element.Pos;
        }
        if (reader.hasAttribute("Third")) {
            setElement(2,
                       GeoElementId(reader.getAttribute<long>("Third"),
                                    reader.getAttribute<PointPos>("ThirdPos")));
        }
        else {
            const auto& element = elements[2];
            Third = element.GeoId;
            ThirdPos = element.Pos;
        }
    }
}

void Constraint::substituteIndex(int fromGeoId, int toGeoId)
{
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].GeoId == fromGeoId) {
            setElement(i, GeoElementId(toGeoId, elements[i].Pos));
        }
    }
}

void Constraint::substituteIndexAndPos(int fromGeoId,
                                       PointPos fromPosId,
                                       int toGeoId,
                                       PointPos toPosId)
{
    for (size_t i = 0; i < elements.size(); ++i) {
        if (elements[i].GeoId == fromGeoId && elements[i].Pos == fromPosId) {
            setElement(i, GeoElementId(toGeoId, toPosId));
        }
    }
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
    bool foundInElements = std::ranges::any_of(elements, [geoId](const auto& element) {
        return element.GeoId == geoId;
    });

    // Old way, deprecated - can be removed in the future
    foundInElements = foundInElements || First == geoId || Second == geoId || Third == geoId;

    return foundInElements;
}
/// utility function to check if (`geoId`, `posId`) is one of the points/curves
bool Constraint::involvesGeoIdAndPosId(int geoId, PointPos posId) const
{
    bool foundInElements =
        std::ranges::find(elements, GeoElementId(geoId, posId)) != elements.end();

    // Old way, deprecated - can be removed in the future
    foundInElements = foundInElements || (First == geoId && FirstPos == posId)
        || (Second == geoId && SecondPos == posId) || (Third == geoId && ThirdPos == posId);

    return foundInElements;
}


GeoElementId Constraint::getElement(size_t index)  // const
{
    {
        // Support for old code that uses First, Second, Third directly.
        // Can be made const when the old code is removed.
        elements[0] = GeoElementId(First, FirstPos);
        elements[1] = GeoElementId(Second, SecondPos);
        elements[2] = GeoElementId(Third, ThirdPos);
    }
    return elements[index];
}
void Constraint::setElement(size_t index, GeoElementId element)
{
    {
        // Support for old code that uses First, Second, Third directly.
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
    elements[index] = element;
}

size_t Constraint::getElementsSize() const
{
    return elements.size();
}

void Constraint::addElement(GeoElementId element)
{
    elements.push_back(element);
}
