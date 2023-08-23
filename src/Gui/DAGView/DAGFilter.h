/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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

#ifndef DAGFILTER_H
#define DAGFILTER_H

#include <QString>

#include "DAGModelGraph.h"

namespace Gui
{
  class ViewProviderDocumentObject;
  namespace DAG
  {
    class FilterBase
    {
    public:
      enum class Type
      {
        None = 0, //!< no type designation. shouldn't be used.
        Inclusion,
        Exclusion
      };
      FilterBase();
      //! @return is whether we have a match or not.
      virtual bool goFilter(const Vertex &vertexIn, const Graph &graphIn, const GraphLinkContainer &linkIn) const = 0;
      QString name;
      bool enabled = true;
      Type type = Type::Exclusion;
    };

    /*! Hide all children of app::origin that are not
     * used by subsequent features
     */
    class FilterOrigin : public FilterBase
    {
    public:
      FilterOrigin();
      bool goFilter(const Vertex &vertexIn, const Graph &graphIn, const GraphLinkContainer &linkIn) const override;
    };

    /*! Hide nodes of type*/
    class FilterTyped : public FilterBase
    {
    public:
      explicit FilterTyped(const std::string &typeIn);
      std::string type;
      bool goFilter(const Vertex &vertexIn, const Graph &graphIn, const GraphLinkContainer &linkIn) const override;
    };
  }
}

#endif // DAGFILTER_H
