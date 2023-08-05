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

#ifndef PROPERTYTRAJECTORY_H
#define PROPERTYTRAJECTORY_H

#include <App/Property.h>
#include <Base/BoundBox.h>

#include "Trajectory.h"


namespace Robot
{

/** The part shape property class.
 * @author Werner Mayer
 */
class RobotExport PropertyTrajectory : public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyTrajectory();
    ~PropertyTrajectory() override;

    /** @name Getter/setter */
    //@{
    /// set the part shape
    void setValue(const Trajectory&);
    /// get the part shape
    const Trajectory &getValue() const;
    //@}

 
    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const;
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject() override;
    void setPyObject(PyObject *value) override;
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    App::Property *Copy() const override;
    void Paste(const App::Property &from) override;
    unsigned int getMemSize () const override;
    //@}

private:
    Trajectory _Trajectory;
};


} //namespace Robot


#endif // PROPERTYTOPOSHAPE_H
