/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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

#ifndef APP_CONTAINERS_EXCEPTIONS_H
#define APP_CONTAINERS_EXCEPTIONS_H

#include <Base/Exception.h>
#include <FCConfig.h>

#include <unordered_set> //for private function
#include <CXX/Objects.hxx>

namespace App {


class AppExport ContainerError: public Base::Exception
{
    TYPESYSTEM_HEADER();
public:
    ContainerError(){}
    ContainerError(const char* message)
        : Exception(message){}
    ContainerError(const std::string& message)
        : Exception(message){}
    static PyObject* PyExc;
    virtual PyObject* getPyExcClass() const;
public://statics
    static void initContainerExceptionTypes();
    static void registerPyExceptions(PyObject* module);
};

#define DEFINE_CONTAINER_EXCEPTION(classname)                     \
    class AppExport classname: public ContainerError              \
    {                                                             \
        TYPESYSTEM_HEADER();                                      \
    public:                                                       \
        classname(){}                                             \
        classname(const char* message)                            \
            : ContainerError(message){}                           \
        classname(const std::string& message)                     \
            : ContainerError(message){}                           \
        static PyObject* PyExc;                                   \
        virtual PyObject* getPyExcClass() const override;            \
    }

/**
 * @brief ContainerTreeError: raised if container tree is a graph (a child has multiple parents for example)
 */
DEFINE_CONTAINER_EXCEPTION(ContainerTreeError);

/**
 * @brief AlreadyInContainerError: raised when adding an object to a container, but it is already in another container.
 */
DEFINE_CONTAINER_EXCEPTION(AlreadyInContainerError);

/**
 * @brief ContainerUnsupportedError class : the action is not supported by this kind of container (e.g. newObject to Origin)
 */
DEFINE_CONTAINER_EXCEPTION(ContainerUnsupportedError);

/**
 * @brief RejectedByContainerError: thrown by Container::canAccept when the specific container doesn't accept the object (e.g. PartDesign Body will refuse to take a Part::Feature).
 */
DEFINE_CONTAINER_EXCEPTION(RejectedByContainerError);

/**
 * @brief NotAContainerError: raised when requesting a container interface around an object that is not a container
 */
DEFINE_CONTAINER_EXCEPTION(NotAContainerError);

/**
 * @brief SpecialChildError: raised when attempting to remove a  static child from the container (e.g., Origin).
 */
DEFINE_CONTAINER_EXCEPTION(SpecialChildError);

/**
 * @brief NullContainerError: raised if container doesn't exist
 */
DEFINE_CONTAINER_EXCEPTION(NullContainerError);

/**
 * @brief ObjectNotFoundError: raised by getObject(name)
 */
DEFINE_CONTAINER_EXCEPTION(ObjectNotFoundError);



//--------------------------------------------------------

/**
  * @brief macro CONTAINERBASEPY_STDCATCH_ATTR: catch for exceptions in attribute
  * code (re-throws the exception converted to Py::Exception). It is a helper
  * to avoid repeating the same error handling code over and over again.
  */
#define CONTAINERBASEPY_STDCATCH_ATTR \
    catch (ContainerError &e) {\
        throw Py::Exception(e.getPyExcClass(), e.what());\
    } catch (Base::Exception &e) {\
        throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());\
    }


/**
  * @brief macro CONTAINERPY_STDCATCH_METH: catch for exceptions in method code
  * (returns NULL if an exception is caught). It is a helper to avoid repeating
  * the same error handling code over and over again.
  */
#define CONTAINERPY_STDCATCH_METH \
    catch (ContainerError &e) {\
        PyErr_SetString(e.getPyExcClass(), e.what());\
        return NULL;\
    } catch (Base::Exception &e) {\
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());\
        return NULL;\
    } catch (const Py::Exception &){\
        return NULL;\
    }




}//namespace App
#endif // APP_CONTAINERS_EXCEPTIONS_H
