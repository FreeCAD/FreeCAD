/***************************************************************************
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

//! a class for establishing our connection with the unified measurement facility
//! we are treating sketches like Part objects for now

#include "PreCompiled.h"

#include <App/Application.h>
#include <App/MeasureManager.h>
#include "Base/Console.h"
#include "Measure.h"


void Sketcher::Measure::initialize()
{
    const App::MeasureHandler& handler = App::MeasureManager::getMeasureHandler("Part");

    App::MeasureManager::addMeasureHandler("Sketcher", handler.typeCb);
}
