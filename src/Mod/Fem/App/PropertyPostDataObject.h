/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef FEM_PROPERTYPOSTDATASET_H
#define FEM_PROPERTYPOSTDATASET_H

#include <vtkDataObject.h>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

#include <App/Property.h>
#include <Mod/Fem/FemGlobal.h>


namespace Fem
{

/** The vtk data set property class.
 * @author Stefan Tröger
 */
class FemExport PropertyPostDataObject : public App::Property
{
    TYPESYSTEM_HEADER();

public:
    PropertyPostDataObject();
    ~PropertyPostDataObject();

    /** @name Getter/setter */
    //@{
    /// Scale the point coordinates of the data set with factor \a s
    void scale(double s);
    /// set the dataset
    void setValue(const vtkSmartPointer<vtkDataObject>&);
    /// get the part shape
    const vtkSmartPointer<vtkDataObject>& getValue(void) const;
    /// check if we hold a dataset or a dataobject (which would mean a composite data structure)
    bool isDataSet();
    bool isComposite();
    int  getDataType();
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject(void);
    void setPyObject(PyObject *value);
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    App::Property *Copy(void) const;
    void Paste(const App::Property &from);
    unsigned int getMemSize (void) const;
    //@}

    /// Get valid paths for this property; used by auto completer
    virtual void getPaths(std::vector<App::ObjectIdentifier> & paths) const;

private:
    static void scaleDataObject(vtkDataObject*, double s);

protected:
    void createDataObjectByExternalType(vtkSmartPointer<vtkDataObject> ex);
    vtkSmartPointer<vtkDataObject> m_dataObject;
};

} //namespace FEM


#endif // FEM_PROPERTYPOSTDATASET_H
