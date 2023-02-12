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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#endif

#include "License.h"

using namespace App;

std::map<License::Type, License::LicenseItem> License::licenseItems;

License::License(License::Type type)
  : type{type}
{
    init();
}

License::License(long id)
  : type{static_cast<Type>(id)}
{
    if (id < 0 || id > static_cast<long>(Type::Other)) {
        type = Type::Other;
    }

    init();
}

License::License(int id)
  : License(static_cast<long>(id))
{
}

void License::init()
{
    static bool first = true;
    if (!first)
        return;
    first = false;

    licenseItems[Type::AllRightsReserved] = LicenseItem{"All rights reserved",
            "https://en.wikipedia.org/wiki/All_rights_reserved"};
    licenseItems[Type::CC_BY_40] = LicenseItem{"Creative Commons Attribution",
            "https://creativecommons.org/licenses/by/4.0/"};
    licenseItems[Type::CC_BY_SA_40] = LicenseItem{"Creative Commons Attribution-ShareAlike",
            "https://creativecommons.org/licenses/by-sa/4.0/"};
    licenseItems[Type::CC_BY_ND_40] = LicenseItem{"Creative Commons Attribution-NoDerivatives",
            "https://creativecommons.org/licenses/by-nd/4.0/"};
    licenseItems[Type::CC_BY_NC_40] = LicenseItem{"Creative Commons Attribution-NonCommercial",
            "https://creativecommons.org/licenses/by-nc/4.0/"};
    licenseItems[Type::CC_BY_NC_SA_40] = LicenseItem{"Creative Commons Attribution-NonCommercial-ShareAlike",
            "https://creativecommons.org/licenses/by-nc-sa/4.0/"};
    licenseItems[Type::CC_BY_NC_ND_40] = LicenseItem{"Creative Commons Attribution-NonCommercial-NoDerivatives",
            "https://creativecommons.org/licenses/by-nc-nd/4.0/"};
    licenseItems[Type::PublicDomain] = LicenseItem{"Public Domain",
            "https://en.wikipedia.org/wiki/Public_domain"};
    licenseItems[Type::FreeArt] = LicenseItem{"FreeArt", "https://artlibre.org/licence/lal"};
    licenseItems[Type::CERN_OHS_S] = LicenseItem{"CERN Open Hardware Licence strongly-reciprocal",
            "https://cern-ohl.web.cern.ch/"};
    licenseItems[Type::CERN_OHS_W] = LicenseItem{"CERN Open Hardware Licence weakly-reciprocal",
            "https://cern-ohl.web.cern.ch/"};
    licenseItems[Type::CERN_OHS_P] = LicenseItem{"CERN Open Hardware Licence permissive",
            "https://cern-ohl.web.cern.ch/"};
    licenseItems[Type::Other] = LicenseItem{"Other", ""};
}

License::Type License::getType() const
{
    return type;
}

std::string License::getLicense() const
{
    return licenseItems.at(type).license;
}

std::string License::getUrl() const
{
    return licenseItems.at(type).url;
}

std::vector<std::string> License::getLicenses()
{
    init();
    std::vector<std::string> output;
    output.reserve(licenseItems.size());
    using Value = std::pair<Type, LicenseItem>;
    std::transform(licenseItems.cbegin(), licenseItems.cend(), std::back_inserter(output), [](const Value& val) {
        return val.second.license;
    });

    return output;
}
