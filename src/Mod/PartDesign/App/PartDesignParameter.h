// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTDESIGN_PARAMETER_H
#define PARTDESIGN_PARAMETER_H


#include <Base/ParameterObserver.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesign
{

/** Convenient class to obtain PartDesign related parameters
 *
 * The parameters are under group "User parameter:BaseApp/Preferences/Mod/PartDesign"
 */
class PartDesignExport PartDesignParameter : public Base::ParameterObserver
{
public:
    PartDesignParameter();
    static PartDesignParameter* instance();

    bool getAllowCompoundDefault() const;
    void setAllowCompoundDefault(bool v);

private:
    void setup();
};

} // namespace PartDesign

#endif // PARTDESIGN_PARAMETER_H
