/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#include "SoFCDetail.h"

SO_DETAIL_SOURCE(SoFCDetail);

SoFCDetail::SoFCDetail(void)
{
}

SoFCDetail::~SoFCDetail()
{
}

void
SoFCDetail::initClass(void)
{
  SO_DETAIL_INIT_CLASS(SoFCDetail, SoDetail);
}

SoDetail *
SoFCDetail::copy(void) const
{
  SoFCDetail *copy = new SoFCDetail();
  copy->indexArray = this->indexArray;
  return copy;
}

void
SoFCDetail::setIndices(Type type, std::set<int> &&indices)
{
  if(type >= 0 && type < TypeMax)
    indexArray[type] = std::move(indices);
}

bool
SoFCDetail::addIndex(Type type, int index)
{
  if(type >= 0 && type < TypeMax)
    return indexArray[type].insert(index).second;
  return false;
}

bool
SoFCDetail::removeIndex(Type type, int index)
{
  if(type >= 0 && type < TypeMax)
    return indexArray[type].erase(index);
  return false;
}

const std::set<int> &
SoFCDetail::getIndices(Type type) const
{
  if(type < 0 || type >= TypeMax) {
    static std::set<int> none;
    return none;
  }
  return indexArray[type];
}

// vim: noai:ts=2:sw=2
