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
#include <cmath>
#include <sstream>
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
    : Value(0.0)
    , Type(None)
    , AlignmentType(Undef)
    , LabelDistance(10.f)
    , LabelPosition(0.f)
    , isDriving(true)
    , InternalAlignmentIndex(-1)
    , isInVirtualSpace(false)
    , isActive(true)
    , isTextHeight(true)
    , elements(3)
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
    temp->elements = this->elements;
    temp->LabelDistance = this->LabelDistance;
    temp->LabelPosition = this->LabelPosition;
    temp->isDriving = this->isDriving;
    temp->InternalAlignmentIndex = this->InternalAlignmentIndex;
    temp->isInVirtualSpace = this->isInVirtualSpace;
    temp->isActive = this->isActive;
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
    std::string encodeText = encodeAttribute(Text);
    std::string encodeFont = encodeAttribute(Font);
    writer.Stream() << writer.ind() << "<Constrain "
                    << "Name=\"" << encodeName << "\" "
                    << "Text=\"" << encodeText << "\" "
                    << "Font=\"" << encodeFont << "\" "
                    << "Type=\"" << (int)Type << "\" ";
    if (this->Type == InternalAlignment) {
        writer.Stream() << "InternalAlignmentType=\"" << (int)AlignmentType << "\" "
                        << "InternalAlignmentIndex=\"" << InternalAlignmentIndex << "\" ";
    }

    // FOR FORWARD COMPATIBILITY: Write the new vector-based attribute
    std::stringstream elements_str;
    for (const auto& elem : elements) {
        elements_str << elem.GeoId << "," << elem.posIdAsInt() << ";";
    }
    writer.Stream() << "Elements=\"" << elements_str.str() << "\" ";

    // BACKWARD COMPATIBILITY: Write the old attributes First, FirstPos...
    writer.Stream() << "Value=\"" << Value << "\" "
                    << "First=\"" << getGeoId(0) << "\" "
                    << "FirstPos=\"" << getPosIdAsInt(0) << "\" "
                    << "Second=\"" << getGeoId(1) << "\" "
                    << "SecondPos=\"" << getPosIdAsInt(1) << "\" "
                    << "Third=\"" << getGeoId(2) << "\" "
                    << "ThirdPos=\"" << getPosIdAsInt(2) << "\" "
                    << "LabelDistance=\"" << LabelDistance << "\" "
                    << "LabelPosition=\"" << LabelPosition << "\" "
                    << "IsDriving=\"" << (int)isDriving << "\" "
                    << "IsInVirtualSpace=\"" << (int)isInVirtualSpace << "\" "
                    << "IsActive=\"" << (int)isActive << "\" />"

                    << std::endl;
}

void Constraint::Restore(XMLReader& reader)
{
    reader.readElement("Constrain");
    Name = reader.getAttribute<const char*>("Name");
    Text = reader.hasAttribute("Text") ? reader.getAttribute<const char*>("Text") : "";
    Font = reader.hasAttribute("Font") ? reader.getAttribute<const char*>("Font") : "";
    Type = reader.getAttribute<ConstraintType>("Type");
    Value = reader.getAttribute<double>("Value");


    elements.clear();

    // --- FORWARD COMPATIBILITY: Check for the new attribute first ---
    if (reader.hasAttribute("Elements")) {
        // New format found, parse it
        std::string elements_str = reader.getAttribute<const char*>("Elements");
        std::stringstream ss(elements_str);
        std::string segment;

        // Split the string by the semicolon delimiter
        while (std::getline(ss, segment, ';')) {
            if (segment.empty()) {
                continue;
            }

            std::stringstream segment_ss(segment);
            std::string id_str, pos_str;

            // Split the segment by the comma delimiter
            if (std::getline(segment_ss, id_str, ',') && std::getline(segment_ss, pos_str)) {
                int geoId = std::stoi(id_str);
                PointPos pos = (PointPos)std::stoi(pos_str);
                elements.emplace_back(geoId, pos);
            }
        }
    }
    else {
        // --- FALLBACK: Old file, Read the old format ---
        int firstId = reader.getAttribute<long>("First");
        PointPos firstPos = reader.getAttribute<PointPos>("FirstPos");
        if (firstId != GeoEnum::GeoUndef) {
            elements.emplace_back(firstId, firstPos);
        }

        int secondId = reader.getAttribute<long>("Second");
        PointPos secondPos = reader.getAttribute<PointPos>("SecondPos");
        if (secondId != GeoEnum::GeoUndef) {
            elements.emplace_back(secondId, secondPos);
        }

        if (reader.hasAttribute("Third")) {
            int thirdId = reader.getAttribute<long>("Third");
            PointPos thirdPos = reader.getAttribute<PointPos>("ThirdPos");
            if (thirdId != GeoEnum::GeoUndef) {
                elements.emplace_back(thirdId, thirdPos);
            }
        }
    }

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
}

void Constraint::substituteIndex(int fromGeoId, int toGeoId)
{
    for (auto& elt : elements) {
        if (elt.GeoId == fromGeoId) {
            elt.GeoId = toGeoId;
        }
    }
}

void Constraint::substituteIndexAndPos(int fromGeoId,
                                       PointPos fromPosId,
                                       int toGeoId,
                                       PointPos toPosId)
{
    for (auto& elt : elements) {
        if (elt.GeoId == fromGeoId && elt.Pos == fromPosId) {
            elt.GeoId = toGeoId;
            elt.Pos = toPosId;
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

GeoElementId Constraint::getElement(int index) const
{
    return hasElement(index) ? elements[index] : GeoElementId();
}

int Constraint::getGeoId(int index) const
{
    return hasElement(index) ? elements[index].GeoId : GeoEnum::GeoUndef;
}

PointPos Constraint::getPosId(int index) const
{
    return hasElement(index) ? elements[index].Pos : PointPos::none;
}

int Constraint::getPosIdAsInt(int index) const
{
    return hasElement(index) ? elements[index].posIdAsInt() : 0;
}

bool Constraint::hasElement(int index) const
{
    return index >= 0 && index < elements.size();
}

bool Constraint::isElementsEmpty() const
{
    return elements.empty();
}

void Constraint::truncateElements(size_t newSize)
{
    if (newSize < elements.size()) {
        elements.resize(newSize);
    }
}

void Constraint::pushBackElement(GeoElementId elt)
{
    elements.push_back(elt);
}

void Constraint::setElement(int index, GeoElementId elt)
{
    if (ensureElementExists(index)) {
        elements[index] = elt;
    }
}

void Constraint::setGeoId(int index, int geoId)
{
    if (ensureElementExists(index)) {
        elements[index].GeoId = geoId;
    }
}

void Constraint::setPosId(int index, PointPos pos)
{
    if (ensureElementExists(index)) {
        elements[index].Pos = pos;
    }
}

void Constraint::setPosId(int index, int pos)
{
    if (ensureElementExists(index)) {
        elements[index].Pos = static_cast<PointPos>(pos);
    }
}

bool Constraint::ensureElementExists(int index)
{
    if (index < 0) {
        return false;  // Indicate failure for an invalid index
    }
    if (index >= elements.size()) {
        elements.resize(index + 1);
    }
    return true;
}


void Constraint::swapElements(int index1, int index2)
{
    if (index1 == index2) {
        return;
    }
    if (ensureElementExists(index1) && ensureElementExists(index2)) {
        std::swap(elements[index1], elements[index2]);
    }
}
