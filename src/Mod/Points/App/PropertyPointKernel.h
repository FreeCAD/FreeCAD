/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef POINTS_PROPERTYPOINTKERNEL_H
#define POINTS_PROPERTYPOINTKERNEL_H

#include "Points.h"

namespace Points
{

/** The point kernel property
 */
class PointsExport PropertyPointKernel : public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    PropertyPointKernel();
    ~PropertyPointKernel();

    /** @name Getter/setter */
    //@{
    /// Sets the points to the property
    void setValue( const PointKernel& m);
    /// get the points (only const possible!)
    const PointKernel &getValue() const;
    const Data::ComplexGeoData* getComplexData() const;
    void setTransform(const Base::Matrix4D& rclTrf);
    Base::Matrix4D getTransform() const;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const;
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject();
    void setPyObject(PyObject *value);
    //@}

    /** @name Undo/Redo */
    //@{
    /// returns a new copy of the property (mainly for Undo/Redo and transactions)
    App::Property *Copy() const;
    /// paste the value from the property (mainly for Undo/Redo and transactions)
    void Paste(const App::Property &from);
    unsigned int getMemSize () const;
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);
    //@}

    /** @name Modification */
    //@{
    PointKernel* startEditing();
    void finishEditing();
    /// Transform the real 3d point kernel
    void transformGeometry(const Base::Matrix4D &rclMat);
    void removeIndices( const std::vector<unsigned long>& );
    //@}

private:
    Base::Reference<PointKernel> _cPoints;
};

} // namespace Points


#endif // POINTS_PROPERTYPOINTKERNEL_H 
