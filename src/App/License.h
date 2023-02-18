/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef APP_LICENSE_H
#define APP_LICENSE_H

#include <FCGlobal.h>
#include <string>
#include <map>
#include <vector>

namespace App {

/*!
 * \brief The License class
 * Handling of standard licenses.
 */
class AppExport License
{
public:
    enum class Type {
        AllRightsReserved,
        CC_BY_40,
        CC_BY_SA_40,
        CC_BY_ND_40,
        CC_BY_NC_40,
        CC_BY_NC_SA_40,
        CC_BY_NC_ND_40,
        PublicDomain,
        FreeArt,
        CERN_OHS_S,
        CERN_OHS_W,
        CERN_OHS_P,
        Other
    };

    explicit License(Type);
    explicit License(long);
    explicit License(int);
    Type getType() const;
    std::string getLicense() const;
    std::string getUrl() const;

    static std::vector<std::string> getLicenses();

private:
    static void init();

    struct LicenseItem{
        std::string license;
        std::string url;
    };
    Type type;

    static std::map<Type, LicenseItem> licenseItems;
};

}

#endif // APP_LICENSE_H
